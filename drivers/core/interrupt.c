#include <stdlib.h>
#include <rtems/sysinit.h>
#include <rtems/bspIo.h>

#include "dm/interrupt.h"
#include "dm/device.h"
#include "dm/devres.h"
#include "dm/device_compat.h"


struct irq_devres {
    rtems_interrupt_server_request req;
    rtems_interrupt_handler handler;
    void *arg;
};

static void devm_irq_release(struct udevice *dev, void *res)
{
    struct irq_devres *this = res;

    rtems_interrupt_handler_remove(this->req.entry.vector, 
        this->handler, this->arg);
}

static int devm_irq_match(struct udevice *dev, void *res, void *data)
{
    struct irq_devres *this = res, *match = data;

    return this->req.entry.vector == match->req.entry.vector && 
            this->req.action.arg == match->req.action.arg;
}

static void threaded_irq_wrappwer(void *arg)
{
    struct irq_devres *dr = arg;
    
    dr->handler(dr->req.action.arg);
    rtems_interrupt_server_request_submit(&dr->req);
}

int devm_request_threaded_irq(struct udevice *dev, unsigned int irq,
                  rtems_interrupt_handler handler, 
                  rtems_interrupt_handler thread_fn,
                  unsigned long irqflags, const char *devname,
                  void *arg)
{
    struct irq_devres *dr;
    rtems_option flags;
    rtems_status_code sc;
    int cpu;
    
    if (handler == NULL && thread_fn == NULL) {
        dev_err(dev, "Interrupt(%u) handler invalid\n", irq);
        return -EINVAL;
    }
    
    cpu = IRQF_CPU_NR(irqflags);
    if (cpu > rtems_scheduler_get_processor()) {
        dev_err(dev, "Interrupt(%d) cpu(%d) is invalid\n", irq, cpu);
        return -EINVAL;
    }

    dr = devres_alloc(devm_irq_release, sizeof(struct irq_devres),
              GFP_KERNEL);
    if (!dr) {
        dev_err(dev, "No more memory\n");
        return -ENOMEM;
    }
    if (!devname)
        devname = dev->name;

    if (irqflags & IRQF_SHARED)
        flags = RTEMS_INTERRUPT_SHARED;
    else
        flags = RTEMS_INTERRUPT_UNIQUE;
        
    rtems_interrupt_server_request_initialize(cpu, &dr->req,
        thread_fn, arg);
    if (handler) {
        if (!thread_fn) {
            sc = rtems_interrupt_handler_install(irq, devname,
                flags, handler, arg);
            if (sc != RTEMS_SUCCESSFUL) 
                goto _free_dr;
            dr->handler = handler;
            dr->arg = arg;
        } else {
            sc = rtems_interrupt_handler_install(irq, devname,
                flags, threaded_irq_wrappwer, dr);
            if (sc != RTEMS_SUCCESSFUL) 
                goto _free_dr;
            dr->handler = threaded_irq_wrappwer;
            dr->arg = dr;
        }
    } else {
        sc = rtems_interrupt_handler_install(irq, devname, flags,
            (rtems_interrupt_handler)rtems_interrupt_server_request_submit, 
            &dr->req);
        if (sc != RTEMS_SUCCESSFUL)
            goto _free_dr;
        dr->handler = 
            (rtems_interrupt_handler)rtems_interrupt_server_request_submit;
        dr->arg = &dr->req;
    }

    dr->req.entry.vector = irq;
    devres_add(dev, dr);
    return 0;
    
_free_dr:
    devres_free(dr);
    dev_err(dev, "Install interrupt(%u) failed: %s\n", irq, 
        rtems_status_text(sc));
    return rtems_status_code_to_errno(sc);
}

void devm_free_irq(struct udevice *dev, unsigned int irq, void *arg)
{
    struct irq_devres match_data;

    match_data.req.action.arg = arg;
    match_data.req.entry.vector = irq;
    devres_destroy(dev, devm_irq_release, devm_irq_match,
                 &match_data);
}

#if !defined(__rtems_libbsd__)
static void irq_init(void)
{
    rtems_status_code sc;

    sc = rtems_interrupt_server_initialize(10, 4096,
        RTEMS_PREEMPT | RTEMS_NO_ASR | RTEMS_NO_TIMESLICE,
        RTEMS_LOCAL, NULL);
    if (sc != RTEMS_SUCCESSFUL)
        printk("***Error: %s()-> %s\n", __func__, rtems_status_text(sc));
}

RTEMS_SYSINIT_ITEM(irq_init,
    RTEMS_SYSINIT_CLASSIC_USER_TASKS,
    RTEMS_SYSINIT_ORDER_MIDDLE);
#endif /* !__rtems_libbsd__ */
