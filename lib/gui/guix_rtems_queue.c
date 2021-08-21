#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <rtems.h>
#include <rtems/thread.h>
#include <rtems/score/chainimpl.h>

#include "gx_api.h"
#include "gx_widget.h"
#include "gx_system.h"
#include "gx_system_rtos_bind.h"

#include "base/observer.h"

/* GUIX system events */
#define GUIX_TIMER_WAKEUP_EVENT RTEMS_EVENT_0
#define GUIX_TIMER_DONE_EVENT   RTEMS_EVENT_1

struct guix_event {
    Chain_Node node;
    GX_EVENT event;
};

struct guix_struct {
    Chain_Control pending;
    Chain_Control free;
    rtems_mutex qmutex;
    rtems_condition_variable qcond;
    rtems_mutex mutex;
    rtems_id timer;
    rtems_id thread;
#if (CONFIG_GUIX_MEMPOOL_SIZE > 0)
    rtems_id mpool_id;
    void *mpool;
#endif
    bool timer_running;
    bool timer_active;
    VOID (*entry)(ULONG);
    struct guix_event events[GX_MAX_QUEUE_EVENTS];
};

static struct guix_struct guix_class;


/*
 * memory pool for guix
 */
#if (CONFIG_GUIX_MEMPOOL_SIZE > 0)
static int guix_memory_pool_init(void *start, size_t size)
{
    rtems_status_code sc;
    sc = rtems_region_create(rtems_build_name('g','u','i','x'),
        start, size, sizeof(long), RTEMS_LOCAL, &guix_class.mpool_id);
    return rtems_status_code_to_errno(sc);
}
#endif

static void *guix_memory_allocate(ULONG size)
{
#if (CONFIG_GUIX_MEMPOOL_SIZE > 0)
    void *p = NULL;
    rtems_region_get_segment(guix_class.mpool_id, (uintptr_t)size,
        RTEMS_WAIT, RTEMS_NO_TIMEOUT, &p);
    return p;
#else
    return malloc((size_t)size);
#endif
}

static void guix_memory_free(void *ptr)
{
#if (CONFIG_GUIX_MEMPOOL_SIZE > 0)
    rtems_region_return_segment(guix_class.mpool_id, ptr);
#else
    free(ptr);
#endif
}

/* 
 * GUIX notify interface
 */
static struct observer_base *guix_suspend_notifier_list;
static rtems_mutex guix_notifier_lock;

int guix_suspend_notify_register(struct observer_base *observer)
{
    int ret;
    rtems_mutex_lock(&guix_notifier_lock);
    ret = observer_cond_register(&guix_suspend_notifier_list,
            observer);
    rtems_mutex_unlock(&guix_notifier_lock);
    return ret;
}

int guix_suspend_notify_unregister(struct observer_base *observer)
{
    int ret;
    rtems_mutex_lock(&guix_notifier_lock);
    ret = observer_unregister(&guix_suspend_notifier_list,
            observer);
    rtems_mutex_unlock(&guix_notifier_lock);
    return ret;
}

static int _guix_suspend_notify(unsigned int state)
{
    int ret;
    rtems_mutex_lock(&guix_notifier_lock);
    ret = observer_notify(&guix_suspend_notifier_list, state, NULL);
    rtems_mutex_unlock(&guix_notifier_lock);
    return ret;
}

static void guix_wait_event(rtems_event_set in)
{
    rtems_status_code sc;
    rtems_event_set evt;
    
    sc = rtems_event_receive(in, RTEMS_WAIT | RTEMS_EVENT_ALL, 
        RTEMS_NO_TIMEOUT, &evt);
    assert(sc == RTEMS_SUCCESSFUL);
}

static inline void guix_wake_up(rtems_id thread, rtems_event_set evt)
{
    rtems_event_send(thread, evt);
}

static rtems_task guix_timer_server(rtems_task_argument arg)
{
    struct guix_struct *gx = &guix_class;
    rtems_interval ticks;
    
    ticks = rtems_configuration_get_microseconds_per_tick();
    ticks = (1000UL * GX_SYSTEM_TIMER_MS) / ticks;
    if (ticks == 0)
        ticks = 1;

    if (!gx->timer_running)
        rtems_task_suspend(RTEMS_SELF);
    gx->timer_active = true;
    
    while (true) {
        rtems_task_wake_after(ticks);

        while (!gx->timer_active) {
            guix_wake_up(gx->thread, GUIX_TIMER_DONE_EVENT);
            guix_wait_event(GUIX_TIMER_WAKEUP_EVENT);
        }
        
        _gx_system_timer_expiration(0);
    }
}

static void guix_thread_adaptor(rtems_task_argument arg)
{
    struct guix_struct *gx = (struct guix_struct *)arg;
    GX_EVENT event;
    bool wake_up ;
    UINT ret;

    while (true) {
        
        /* Process GUI event */
        gx->entry(0);

        /* GUIX thread exited and stop timer */
        if (!rtems_task_is_suspended(gx->timer)) {
            wake_up = true;
            gx->timer_active = false;
            guix_wait_event(GUIX_TIMER_DONE_EVENT);
        } else {
            wake_up = false;
        }

        /* Notify system that gui will enter in suspend state */
        _guix_suspend_notify(GUIX_ENTER_SLEEP);
        
        /* Flush event and wait */
        do {
            ret = gx_generic_event_pop(&event, GX_FALSE);
            if (ret == GX_FAILURE) {
                ret = gx_generic_event_pop(&event, GX_TRUE);
                break;
            }
        } while (true);

        /* Refresh screen */
        event.gx_event_type = GX_EVENT_REDRAW;
        event.gx_event_target = NULL;
        gx_system_event_send(&event);

        /* Notify system that gui is ready */
        _guix_suspend_notify(GUIX_EXIT_SLEEP);
        
        /* Wake up GUIX timer */
        if (wake_up) {
            gx->timer_active = true;
            guix_wake_up(gx->timer, GUIX_TIMER_WAKEUP_EVENT);
        }
    }
}

/* 
 * rtos initialize: perform any setup that needs to be done 
 * before the GUIX task runs here 
 */
VOID gx_generic_rtos_initialize(VOID)
{
    struct guix_struct *gx = &guix_class;

    _Chain_Initialize_empty(&gx->pending);
    _Chain_Initialize_empty(&gx->free);
    _Chain_Initialize(&gx->free, gx->events, GX_MAX_QUEUE_EVENTS, 
        sizeof(struct guix_event));
	rtems_mutex_init(&gx->mutex, "guix_system");
    rtems_mutex_init(&gx->qmutex, "guix_qevent");
    rtems_condition_variable_init(&gx->qcond, "guix_qevent");
    rtems_mutex_init(&guix_notifier_lock, "guix_notifier");
    
#if (CONFIG_GUIX_MEMPOOL_SIZE > 0)
    gx->mpool = malloc(CONFIG_GUIX_MEMPOOL_SIZE);
    if (gx->mpool == NULL)
        return;

    int ret;
    ret = guix_memory_pool_init(gx->mpool, CONFIG_GUIX_MEMPOOL_SIZE);
    if (ret) {
        printf("%s create memory pool failed with error %d\n", __func__, ret);
        return;
    }
 #endif

    gx_system_memory_allocator_set(guix_memory_allocate, 
        guix_memory_free);
}

/* thread_start: start the GUIX thread running. */
UINT gx_generic_thread_start(VOID(*guix_thread_entry)(ULONG))
{
    struct guix_struct *gx = &guix_class;
    rtems_status_code sc;

    gx->entry = guix_thread_entry;
    sc = rtems_task_create(rtems_build_name('g', 'u', 'i', 'x'), 
        GX_SYSTEM_THREAD_PRIORITY, GX_THREAD_STACK_SIZE, 
        RTEMS_PREEMPT | RTEMS_NO_TIMESLICE,
        RTEMS_NO_FLOATING_POINT | RTEMS_LOCAL, &gx->thread);
    if (sc != RTEMS_SUCCESSFUL) {
        printf("%s create guix thread failed(%s)\n", __func__, 
            rtems_status_text(sc));
        return GX_FAILURE;
    }

    sc = rtems_task_create(rtems_build_name('g', 'x', 't', 'm'), 
        GX_SYSTEM_THREAD_PRIORITY, GX_TIMER_THREAD_STACK_SIZE, 
        RTEMS_PREEMPT | RTEMS_NO_TIMESLICE,
        RTEMS_NO_FLOATING_POINT | RTEMS_LOCAL, &gx->timer);
    if (sc != RTEMS_SUCCESSFUL) {
        printf("%s create guix timer-server failed(%s)\n", __func__, 
            rtems_status_text(sc));
        rtems_task_delete(gx->thread);
        return GX_FAILURE;
    }

    sc = rtems_task_start(gx->timer, guix_timer_server, 
        (rtems_task_argument)gx);
    if (sc != RTEMS_SUCCESSFUL) {
        printf("%s start guix timer-server failed(%s)\n", __func__, 
            rtems_status_text(sc));
        return GX_FAILURE;
    }
    
    sc = rtems_task_start(gx->thread, guix_thread_adaptor, 
        (rtems_task_argument)gx);
    if (sc != RTEMS_SUCCESSFUL) {
        printf("%s start guix thread failed(%s)\n", __func__, 
            rtems_status_text(sc));
        return GX_FAILURE;
    }

    return GX_SUCCESS;
}

/* event_post: push an event into the fifo event queue */
UINT gx_generic_event_post(GX_EVENT *event_ptr)
{
    struct guix_struct *gx = &guix_class;
    struct guix_event *ge;

    rtems_mutex_lock(&gx->qmutex);
    if (_Chain_Is_empty(&gx->free)) {
        rtems_mutex_unlock(&gx->qmutex);
        return GX_FAILURE;
    }
    
    ge = (struct guix_event *)_Chain_Get_first_unprotected(&gx->free);
    ge->event = *event_ptr;
    _Chain_Append_unprotected(&gx->pending, &ge->node);
    if (_Chain_Has_only_one_node(&gx->pending))
        rtems_condition_variable_signal(&gx->qcond);
    rtems_mutex_unlock(&gx->qmutex);
    
    return GX_SUCCESS;
}

/* event_fold: update existing matching event, otherwise post new event */
UINT gx_generic_event_fold(GX_EVENT *event_ptr)
{
    struct guix_struct *gx = &guix_class;
    struct guix_event *ge;
    GX_EVENT *e;
   
    rtems_mutex_lock(&gx->qmutex);
    Chain_Control *head = &gx->pending;
    Chain_Node *iter = _Chain_First(head);
    
    while (iter != _Chain_Tail(head)) {
        ge = (struct guix_event *)iter;
        e = &ge->event;
    
        if (e->gx_event_type == event_ptr->gx_event_type &&
            e->gx_event_target == event_ptr->gx_event_target) {

            /* for timer event, update tick count */
            if (e->gx_event_type == GX_EVENT_TIMER) {
                e->gx_event_payload.gx_event_ulongdata++;
            } else {
                e->gx_event_payload.gx_event_ulongdata =
                    event_ptr->gx_event_payload.gx_event_ulongdata;
            }

            if (e->gx_event_type == GX_EVENT_PEN_DRAG)
                 _gx_system_pen_speed_update(&e->gx_event_payload.gx_event_pointdata);
            rtems_mutex_unlock(&gx->qmutex);
            return GX_SUCCESS;
        }

        iter = _Chain_Next(iter);
    }

    if (_Chain_Is_empty(&gx->free)) {
        rtems_mutex_unlock(&gx->qmutex);
        return GX_FAILURE;
    }
    ge = (struct guix_event *)_Chain_Get_first_unprotected(&gx->free);
    ge->event = *event_ptr;
    _Chain_Append_unprotected(&gx->pending, &ge->node);
    if (_Chain_Has_only_one_node(&gx->pending))
        rtems_condition_variable_signal(&gx->qcond);
    rtems_mutex_unlock(&gx->qmutex);
    return GX_SUCCESS;
}

/* event_pop: pop oldest event from fifo queue, block if wait and no events exist */
UINT gx_generic_event_pop(GX_EVENT *put_event, GX_BOOL wait)
{
    struct guix_struct *gx = &guix_class;
    struct guix_event *ge;
    
    if (!wait && _Chain_Is_empty(&gx->pending))
       return GX_FAILURE;

    rtems_mutex_lock(&gx->qmutex);
    while (_Chain_Is_empty(&gx->pending))
        rtems_condition_variable_wait(&gx->qcond, &gx->qmutex);

    ge = (struct guix_event *)_Chain_Get_first_unprotected(&gx->pending);
    *put_event = ge->event;
    _Chain_Append_unprotected(&gx->free, &ge->node);
    rtems_mutex_unlock(&gx->qmutex);
    return GX_SUCCESS;
}

/* event_purge: delete events targetted to particular widget */
VOID gx_generic_event_purge(GX_WIDGET *target)
{
    struct guix_struct *gx = &guix_class;
    GX_BOOL purge;
    
    rtems_mutex_lock(&gx->qmutex);
    Chain_Control *head = &gx->pending;
    Chain_Node *iter = _Chain_First(head);

    while (iter != _Chain_Tail(head)) {
        struct guix_event *ge = (struct guix_event *)iter;
        purge = GX_FALSE;
        if (ge->event.gx_event_target) {
            if (ge->event.gx_event_target == target) {
                purge = GX_TRUE;
            } else {
                gx_widget_child_detect(target, ge->event.gx_event_target,
                    &purge);
            }
            if (purge == GX_TRUE) {
                _Chain_Extract_unprotected(iter);
                _Chain_Append_unprotected(&gx->free, iter);
            }
        }
        iter = _Chain_Next(iter);
    }
    rtems_mutex_unlock(&gx->qmutex);
}

/* start the RTOS timer */
VOID gx_generic_timer_start(VOID)
{
    struct guix_struct *gx = &guix_class;
    if (!gx->timer_running) {
        gx->timer_running = true;
        rtems_task_resume(gx->timer);
    }
}

/* stop the RTOS timer */
VOID gx_generic_timer_stop(VOID)
{
    struct guix_struct *gx = &guix_class;
    if (gx->timer_running) {
        gx->timer_running = false;
        rtems_task_suspend(gx->timer);
    }
}

/* lock the system protection mutex */
VOID gx_generic_system_mutex_lock(VOID)
{
    struct guix_struct *gx = &guix_class;
    rtems_mutex_lock(&gx->mutex);
}

/* unlock system protection mutex */
VOID gx_generic_system_mutex_unlock(VOID)
{
    struct guix_struct *gx = &guix_class;
    rtems_mutex_unlock(&gx->mutex);
}

/* return number of low-level system timer ticks. Used for pen speed caculations */
ULONG gx_generic_system_time_get(VOID)
{
    return (ULONG)rtems_clock_get_ticks_since_boot();
}

/* thread_identify: return current thread identifier, cast as VOID * */
VOID *gx_generic_thread_identify(VOID)
{
    return (VOID *)rtems_task_self();
}

VOID gx_generic_time_delay(int ticks)
{
    rtems_task_wake_after((rtems_interval)ticks);
}

