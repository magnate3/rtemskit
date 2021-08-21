// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2014 The Chromium OS Authors.
 */
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <rtems/malloc.h>
#include <rtems/sysinit.h>

#include "common.h"
#include "dm.h"
#include "dm/device.h"
#include "dm/device-internal.h"
#include "dm/of_access.h"
#include "dm/serial.h"


#define SERIAL_DEFAULT_BDR 115200 

struct serial_class {   
    rtems_termios_device_context base;
    rtems_termios_tty *tty;
    struct udevice *dev;
    char name[16];
    const char *buf;
    size_t total;
    size_t remaining;
    size_t current;
    bool txintr;
};

static struct udevice *serial_console;



int serial_class_enqueue(struct udevice *dev, const char *buf, int len)
{
    struct serial_class *platdata = dev_get_uclass_plat(dev);

    return rtems_termios_enqueue_raw_characters(platdata->tty, buf, len);
}

int serial_class_dequeue(struct udevice *dev)
{
    struct serial_class *platdata = dev_get_uclass_plat(dev);
    const struct dm_serial_ops *ops = serial_get_ops(dev);
    int ret = 0;

    if (platdata->total && ops->tx_empty(dev)) {
        size_t current = platdata->current;
        platdata->buf += current;
        platdata->remaining -= current;
        if (platdata->remaining > 0) {
            platdata->current = ops->fill_fifo(dev, platdata->buf, 
                platdata->remaining);
        } else {
            ret = rtems_termios_dequeue_characters(platdata->tty, 
                platdata->total);
            platdata->total = 0;
        }
    }
    return ret;
}

static void serial_class_txintr_enable(struct serial_class *platdata,
    const struct dm_serial_ops *ops)
{
    rtems_interrupt_lock_context ctx;

    rtems_termios_device_lock_acquire(&platdata->base, &ctx);
    ops->tx_irqena(platdata->dev);
    platdata->txintr = true;
    rtems_termios_device_lock_release(&platdata->base, &ctx);
}

static bool serial_class_txintr_disable(struct serial_class *platdata,
    const struct dm_serial_ops *ops)
{
    rtems_interrupt_lock_context ctx;
    bool state;

    rtems_termios_device_lock_acquire(&platdata->base, &ctx);
    ops->tx_irqdis(platdata->dev);
    state = platdata->txintr;
    platdata->txintr = false;
    rtems_termios_device_lock_release(&platdata->base, &ctx);
    return state;
}

static bool serial_class_open(struct rtems_termios_tty *tty,
                            rtems_termios_device_context *base,
                            struct termios *term,
                            rtems_libio_open_close_args_t *args)
{
    struct serial_class *platdata = container_of(base, 
        struct serial_class, base);
    const struct dm_serial_ops *ops = serial_get_ops(platdata->dev);

    platdata->tty = tty;
    rtems_termios_set_initial_baud(tty, SERIAL_DEFAULT_BDR); // class default baudrate
    return !ops->open(platdata->dev);
}

static void serial_class_close(struct rtems_termios_tty *tty,
                            rtems_termios_device_context *base,
                            rtems_libio_open_close_args_t *args
)
{
    struct serial_class *platdata = container_of(base, 
        struct serial_class, base);
    const struct dm_serial_ops *ops = serial_get_ops(platdata->dev);

    ops->release(platdata->dev);
}

static void serial_class_putc_poll(rtems_termios_device_context *base, 
    char out)
{
    struct serial_class *platdata = container_of(base, 
        struct serial_class, base);
    const struct dm_serial_ops *ops = serial_get_ops(platdata->dev);
    bool enabled;

    enabled = serial_class_txintr_disable(platdata, ops);
    ops->putc_poll(platdata->dev, out);
     if (enabled)
        serial_class_txintr_enable(platdata, ops);
}

static void serial_class_flowctrl_starttx(rtems_termios_device_context *base)
{
    struct serial_class *platdata = container_of(base, struct serial_class, base);
    const struct dm_serial_ops *ops = serial_get_ops(platdata->dev);
    rtems_interrupt_lock_context ctx;

    rtems_termios_device_lock_acquire(base, &ctx);
    ops->set_mctrl(platdata->dev, TIOCM_RTS);
    rtems_termios_device_lock_release(base, &ctx);
}

static void serial_class_flowctrl_stoptx(rtems_termios_device_context *base)
{
    struct serial_class *platdata = container_of(base, struct serial_class, base);
    const struct dm_serial_ops *ops = serial_get_ops(platdata->dev);
    rtems_interrupt_lock_context ctx;

    rtems_termios_device_lock_acquire(base, &ctx);
    ops->set_mctrl(platdata->dev, 0);
    rtems_termios_device_lock_release(base, &ctx);
}

static bool serial_class_termios_set(rtems_termios_device_context *base,
    const struct termios *t)
{
    struct serial_class *platdata = container_of(base, struct serial_class, base);
    struct dm_serial_ops *ops = serial_get_ops(platdata->dev);
    rtems_interrupt_lock_context ctx;
    int ret;

    rtems_termios_device_lock_acquire(base, &ctx);
    ret = ops->set_termios(platdata->dev, t);
    rtems_termios_device_lock_release(base, &ctx);
    return !ret;
}

static void serial_class_xmit_start(rtems_termios_device_context *base,
    const char *buf, size_t len)
{
    struct serial_class *platdata = container_of(base, struct serial_class, base);
    const struct dm_serial_ops *ops = serial_get_ops(platdata->dev);

    platdata->total = len;
    if (len > 0) {
        platdata->remaining = len;
        platdata->buf = buf;
        platdata->current = ops->fill_fifo(platdata->dev, buf, len);
        serial_class_txintr_enable(platdata, ops);
    } else {
        serial_class_txintr_disable(platdata, ops);
    }
}

static const rtems_termios_device_flow serial_flowctrl_ops = {
    .stop_remote_tx  = serial_class_flowctrl_stoptx,
    .start_remote_tx = serial_class_flowctrl_starttx
};

static const rtems_termios_device_handler serial_termios_ops = {
    .first_open     = serial_class_open,
    .last_close     = serial_class_close,
    .write          = serial_class_xmit_start,
    .set_attributes = serial_class_termios_set,
    .mode           = TERMIOS_IRQ_DRIVEN
};

static void serial_console_putc(char c)
{
    struct serial_class *platdata = 
        dev_get_uclass_plat(serial_console);
    serial_class_putc_poll(&platdata->base, c);
}

static int serial_console_parse(void)
{
#if CONFIG_IS_ENABLED(OF_CONTROL)
	struct udevice *dev;

	if (of_live_active()) {
		struct device_node *np = of_get_stdout();
        struct serial_class *platdata;
		if (np && !uclass_get_device_by_ofnode(UCLASS_SERIAL,
				np_to_ofnode(np), &dev)) {
            platdata = dev_get_uclass_plat(dev);
			serial_console = dev; //TODO:
			link(platdata->name, "/dev/console");
			return 0;
		}
	}
    return -EINVAL;
#else
	return -EINVAL;
#endif
}

static int serial_pre_probe(struct udevice *dev)
{
    const struct dm_serial_ops *ops = serial_get_ops(dev);
    const rtems_termios_device_flow *fops = NULL;
    struct serial_class *platdata;
    char name[] = {"/dev/ttyS0"};
    rtems_status_code sc;
    int ret = 0;

    platdata = rtems_calloc(1, sizeof(*platdata));
    if (!platdata) {
        printk("%s, No more memory\n", __func__);
        return -ENOMEM;
    }
    if (ops->set_mctrl)
        fops = &serial_flowctrl_ops;
    platdata->dev = dev;
    name[sizeof(name)-1] += dev_seq(dev);
    strcpy(platdata->name, name); 
    rtems_termios_device_context_initialize(&platdata->base, "UART");
    sc = rtems_termios_device_install(platdata->name, &serial_termios_ops, 
        fops, &platdata->base);
    if (sc != RTEMS_SUCCESSFUL) {
        printk("UART(%s) register failed: %s\n", name, 
            rtems_status_text(sc));
        ret = rtems_status_code_to_errno(sc);
        goto _free;
    }

    dev_set_uclass_plat(dev, platdata);
	return 0;
_free:
    free(platdata);
    return ret;
}

static int serial_post_probe(struct udevice *dev)
{
    if (serial_console == NULL) {
        serial_console_parse();
        BSP_output_char = serial_console_putc;
    }
	return 0;
}

static int serial_pre_remove(struct udevice *dev)
{
    struct serial_class *platdata = dev_get_uclass_plat(dev);
    dev_set_uclass_plat(dev, NULL);
    free(platdata);
	return 0;
}

static int serial_class_init(struct uclass *class)
{
	rtems_termios_initialize();
	return 0;
}

static void dm_serial_init(void)
{
	struct udevice *dev;

	uclass_foreach_dev_probe(UCLASS_SERIAL, dev);
}

RTEMS_SYSINIT_ITEM(dm_serial_init,
    RTEMS_SYSINIT_BSP_PRE_DRIVERS,
    RTEMS_SYSINIT_ORDER_SECOND
);

UCLASS_DRIVER(serial) = {
	.id		    = UCLASS_SERIAL,
	.name		= "serial",
	.flags		= DM_UC_FLAG_SEQ_ALIAS,
	.pre_probe	= serial_pre_probe,
    .post_probe = serial_post_probe,
	.pre_remove	= serial_pre_remove,
	.init       = serial_class_init,
};
