#include <rtems/score/basedefs.h>
#include <rtems/sysinit.h>
#include <bsp/irq-generic.h>

#include "base/reboot_listener.h"
#include "am_mcu_apollo.h"
#include "am_hal_gpio.h"


#define IO_NONE 0
#define IO_HIGH 1
#define IO_LOW  2

struct io_config {
    const char *name;
    uint16_t	pin;
    uint16_t	ival;
    const am_hal_gpio_pincfg_t *cfg;	
};

#define _IO_ITEM(_name, _pin, _ival, _cfg) {\
    .name = _name, \
    .pin = _pin, \
    .ival = _ival, \
    .cfg = _cfg }

/*
 * GPIO configure for UART 
 */
static const am_hal_gpio_pincfg_t k_uart0_tx RTEMS_UNUSED = {
    .uFuncSel       = AM_HAL_PIN_22_UART0TX,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA
};

static const am_hal_gpio_pincfg_t k_uart0_rx RTEMS_UNUSED = {
    .uFuncSel       = AM_HAL_PIN_23_UART0RX
};

static const am_hal_gpio_pincfg_t k_uart1_tx RTEMS_UNUSED = {
    .uFuncSel		= AM_HAL_PIN_71_UART1TX,
    .eDriveStrength	= AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA
};

static const am_hal_gpio_pincfg_t k_uart1_rx RTEMS_UNUSED = {
    .uFuncSel		= AM_HAL_PIN_72_UART1RX
};


/*
 * GPIO configure for interrupt 
 */
static const am_hal_gpio_pincfg_t k_gpio_intr RTEMS_UNUSED = {
    .uFuncSel = 3,
    .eIntDir = AM_HAL_GPIO_PIN_INTDIR_HI2LO,
    .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,
};	

/*
 * GPIO congiure for output
 */
static const am_hal_gpio_pincfg_t k_gpio_output RTEMS_UNUSED = {
    .uFuncSel       = 3,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL
};

/*
 * GPIO configure for MSPI0
 */
static const am_hal_gpio_pincfg_t k_mspi0_ce0 RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_10_NCE10,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0,
    .uNCE                = 0,
    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
};

static const am_hal_gpio_pincfg_t k_mspi0_d0 RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_22_MSPI0_0,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_mspi0_d1 RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_26_MSPI0_1,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_mspi0_d2 RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_4_MSPI0_2,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_mspi0_d3 RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_23_MSPI0_3,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_mspi0_sck RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_24_MSPI0_8,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

/*
 * I/O Master
 */
static const am_hal_gpio_pincfg_t k_iom2_scl RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_27_M2SCL,
    .ePullup             = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    .bIomMSPIn           = 1,
    .uIOMnum             = 2
};

static const am_hal_gpio_pincfg_t k_iom2_sda RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_25_M2SDAWIR3,
    .ePullup             = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    .bIomMSPIn           = 1,
    .uIOMnum             = 2
};

/*
 * MSPI1
 */
static const am_hal_gpio_pincfg_t k_mspi1_ce0 RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_50_NCE50,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1,
    .uNCE                = 0,
    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
};

static const am_hal_gpio_pincfg_t k_mspi1_d0 RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_51_MSPI1_0,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};

static const am_hal_gpio_pincfg_t k_mspi1_d1 RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_52_MSPI1_1,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};

static const am_hal_gpio_pincfg_t k_mspi1_d2 RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_53_MSPI1_2,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};

static const am_hal_gpio_pincfg_t k_mspi1_d3 RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_54_MSPI1_3,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};


static const am_hal_gpio_pincfg_t k_mspi1_sck RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_59_MSPI1_8,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};

/*
 * IOM4 (SPI)
 */
static const am_hal_gpio_pincfg_t k_iom4_spi_miso RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_40_M4MISO,
    .bIomMSPIn           = 1,
    .uIOMnum             = 4
};

static const am_hal_gpio_pincfg_t k_iom4_spi_mosi RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_44_M4MOSI,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .bIomMSPIn           = 1,
    .uIOMnum             = 4
};

static const am_hal_gpio_pincfg_t k_iom4_spi_sck RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_39_M4SCK,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .bIomMSPIn           = 1,
    .uIOMnum             = 4
};

const am_hal_gpio_pincfg_t k_iom4_spi_cs RTEMS_UNUSED = {
    .uFuncSel            = AM_HAL_PIN_31_NCE31,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 1,
    .uIOMnum             = 4,
    .uNCE                = 0,
    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
};

/*
 * Board GPIO configure information
 */
static const struct io_config board_gpio[] = {
    _IO_ITEM("uart0_tx",  22, IO_NONE, &k_uart0_tx),
    _IO_ITEM("uart0_rx",  23, IO_NONE, &k_uart0_rx),
};

static void apollo3p_gpio_init(void)
{
    for (int i = 0; i < RTEMS_ARRAY_SIZE(board_gpio); i++) {
        const struct io_config *gpio = board_gpio + i;

        am_hal_gpio_pinconfig(gpio->pin, *gpio->cfg);
        switch (gpio->ival) {
        case IO_LOW:
            am_hal_gpio_output_clear(gpio->pin);
            break;
        case IO_HIGH:
            am_hal_gpio_output_set(gpio->pin);
            break;
        default:
            break;
        }
    }		
}

static void apollo3p_board_init(void)
{
    am_hal_burst_avail_e burst;
       
    /* Set the clock frequency */
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);

    /* Set the default cache configuration */
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();

    /* Configure the board for low power. */
    am_hal_pwrctrl_low_power_init();
    am_hal_burst_mode_initialize(&burst);
    
    if (burst == AM_HAL_BURST_AVAIL) {
        am_hal_burst_mode_e burst_state;
        am_hal_burst_mode_disable(&burst_state);
        am_hal_burst_mode_enable(&burst_state);
    }
    
    apollo3p_gpio_init();
    bsp_interrupt_initialize();
    am_hal_interrupt_master_enable();
}

#ifdef CONFIG_SHELL_REBOOT
static int apollo3p_reboot(struct observer_base *nb, 
    unsigned long action, void *data)
{
    am_hal_sysctrl_aircr_reset();
    return NOTIFY_DONE;
}

static void apollo_reboot_init(void)
{
    static struct observer_base reboot_listener = 
        OBSERVER_STATIC_INIT(apollo3p_reboot, 0);
    reboot_notify_register(&reboot_listener);
}

RTEMS_SYSINIT_ITEM(apollo_reboot_init,
    RTEMS_SYSINIT_DEVICE_DRIVERS, 
    RTEMS_SYSINIT_ORDER_MIDDLE);
#endif

RTEMS_SYSINIT_ITEM(apollo3p_board_init,
    RTEMS_SYSINIT_BSP_EARLY, 
    RTEMS_SYSINIT_ORDER_FIRST);
		
