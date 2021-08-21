#include <rtems/sysinit.h>
#include <rtems/rtems/cache.h>
#include <bsp/irq-generic.h>
#include <bsp/start.h>
#include <bsp.h>

#include "base/macro.h"

#include "board/stm32f7/stm32f7_pinmux.h"
#include "board/stm32f7/stm32_pinmux.h"

#include "stm32_ll_utils.h"
#include "stm32_ll_rcc.h"

#define DMA_BUF_SIZE KB(128)

#define STM32F7_PINMUX_FUN_CAN \
  (STM32_PINMUX_ALT_FUNC_9 | STM32_PUSHPULL_PULLUP | STM32_OSPEEDR_HIGH_SPEED)


static struct pin_config board_gpio[] = {
    /* UART 1 */
    {STM32_PIN_PA9, STM32F7_PINMUX_FUNC_PA9_USART1_TX},
    {STM32_PIN_PB7, STM32F7_PINMUX_FUNC_PB7_USART1_RX},

    /* UART 6 */
    {STM32_PIN_PC6, STM32F7_PINMUX_FUNC_PC6_USART6_TX},
    {STM32_PIN_PC7, STM32F7_PINMUX_FUNC_PC7_USART6_RX},

    /* PWM */
    {STM32_PIN_PB4, STM32F7_PINMUX_FUNC_PB4_PWM3_CH1},

    /* Ethernet */
    {STM32_PIN_PC1, STM32F7_PINMUX_FUNC_PC1_ETH},
    {STM32_PIN_PC4, STM32F7_PINMUX_FUNC_PC4_ETH},
    {STM32_PIN_PC5, STM32F7_PINMUX_FUNC_PC5_ETH},
    {STM32_PIN_PA1, STM32F7_PINMUX_FUNC_PA1_ETH},
    {STM32_PIN_PA2, STM32F7_PINMUX_FUNC_PA2_ETH},
    {STM32_PIN_PA7, STM32F7_PINMUX_FUNC_PA7_ETH},
    {STM32_PIN_PG11, STM32F7_PINMUX_FUNC_PG11_ETH},
    {STM32_PIN_PG13, STM32F7_PINMUX_FUNC_PG13_ETH},
    {STM32_PIN_PG14, STM32F7_PINMUX_FUNC_PG14_ETH},

    /* I2C-1 */
    {STM32_PIN_PB8, STM32F7_PINMUX_FUNC_PB8_I2C1_SCL},
    {STM32_PIN_PB9, STM32F7_PINMUX_FUNC_PB9_I2C1_SDA},

    /* SPI-2 */
#ifdef CONFIG_SPI_STM32_USE_HW_SS
    {STM32_PIN_PI0,  STM32F7_PINMUX_FUNC_PI0_SPI2_NSS},
#endif /* CONFIG_SPI_STM32_USE_HW_SS */
    {STM32_PIN_PI1,  STM32F7_PINMUX_FUNC_PI1_SPI2_SCK},
    {STM32_PIN_PB14, STM32F7_PINMUX_FUNC_PB14_SPI2_MISO},
    {STM32_PIN_PB15, STM32F7_PINMUX_FUNC_PB15_SPI2_MOSI},

    /* USB */
    {STM32_PIN_PA11, STM32F7_PINMUX_FUN_CAN},
    {STM32_PIN_PA12, STM32F7_PINMUX_FUN_CAN},
    
    //{STM32_PIN_PA11, STM32F7_PINMUX_FUNC_PA11_OTG_FS_DM},
    //{STM32_PIN_PA12, STM32F7_PINMUX_FUNC_PA12_OTG_FS_DP},
};

static void BSP_START_TEXT_SECTION board_gpio_init(void)
{
	stm32_setup_pins(board_gpio, ARRAY_SIZE(board_gpio));
}

static void BSP_START_TEXT_SECTION cpu_enable_cache(void)
{
    SCB_EnableICache();
    SCB_EnableDCache();
}

#ifndef CONFIG_UBOOT
static void BSP_START_TEXT_SECTION pll_setup(void)
{
    /*
    * System Clock source            = PLL (HSE)
    * SYSCLK(Hz)                     = 200000000
    * HCLK(Hz)                       = 200000000
    * AHB Prescaler                  = 1
    * APB1 Prescaler                 = 4
    * APB2 Prescaler                 = 2
    * HSI Frequency(Hz)              = 25000000
    * PLL_M                          = 25
    * PLL_N                          = 400
    * PLL_P                          = 2
    * PLL_Q                          = 8
    * VDD(V)                         = 3.3
    * Main regulator output voltage  = Scale1 mode
    * Flash Latency(WS) 
    */
    LL_UTILS_PLLInitTypeDef  pllcfg = {
        .PLLM = LL_RCC_PLLM_DIV_25,
        .PLLN = 400,
        .PLLP = LL_RCC_PLLP_DIV_2
    };
    LL_UTILS_ClkInitTypeDef  clkcfg = {
        .AHBCLKDivider = LL_RCC_SYSCLK_DIV_1,
        .APB1CLKDivider = LL_RCC_APB1_DIV_4,
        .APB2CLKDivider = LL_RCC_APB2_DIV_2
    };

    LL_PLL_ConfigSystemClock_HSE(25000000, 0, &pllcfg, &clkcfg);
}
#endif

static char dma_buffer[DMA_BUF_SIZE] __fastdata __aligned(4);

static void board_setup(void)
{
    
    cpu_enable_cache();
#ifndef CONFIG_UBOOT
    pll_setup();
#endif
    board_gpio_init();
    bsp_interrupt_initialize();
    rtems_cache_coherent_add_area(dma_buffer, sizeof(dma_buffer));
}

RTEMS_SYSINIT_ITEM(board_setup,
		   RTEMS_SYSINIT_BSP_EARLY,
	           RTEMS_SYSINIT_ORDER_SECOND);

