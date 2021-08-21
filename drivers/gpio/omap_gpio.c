// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * This work is derived from the linux 2.6.27 kernel source
 * To fetch, use the kernel repository
 * git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux-2.6.git
 * Use the v2.6.27 tag.
 *
 * Below is the original's header including its copyright
 *
 *  linux/arch/arm/plat-omap/gpio.c
 *
 * Support functions for OMAP GPIO
 *
 * Copyright (C) 2003-2005 Nokia Corporation
 * Written by Juha Yrjölä <juha.yrjola@nokia.com>
 */
#include <bsp/irq-generic.h>

#include "common.h"
#include "dm.h"
#include "fdtdec.h"
#include "asm/gpio.h"
#include "asm/io.h"
#include "dm/device-internal.h"
#include "linux/errno.h"

//#if defined(CONFIG_AM43XX)
#define OMAP_GPIO_IRQSTATUS_CLR_0  0x003c
#define OMAP_GPIO_IRQSTATUS_CLR_1  0x0040
//#endif

#define OMAP_GPIO_DIR_OUT	0
#define OMAP_GPIO_DIR_IN	1
#define GPIO_PER_BANK		32

struct gpio_bank {
	phys_addr_t base;	/* address of registers in physical memory */
	int irq;
};

int gpio_is_valid(int gpio)
{
	return (gpio >= 0) && (gpio < OMAP_MAX_GPIO);
}

static void _set_gpio_direction(const struct gpio_bank *bank, int gpio,
				int is_input)
{
	uint32_t l;

	l = __raw_readl(bank->base + OMAP_GPIO_OE);
	if (is_input)
		l |= 1 << gpio;
	else
		l &= ~(1 << gpio);
	__raw_writel(l, bank->base + OMAP_GPIO_OE);
}

static int _get_gpio_direction(const struct gpio_bank *bank, int gpio)
{
	uint32_t v;

	v = __raw_readl(bank->base + OMAP_GPIO_OE);
	return !!(v & BIT(gpio));
}

static void _set_gpio_dataout(const struct gpio_bank *bank, int gpio,
				int enable)
{
	uint32_t reg_offset;

	reg_offset = OMAP_GPIO_CLEARDATAOUT + (enable << 2);
	__raw_writel(BIT(gpio), bank->base + reg_offset);
}

static int _get_gpio_value(const struct gpio_bank *bank, int gpio)
{
	uint32_t reg_offset;
	int v;

	v = _get_gpio_direction(bank, gpio);
	reg_offset = OMAP_GPIO_DATAIN + (v << 2);
	v = __raw_readl(bank->base + reg_offset);
	return !!(v & BIT(gpio));
}

static int omap_gpio_direction_input(struct udevice *dev, unsigned offset)
{
	struct gpio_bank *bank = dev_get_priv(dev);

	_set_gpio_direction(bank, offset, 1);
	return 0;
}

static int omap_gpio_direction_output(struct udevice *dev, unsigned offset,
				       int value)
{
	struct gpio_bank *bank = dev_get_priv(dev);

	_set_gpio_dataout(bank, offset, value);
	_set_gpio_direction(bank, offset, 0);
	return 0;
}

static int omap_gpio_get_value(struct udevice *dev, unsigned offset)
{
	struct gpio_bank *bank = dev_get_priv(dev);

	return _get_gpio_value(bank, offset);
}

static int omap_gpio_set_value(struct udevice *dev, unsigned offset,
				 int value)
{
	struct gpio_bank *bank = dev_get_priv(dev);

	_set_gpio_dataout(bank, offset, value);
	return 0;
}

static int omap_gpio_get_function(struct udevice *dev, unsigned offset)
{
	struct gpio_bank *bank = dev_get_priv(dev);

	if (_get_gpio_direction(bank, offset) == OMAP_GPIO_DIR_OUT)
		return GPIOF_OUTPUT;
	return GPIOF_INPUT;
}

static void omap_gpio_irqhandler(void *arg)
{
	struct udevice *dev = arg;
	struct gpio_bank *bank = dev_get_priv(dev);
    uint32_t pending;

    /* Clear interrupt line */
    pending = readl(bank->base + OMAP_GPIO_IRQSTATUS1);
    writel(pending, bank->base + OMAP_GPIO_IRQSTATUS1);
    while (pending) {
        int index = __builtin_ctz(pending);
        pending &= ~BIT(index);
		gpio_irq_handler(dev, index);
    }
}

static void omap_gpio_irq_mask(struct gpio_desc *desc)
{
	struct gpio_bank *bank = dev_get_priv(desc->dev);

	writel(BIT(desc->offset), 
		bank->base + OMAP_GPIO_IRQSTATUS_CLR_0);
}

static void omap_gpio_irq_unmask(struct gpio_desc *desc)
{
	struct gpio_bank *bank = dev_get_priv(desc->dev);

	writel(BIT(desc->offset), 
		bank->base + OMAP_GPIO_IRQSTATUS_SET_0);
}

static int omap_gpio_set_irq_type(struct gpio_desc *desc, 
	unsigned int type)
{
	struct gpio_bank *bank = dev_get_priv(desc->dev);
    uint32_t level_0, level_1;
    uint32_t edge_0, edge_1;
    uint32_t value;
    uint32_t rvalue;

	value = BIT(desc->offset);
	rvalue = ~value;
	level_0 = readl_relaxed(bank->base + OMAP_GPIO_LEVELDETECT0);
	level_1 = readl_relaxed(bank->base + OMAP_GPIO_LEVELDETECT1);
	edge_0 = readl_relaxed(bank->base + OMAP_GPIO_FALLINGDETECT);
	edge_1 = readl_relaxed(bank->base + OMAP_GPIO_RISINGDETECT);
	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		writel_relaxed(value | edge_1, bank->base + OMAP_GPIO_RISINGDETECT);
		writel_relaxed(rvalue & edge_0, bank->base + OMAP_GPIO_FALLINGDETECT);
		writel_relaxed(rvalue & level_0, bank->base + OMAP_GPIO_LEVELDETECT0);
		writel_relaxed(rvalue & level_1, bank->base + OMAP_GPIO_LEVELDETECT1);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		writel_relaxed(value | edge_0, bank->base + OMAP_GPIO_FALLINGDETECT);
		writel_relaxed(rvalue & edge_1, bank->base + OMAP_GPIO_RISINGDETECT);
		writel_relaxed(rvalue & level_0, bank->base + OMAP_GPIO_LEVELDETECT0);
		writel_relaxed(rvalue & level_1, bank->base + OMAP_GPIO_LEVELDETECT1);
		break;
	case IRQ_TYPE_EDGE_BOTH:
		writel_relaxed(value | edge_0, bank->base + OMAP_GPIO_FALLINGDETECT);
		writel_relaxed(value | edge_1, bank->base + OMAP_GPIO_RISINGDETECT);
		writel_relaxed(rvalue & level_0, bank->base + OMAP_GPIO_LEVELDETECT0);
		writel_relaxed(rvalue & level_1, bank->base + OMAP_GPIO_LEVELDETECT1);
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		writel_relaxed(value | level_1, bank->base + OMAP_GPIO_LEVELDETECT1);
		writel_relaxed(rvalue & edge_0, bank->base + OMAP_GPIO_FALLINGDETECT);
		writel_relaxed(rvalue & edge_1, bank->base + OMAP_GPIO_RISINGDETECT);
		writel_relaxed(rvalue & level_0, bank->base + OMAP_GPIO_LEVELDETECT0);
		break;
	case IRQ_TYPE_LEVEL_LOW:
		writel_relaxed(value | level_0, bank->base + OMAP_GPIO_LEVELDETECT0);
		writel_relaxed(rvalue & edge_0, bank->base + OMAP_GPIO_FALLINGDETECT);
		writel_relaxed(rvalue & edge_1, bank->base + OMAP_GPIO_RISINGDETECT);
		writel_relaxed(rvalue & level_1, bank->base + OMAP_GPIO_LEVELDETECT1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct gpio_irqchip omap_gpio_irqchip = {
	.name = "omap-gpio",
	.irq_mask = omap_gpio_irq_mask,
	.irq_unmask = omap_gpio_irq_unmask,
	.irq_set_type = omap_gpio_set_irq_type
};

static const struct dm_gpio_ops gpio_omap_ops = {
	.direction_input	= omap_gpio_direction_input,
	.direction_output	= omap_gpio_direction_output,
	.get_value		= omap_gpio_get_value,
	.set_value		= omap_gpio_set_value,
	.get_function		= omap_gpio_get_function,
};

static int omap_gpio_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct gpio_bank *bank = dev_get_priv(dev);
	char name[18], *str;
	rtems_status_code sc;

	sprintf(name, "gpio@%4x_", (unsigned int)bank->base);
	str = strdup(name);
	if (!str)
		return -ENOMEM;
	uc_priv->bank_name = str;
	uc_priv->gpio_count = GPIO_PER_BANK;
	uc_priv->irqchip = &omap_gpio_irqchip;
	sc = rtems_interrupt_handler_install(bank->irq, str, 
		RTEMS_INTERRUPT_UNIQUE, omap_gpio_irqhandler, dev);
	return rtems_status_code_to_errno(sc);
}

static const struct udevice_id omap_gpio_ids[] = {
	{ .compatible = "ti,omap3-gpio" },
	{ .compatible = "ti,omap4-gpio" },
	{ .compatible = "ti,am4372-gpio" },
	{ }
};

static int omap_gpio_of_to_plat(struct udevice *dev)
{
	struct gpio_bank *bank = dev_get_priv(dev);

	bank->base = (phys_addr_t)dev_read_addr(dev);
	if (bank->base == FDT_ADDR_T_NONE)
		return -EINVAL;
	bank->irq = (int)dev_read_irq_index(dev, 0, NULL);
	if (bank->irq == FDT_ADDR_T_NONE)
		return -EINVAL;
	return 0;
}

DM_DRIVER(gpio_omap) = {
	.name	= "gpio_omap",
	.id	= UCLASS_GPIO,
	.of_match = omap_gpio_ids,
	.of_to_plat = of_match_ptr(omap_gpio_of_to_plat),
	.ops	= &gpio_omap_ops,
	.probe	= omap_gpio_probe,
	.priv_auto	= sizeof(struct gpio_bank),
};
