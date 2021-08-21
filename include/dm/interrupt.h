#ifndef DM_INTERRUPT_H_
#define DM_INTERRUPT_H_

#include <bsp/irq-generic.h>

#ifdef __cplusplus
extern "C"{
#endif

struct udevice;

/*
 * Interrupt flags
 */
#define IRQF_SHARED		0x00000080

#define IRQF_CPU_MASK   0xFF000000
#define IRQF_CPU(n)     (((n) << 24) & IRQF_CPU_MASK) 
#define IRQF_CPU_NR(n)  (((n) & IRQF_CPU_MASK) >> 24)

int devm_request_threaded_irq(struct udevice *dev, unsigned int irq,
                  rtems_interrupt_handler handler, 
                  rtems_interrupt_handler thread_fn,
                  unsigned long irqflags, const char *devname,
                  void *arg);
void devm_free_irq(struct udevice *dev, unsigned int irq, void *arg);

#ifdef __cplusplus
}
#endif
#endif /* DM_INTERRUPT_H_ */
