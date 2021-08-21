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

#include <bsp.h>
#include <bsp/start.h>

#if (RTEMS_BSP == stm32f7_libbsd)
#include <bsp/stm32f746xx.h>

static void board_early_puts(const char *s)
{
    USART_TypeDef *reg = USART1;
    
    while (*s != '\0') {
        while (!(reg->ISR & USART_ISR_TXE));
        reg->TDR = *s;
        if (*s == '\n') {
            while (!(reg->ISR & USART_ISR_TXE));
            reg->TDR = '\r';
        }
        s++;
    }
}
#endif

void BSP_START_TEXT_SECTION bsp_start_hook_0(void)
{
#if (RTEMS_BSP == stm32f7)
    extern void SystemInit(void);
    SystemInit();
#elif (RTEMS_BSP == stm32f7_libbsd)
    board_early_puts("\nRTEMS loading success...\n");
#endif
}

void BSP_START_TEXT_SECTION bsp_start_hook_1(void)
{
    bsp_start_copy_sections();
    bsp_start_clear_bss();

    /* At this point we can use objects outside the .start section */
}
