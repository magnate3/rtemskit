#include <rtems.h>

/**
 * @brief This function configures the source of stm32cube time base.
 *        Cube HAL expects a 1ms tick which matches with k_uptime_get_32.
 *        Tick interrupt priority is not used
 * @return HAL status
 */
uint32_t HAL_GetTick(void)
{
    uint32_t ms = rtems_configuration_get_milliseconds_per_tick();
    return rtems_clock_get_ticks_since_boot() * ms;
}

/**
 * @brief This function provides minimum delay (in milliseconds) based
 *	  on variable incremented.
 * @param Delay: specifies the delay time length, in milliseconds.
 * @return None
 */
void HAL_Delay(uint32_t delay)
{
    rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(delay));
}
