#ifndef LINUX_DELAY_H_
#define LINUX_DELAY_H_

#include <rtems.h>
#include <rtems/counter.h>

#ifdef __cplusplus
extern "C"{
#endif

static inline void msleep(unsigned int msecs) 
{
    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(msecs));
}
    
static inline void ssleep(unsigned int seconds)
{
    rtems_interval ticks = RTEMS_MILLISECONDS_TO_TICKS(seconds * 1000);
    rtems_task_wake_after(ticks);
}

static inline rtems_counter_ticks microsec_to_ticks(uint32_t us)
{
    return us * (rtems_counter_frequency() / 1000000ul);
}

static inline rtems_counter_ticks microsec_forward(uint32_t us)
{
    rtems_counter_ticks ticks = microsec_to_ticks(us);
    return rtems_counter_read() + ticks;
}

static inline void udelay(unsigned long us)
{
    rtems_counter_ticks ticks = microsec_to_ticks((uint32_t)us);
    rtems_counter_delay_ticks(ticks);
}

#define ndelay(x) \
    rtems_counter_delay_nanoseconds((uint32_t)x)

#ifdef __cplusplus
}
#endif
#endif /* LINUX_DELAY_H_ */

