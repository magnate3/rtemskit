#ifndef LIBBSP_ARM_STM32F7_BSP_H
#define LIBBSP_ARM_APOLLO3P_BSP_H

#include <bspopts.h>
#include <bsp/default-initial-extension.h>

#include <rtems.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BSP_IDLE_TASK_BODY _BSP_Thread_Idle_body

#define BSP_FEATURE_IRQ_EXTENSION

#define BSP_ARMV7M_IRQ_PRIORITY_DEFAULT (13 << 4)

#define BSP_ARMV7M_SYSTICK_PRIORITY (14 << 4)

#define BSP_ARMV7M_SYSTICK_FREQUENCY 96000000

void *_BSP_Thread_Idle_body( uintptr_t ignored );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIBBSP_ARM_APOLLO3P_BSP_H */
