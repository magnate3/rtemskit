#ifndef STM32_BOARD_PINMUX_H_
#define STM32_BOARD_PINMUX_H_

#include <stdint.h>
#include <stddef.h>

#include "stm32-pinctrl.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pin_config {
	uint8_t pin_num;
	uint32_t mode;
};

struct pinmux_config {
	uint32_t	base_address;
};


#define STM32_PORT(_pin) ((_pin) >> 4)
#define STM32_PIN(_pin)  ((_pin) & 0xf)


void stm32_setup_pins(const struct pin_config *pinconf,size_t pins);
//int  stm32_gpio_configure(uint32_t *base_addr, int pin, int conf, int altf);

#ifdef __cplusplus
}
#endif

#endif /* STM32_BOARD_PINMUX_H_ */

