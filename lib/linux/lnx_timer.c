#include <rtems/rtems/timerimpl.h>
#include <rtems/rtems/clockimpl.h>
#include <rtems/rtems/status.h>
#include <rtems/rtems/support.h>
#include <rtems/score/assert.h>
#include <rtems/score/chainimpl.h>
#include <rtems/score/thread.h>
#include <rtems/score/todimpl.h>
#include <rtems/score/watchdogimpl.h>

#include "linux/timer.h"


static inline Per_CPU_Control *
timer_acquire(struct timer_list *the_timer, ISR_lock_Context *lock_context)
{
	Per_CPU_Control *cpu;
	ISR_Level level;

    _ISR_Local_disable(level);
    _ISR_lock_Context_set_level(lock_context, level );
	cpu = _Watchdog_Get_CPU(&the_timer->Ticker);
	_Watchdog_Per_CPU_acquire_critical(cpu, lock_context);
    return cpu;
}

static inline void 
timer_release(Per_CPU_Control *cpu, ISR_lock_Context *lock_context)
{
    _Watchdog_Per_CPU_release_critical(cpu, lock_context);
    _ISR_lock_ISR_enable(lock_context);
}

static inline bool remove_timer(Per_CPU_Control *cpu, 
    struct timer_list *the_timer)
{
    if (_Watchdog_Is_scheduled(&the_timer->Ticker)) {
        _Watchdog_Remove(
            &cpu->Watchdog.Header[PER_CPU_WATCHDOG_TICKS],
            &the_timer->Ticker
        );
		return true;
    }
    return false;
}

static inline void enqueue_timer(Per_CPU_Control *cpu, 
    struct timer_list *the_timer, unsigned long expires)
{
    _Watchdog_Insert(
        &cpu->Watchdog.Header[PER_CPU_WATCHDOG_TICKS],
        &the_timer->Ticker,
        cpu->Watchdog.ticks + expires
    );    
}

void timer_setup(struct timer_list *the_timer, 
    void (*fn)(struct timer_list *), unsigned int flags)
{
    _Watchdog_Preinitialize(&the_timer->Ticker, _Per_CPU_Get_snapshot());
    the_timer->Ticker.routine = (Watchdog_Service_routine_entry)fn;
	the_timer->flags = flags;
}

void add_timer(struct timer_list *the_timer, unsigned long expires)
{
    ISR_lock_Context lock_context;
    Per_CPU_Control *cpu;

    cpu = timer_acquire(the_timer, &lock_context);
    enqueue_timer(cpu, the_timer, expires);
    timer_release(cpu, &lock_context);
}

int mod_timer(struct timer_list *the_timer, unsigned long expires)
{
    ISR_lock_Context lock_context;
    Per_CPU_Control *cpu;
    bool removed;

    cpu = timer_acquire(the_timer, &lock_context);  
    removed = remove_timer(cpu, the_timer);
    enqueue_timer(cpu, the_timer, expires);
    timer_release(cpu, &lock_context);
    return removed;
}

int del_timer(struct timer_list *the_timer)
{
    ISR_lock_Context lock_context;
    Per_CPU_Control *cpu;
    bool removed;

    cpu = timer_acquire(the_timer, &lock_context);  
    removed = remove_timer(cpu, the_timer);
    timer_release(cpu, &lock_context);
    return removed;
}