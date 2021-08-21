#include "stm32_pinmux.h"
#include "stm32-pinctrl.h"
#include "stm32f7_pinmux.h"

#include "stm32f7xx.h"
#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_ll_bus.h"

#ifndef BIT
#define BIT(n)  (0x1 << (n))
#endif
 
/* base address for where GPIO registers start */
#define GPIO_PORTS_BASE       (GPIOA_BASE)
#define GPIO_REG_SIZE         0x400
#define IOP(_name)            LL_AHB1_GRP1_PERIPH_##_name

static const uint32_t ports_enable[] = {
    IOP(GPIOA),
    IOP(GPIOB),
    IOP(GPIOC),
    IOP(GPIOD),
    IOP(GPIOE),
    IOP(GPIOF),
    IOP(GPIOG),
    IOP(GPIOH),
    IOP(GPIOI),
    IOP(GPIOJ),
    IOP(GPIOK),
};


/**
 * @brief enable IO port clock
 *
 * @param port I/O port ID
 * @param clk  optional clock device
 *
 * @return 0 on success, error otherwise
 */
static void enable_port(uint32_t port)
{
    LL_AHB1_GRP1_EnableClock(ports_enable[port]);
}

static int stm32_gpio_setup(uint32_t *base_addr, int pin, int conf, int altf)
{
	GPIO_TypeDef *gpio = (GPIO_TypeDef *)base_addr;
    uint32_t mode = conf & (STM32_MODER_MASK << STM32_MODER_SHIFT);
    uint32_t otype = conf & (STM32_OTYPER_MASK << STM32_OTYPER_SHIFT);
    uint32_t ospeed = conf & (STM32_OSPEEDR_MASK << STM32_OSPEEDR_SHIFT);
    uint32_t pupd = conf & (STM32_PUPDR_MASK << STM32_PUPDR_SHIFT);
	int pin_ll = BIT(pin);

	LL_GPIO_SetPinMode(gpio, pin_ll, mode >> STM32_MODER_SHIFT);

	if (STM32_MODER_ALT_MODE == mode) {
		if (pin < 8) 
			LL_GPIO_SetAFPin_0_7(gpio, pin_ll, altf);
		else
			LL_GPIO_SetAFPin_8_15(gpio, pin_ll, altf);
	}

	LL_GPIO_SetPinOutputType(gpio, pin_ll, otype >> STM32_OTYPER_SHIFT);
	LL_GPIO_SetPinSpeed(gpio, pin_ll, ospeed >> STM32_OSPEEDR_SHIFT);
	LL_GPIO_SetPinPull(gpio, pin_ll, pupd >> STM32_PUPDR_SHIFT);

	return 0;
}

static int stm32_pin_configure(int pin, int func, int altf)
{
	/* determine IO port registers location */
	uint32_t offset = STM32_PORT(pin) * GPIO_REG_SIZE;
	uint8_t *port_base = (uint8_t *)(GPIO_PORTS_BASE + offset);

	/* not much here, on STM32F10x the alternate function is
	 * controller by setting up GPIO pins in specific mode.
	 */
	return stm32_gpio_setup((uint32_t *)port_base,
				    STM32_PIN(pin), func, altf);
}

/**
 * @brief pin setup
 *
 * @param pin  STM32PIN() encoded pin ID
 * @param func SoC specific function assignment
 * @param clk  optional clock device
 *
 * @return 0 on success, error otherwise
 */
int stm32_set_pinmux(uint32_t pin, uint32_t func)
{
	/* make sure to enable port clock first */
	enable_port(STM32_PORT(pin));

	return stm32_pin_configure(pin, func, func & STM32_AFR_MASK);
}

/**
 * @brief setup pins according to their assignments
 *
 * @param pinconf  board pin configuration array
 * @param pins     array size
 */
void stm32_setup_pins(const struct pin_config *pinconf,size_t pins)
{
	for (int i = 0; i < pins; i++)
		stm32_set_pinmux(pinconf[i].pin_num,  pinconf[i].mode);
}


