/*
 * Copyright (c) 2012 Sebastian Huber.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Obere Lagerstr. 30
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#ifndef LIBBSP_ARM_STM32F7_BSP_H
#define LIBBSP_ARM_STM32F7_BSP_H

#include <bspopts.h>
//#include <bsp/default-initial-extension.h>

#include <rtems.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define __fastdata     __attribute__((section(".bsp_fast_data")))
#define __fasttext     __attribute__((section(".bsp_fast_text")))
#define __stext        __attribute__((section(".bsp_start_text")))
#define __sdata        __attribute__((section(".bsp_start_data")))
#define __cache        __attribute__((section(".bsp_nocache")))


#define BSP_FEATURE_IRQ_EXTENSION

#define BSP_ARMV7M_IRQ_PRIORITY_DEFAULT (13 << 4)

#define BSP_ARMV7M_SYSTICK_PRIORITY (14 << 4)

#define BSP_ARMV7M_SYSTICK_FREQUENCY SystemCoreClock


extern uint32_t SystemCoreClock;

#define stm32f7        0x1001
#define stm32f7_libbsd 0x1002

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */


#endif /* LIBBSP_ARM_STM32F7_BSP_H */
