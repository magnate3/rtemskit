#include <string.h>
#include <stdlib.h>

#include <rtems/malloc.h>
#include <bsp/irq-generic.h>

#include "common.h"
#include "dm.h"
#include "dm/device.h"
#include "dm/of_access.h"
#include "dm/serial.h"
#include "dm/read.h"
#include "dm/of_irq.h"
#include "asm/io.h"


struct ns16550_info {
    uint32_t fifo_size;
    uint32_t reg_shift;
    uint32_t clock;
};

struct ns16550_platdata {
    uintptr_t port;
    int irq;
    uint32_t clock;
    bool has_fractional_divider_register;
    bool has_precision_clock_synthesizer;
    uint8_t modem_control;
    uint8_t line_control;
    uint32_t baud_divisor;
    const struct ns16550_info *info;
};

#define NS16550_DEFAULT_BDR 115200
#define SP_FIFO_SIZE 16

/*
 * Register list
 */
#define _REGOFS(N) ((N) << _reg_shift)

#define NS16550_RECEIVE_BUFFER   _REGOFS(0)
#define NS16550_TRANSMIT_BUFFER  _REGOFS(0)
#define NS16550_DIVISOR_LATCH_L  _REGOFS(0)
#define NS16550_INTERRUPT_ENABLE _REGOFS(1)
#define NS16550_DIVISOR_LATCH_M  _REGOFS(1)
#define NS16550_INTERRUPT_ID     _REGOFS(2)
#define NS16550_FIFO_CONTROL     _REGOFS(2)
#define NS16550_LINE_CONTROL     _REGOFS(3)
#define NS16550_MODEM_CONTROL    _REGOFS(4)
#define NS16550_LINE_STATUS      _REGOFS(5)
#define NS16550_MODEM_STATUS     _REGOFS(6)
#define NS16550_SCRATCH_PAD      _REGOFS(7)
#define NS16550_FRACTIONAL_DIVIDER _REGOFS(10)

/*
 * Define serial port interrupt enable register structure.
 */
#define SP_INT_RX_ENABLE  0x01
#define SP_INT_TX_ENABLE  0x02
#define SP_INT_LS_ENABLE  0x04
#define SP_INT_MS_ENABLE  0x08

#define NS16550_ENABLE_ALL_INTR           (SP_INT_RX_ENABLE | SP_INT_TX_ENABLE)
#define NS16550_DISABLE_ALL_INTR          0x00
#define NS16550_ENABLE_ALL_INTR_EXCEPT_TX (SP_INT_RX_ENABLE)

/*
 * Define serial port interrupt ID register structure.
 */
#define SP_IID_0 0x01
#define SP_IID_1 0x02
#define SP_IID_2 0x04
#define SP_IID_3 0x08

/*
 * Define serial port fifo control register structure.
 */
#define SP_FIFO_ENABLE  0x01
#define SP_FIFO_RXRST 0x02
#define SP_FIFO_TXRST 0x04
#define SP_FIFO_DMA   0x08
#define SP_FIFO_RXLEVEL 0xc0
//#define SP_FIFO_SIZE(platdata) (platdata->info->fifo_size)


/*
 * Define serial port line control register structure.
 */
#define SP_LINE_SIZE  0x03
#define SP_LINE_STOP  0x04
#define SP_LINE_PAR   0x08
#define SP_LINE_ODD   0x10
#define SP_LINE_STICK 0x20
#define SP_LINE_BREAK 0x40
#define SP_LINE_DLAB  0x80

/*
 * Line status register character size definitions.
 */
#define FIVE_BITS 0x0                   /* five bits per character */
#define SIX_BITS 0x1                    /* six bits per character */
#define SEVEN_BITS 0x2                  /* seven bits per character */
#define EIGHT_BITS 0x3                  /* eight bits per character */

/*
 * Define serial port modem control register structure.
 */
#define SP_MODEM_DTR  0x01
#define SP_MODEM_RTS  0x02
#define SP_MODEM_IRQ  0x08
#define SP_MODEM_LOOP 0x10
#define SP_MODEM_DIV4 0x80

/*
 * Define serial port line status register structure.
 */
#define SP_LSR_RDY    0x01
#define SP_LSR_EOVRUN 0x02
#define SP_LSR_EPAR   0x04
#define SP_LSR_EFRAME 0x08
#define SP_LSR_BREAK  0x10
#define SP_LSR_THOLD  0x20
#define SP_LSR_TX   0x40
#define SP_LSR_EFIFO  0x80

static int _reg_shift;

static uint32_t ns16550_baud_divisor_get(struct ns16550_platdata *platdata, 
    uint32_t baud)
{
    uint32_t clock;
    uint32_t baudDivisor;
    uint32_t err;
    uint32_t actual;
    uint32_t newErr;

    if (platdata->clock) 
        clock = platdata->clock;
    else
        clock = 115200;
    baudDivisor = clock / (baud * 16);
    if (platdata->has_precision_clock_synthesizer) {
        uint32_t i;
        err = baud;
        baudDivisor = 0x0001ffff;
        for (i = 2; i <= 0x10000; i *= 2) {
            uint32_t fout;
            uint32_t fin;

            fin = i - 1;
            fout = (baud * fin * 16) / clock;
            actual = (clock * fout) / (16 * fin);
            newErr = actual > baud ? actual - baud : baud - actual;
            if (newErr < err) {
                err = newErr;
                baudDivisor = fin | (fout << 16);
            }
        }
    } else if (platdata->has_fractional_divider_register) {
        uint32_t fractionalDivider = 0x10;
        uint32_t mulVal;
        uint32_t divAddVal;

        err = baud;
        clock /= 16 * baudDivisor;
        for (mulVal = 1; mulVal < 16; ++mulVal) {
            for (divAddVal = 0; divAddVal < mulVal; ++divAddVal) {
                actual = (mulVal * clock) / (mulVal + divAddVal);
                newErr = actual > baud ? actual - baud : baud - actual;
                if (newErr < err) {
                    err = newErr;
                    fractionalDivider = (mulVal << 4) | divAddVal;
                }
            }
        }
        writeb(fractionalDivider, platdata->port + NS16550_FRACTIONAL_DIVIDER);
    } 
    return baudDivisor;
}

static void ns16550_isr(void *arg)
{
    struct udevice *dev = (struct udevice *)arg;
    struct ns16550_platdata *platdata = dev_get_plat(dev);
    char buf [SP_FIFO_SIZE];
    int i;

    do {
        for (i = 0; i < SP_FIFO_SIZE; ++i) {
            if (readb(platdata->port + NS16550_LINE_STATUS) & SP_LSR_RDY)
                buf[i] = readb(platdata->port + NS16550_RECEIVE_BUFFER);
            else
                break;
        }
        serial_class_enqueue(dev, buf, i);
        serial_class_dequeue(dev);
    } while (!(readb(platdata->port + NS16550_INTERRUPT_ID) & SP_IID_0));
}

static int ns16550_open(struct udevice *dev)
{
    struct ns16550_platdata *platdata = dev_get_plat(dev);

    writeb(NS16550_ENABLE_ALL_INTR_EXCEPT_TX, 
        platdata->port + NS16550_INTERRUPT_ENABLE);
    return 0;
}

static int ns16550_release(struct udevice *dev)
{
    struct ns16550_platdata *platdata = dev_get_plat(dev);
    
    writeb(NS16550_DISABLE_ALL_INTR, 
        platdata->port + NS16550_INTERRUPT_ENABLE);
    return 0;
}

static int ns16550_tx_empty(struct udevice *dev)
{
    struct ns16550_platdata *platdata = dev_get_plat(dev);
    int status = readb(platdata->port + NS16550_LINE_STATUS);
    return status & SP_LSR_THOLD;
}

static ssize_t ns16550_fifo_write(struct udevice *dev, 
    const char *buf, size_t size)
{
    struct ns16550_platdata *platdata = dev_get_plat(dev);
    size_t out = size > SP_FIFO_SIZE? SP_FIFO_SIZE: size;

    for (size_t i = 0; i < out; ++i)
        writeb(buf[i], platdata->port + NS16550_TRANSMIT_BUFFER);
    return out;
}

static void ns16550_tx_irqena(struct udevice *dev)
{
    struct ns16550_platdata *platdata = dev_get_plat(dev);

    writeb(SP_INT_TX_ENABLE, platdata->port + NS16550_INTERRUPT_ENABLE);
}

static void ns16550_tx_irqdis(struct udevice *dev)
{
    struct ns16550_platdata *platdata = dev_get_plat(dev);

    writeb(NS16550_ENABLE_ALL_INTR_EXCEPT_TX, 
        platdata->port + NS16550_INTERRUPT_ENABLE);
}

static int ns16550_set_termios(struct udevice *dev, const struct termios *t)
{
    struct ns16550_platdata *platdata = dev_get_plat(dev);
    uint32_t ulBaudDivisor;
    uint8_t ucLineControl;
    uint32_t baud_requested;

    /*
    *  Calculate the baud rate divisor
    *  Assert ensures there is no division by 0.
    */
    baud_requested = rtems_termios_baud_to_number(t->c_ospeed);
    _Assert( baud_requested != 0 );
    ulBaudDivisor = ns16550_baud_divisor_get(platdata, baud_requested);
    ucLineControl = 0;

    /* Parity */
    if (t->c_cflag & PARENB) {
        ucLineControl |= SP_LINE_PAR;
        if (!(t->c_cflag & PARODD))
            ucLineControl |= SP_LINE_ODD;
    }
    /*  Character Size */
    if (t->c_cflag & CSIZE) {
        switch (t->c_cflag & CSIZE) {
        case CS5:
            ucLineControl |= FIVE_BITS;
            break;
        case CS6:
            ucLineControl |= SIX_BITS;
            break;
        case CS7:
            ucLineControl |= SEVEN_BITS;
            break;
        case CS8:
            ucLineControl |= EIGHT_BITS;
            break;
        }
    } else {
        ucLineControl |= EIGHT_BITS;
    }

    /* Stop Bits */
    if (t->c_cflag & CSTOPB) 
        ucLineControl |= SP_LINE_STOP; /* 2 stop bits */

    /* Now actually set the chip */
    if (ulBaudDivisor != platdata->baud_divisor || ucLineControl != platdata->line_control) {
        platdata->baud_divisor = ulBaudDivisor;
        platdata->line_control = ucLineControl;

        /*
        *  Set the baud rate
        *
        *  NOTE: When the Divisor Latch Access Bit (DLAB) is set to 1,
        *        the transmit buffer and interrupt enable registers
        *        turn into the LSB and MSB divisor latch registers.
        */
        writeb(SP_LINE_DLAB, platdata->port + NS16550_LINE_CONTROL);
        writeb(ulBaudDivisor & 0xff, platdata->port + NS16550_DIVISOR_LATCH_L);
        writeb((ulBaudDivisor >> 8) & 0xff, platdata->port + NS16550_DIVISOR_LATCH_M);

        /* Now write the line control */
        if (platdata->has_precision_clock_synthesizer) {
            writeb((uint8_t)(ulBaudDivisor >> 24), platdata->port + NS16550_SCRATCH_PAD);
            writeb(ucLineControl, platdata->port + NS16550_LINE_CONTROL);
            writeb((uint8_t)(ulBaudDivisor >> 16), platdata->port + NS16550_SCRATCH_PAD);
        } else {
            writeb(ucLineControl, platdata->port + NS16550_LINE_CONTROL);
        }
    }
    return 0;
}

static int ns16550_putc_poll(struct udevice *dev, const char ch)
{
    struct ns16550_platdata *platdata = dev_get_plat(dev);
    uint32_t status;

    do {
        status = readb(platdata->port + NS16550_LINE_STATUS);
    } while (!(status & SP_LSR_THOLD));
    writeb(ch, platdata->port + NS16550_TRANSMIT_BUFFER);
    return 0;
}

static int ns16550_probe(struct udevice *dev)
{
    struct ns16550_platdata *platdata = dev_get_plat(dev);
    rtems_status_code sc;
    uint8_t  ucDataByte;
    uint32_t ulBaudDivisor;
    

    platdata->modem_control = SP_MODEM_IRQ;

    /* Clear the divisor latch, clear all interrupt enables,
    * and reset and
    * disable the FIFO's.
    */
    writeb(0, platdata->port + NS16550_LINE_CONTROL);
    writeb(NS16550_DISABLE_ALL_INTR, platdata->port + NS16550_INTERRUPT_ENABLE);

    /* Set the divisor latch and set the baud rate. */
    ulBaudDivisor = ns16550_baud_divisor_get(platdata, NS16550_DEFAULT_BDR);
    platdata->baud_divisor = ulBaudDivisor;
    ucDataByte = SP_LINE_DLAB;
    writeb(ucDataByte, platdata->port + NS16550_LINE_CONTROL);

    /* XXX */
    writeb((uint8_t)(ulBaudDivisor & 0xffU), platdata->port + NS16550_DIVISOR_LATCH_L);
    writeb((uint8_t)( ulBaudDivisor >> 8 ) & 0xffU, platdata->port + NS16550_DIVISOR_LATCH_M);

    /* Clear the divisor latch and set the character size to eight bits */
    /* with one stop bit and no parity checking. */
    ucDataByte = EIGHT_BITS;
    platdata->line_control = ucDataByte;
    if (platdata->has_precision_clock_synthesizer) {
        uint8_t fcr;

        /*
        * Enable precision clock synthesizer.  This must be done with DLAB == 1 in
        * the line control register.
        */
        fcr = readb(platdata->port + NS16550_FIFO_CONTROL);
        fcr |= 0x10;
        writeb(fcr, platdata->port + NS16550_FIFO_CONTROL);

        writeb((uint8_t)(ulBaudDivisor >> 24), platdata->port + NS16550_SCRATCH_PAD);
        writeb(ucDataByte, platdata->port + NS16550_LINE_CONTROL);
        writeb((uint8_t)(ulBaudDivisor >> 16), platdata->port + NS16550_SCRATCH_PAD);
    } else {
        writeb(ucDataByte, platdata->port + NS16550_LINE_CONTROL);
    }

    /* Enable and reset transmit and receive FIFOs. TJA     */
    ucDataByte = SP_FIFO_ENABLE;
    writeb(ucDataByte, platdata->port + NS16550_FIFO_CONTROL);

    ucDataByte = SP_FIFO_ENABLE | SP_FIFO_RXRST | SP_FIFO_TXRST;
    writeb(ucDataByte, platdata->port + NS16550_FIFO_CONTROL);
    writeb(NS16550_DISABLE_ALL_INTR, platdata->port + NS16550_INTERRUPT_ENABLE);
    sc = rtems_interrupt_handler_install(platdata->irq, "NS16550",
        RTEMS_INTERRUPT_UNIQUE, ns16550_isr, dev);
    if (sc != RTEMS_SUCCESSFUL) {
        printk( "%s: Error: Install interrupt handler\n", __func__);
        return rtems_status_code_to_errno(sc);
    }

    /* Set data terminal ready. */
    /* And open interrupt tristate line */
    writeb(platdata->modem_control, platdata->port + NS16550_MODEM_CONTROL);
    readb(platdata->port + NS16550_LINE_STATUS);
    readb(platdata->port + NS16550_RECEIVE_BUFFER);
    return 0;
}

static int ns16550_remove(struct udevice *dev)
{
    struct ns16550_platdata *platdata = dev_get_plat(dev);
    rtems_status_code sc;
    
    writeb(NS16550_DISABLE_ALL_INTR, platdata->port + NS16550_INTERRUPT_ENABLE);
    sc = rtems_interrupt_handler_remove(platdata->irq, ns16550_isr, dev);
    if (sc != RTEMS_SUCCESSFUL) {
        printk("%s: Error: Remove interrupt handler\n", __func__);
        return rtems_status_code_to_errno(sc);
    }
    return 0;
}

#ifdef CONFIG_FLOWCTRL
static void ns16550_set_mctrl(struct udevice *dev, unsigned int mctrl)
{
    struct ns16550_platdata *platdata = dev_get_plat(dev);

    if (mctrl & TIOCM_RTS) {
        platdata->modem_control |= SP_MODEM_DTR;
        writeb(platdata->modem_control, platdata->port + NS16550_MODEM_CONTROL);
    } else {
        platdata->modem_control &= ~SP_MODEM_DTR;
        writeb(platdata->modem_control, platdata->port + NS16550_MODEM_CONTROL);
    }
}
#endif

static int ns16550_of_to_platdata(struct udevice *dev)
{
    struct ns16550_platdata *platdata = dev_get_plat(dev);
    const struct ns16550_info *info = (void *)dev_get_driver_data(dev);
    fdt_addr_t addr, irq;
    
    addr = dev_read_addr_index(dev, 0);
    if (addr == FDT_ADDR_T_NONE)
        return -EINVAL;
    platdata->port = addr;
    irq = dev_read_irq_index(dev, 0, NULL);
    if (irq == FDT_ADDR_T_NONE)
        return -EINVAL;
    platdata->irq = irq;
    platdata->clock = info->clock;
    _reg_shift = info->reg_shift;
    return 0;
}

static struct ns16550_info ti_am437x_info = {
    .fifo_size = 64,
    .reg_shift = 2,
    .clock = 48000000
};

static const struct udevice_id ns16550_ids[] = {
    {.compatible = "ti,am4372-uart", .data = (ulong)&ti_am437x_info},
    {NULL}
};

static const struct dm_serial_ops ns16550_ops = {
    .open          = ns16550_open,
    .release       = ns16550_release,
    .tx_empty      = ns16550_tx_empty,
    .fill_fifo     = ns16550_fifo_write,
    .tx_irqena     = ns16550_tx_irqena,
    .tx_irqdis     = ns16550_tx_irqdis,
    .set_termios   = ns16550_set_termios,
#ifdef CONFIG_FLOWCTRL
    .set_mctrl     = ns16550_set_mctrl,
#endif
    .putc_poll     = ns16550_putc_poll
};

DM_DRIVER(ns16550) = {
    .name       = "ns16550",
    .id         = UCLASS_SERIAL,
    .of_match   = of_match_ptr(ns16550_ids),
    .probe      = ns16550_probe,
    .remove     = of_match_ptr(ns16550_remove),
    .of_to_plat = ns16550_of_to_platdata,
    .plat_auto  = sizeof(struct ns16550_platdata),
    .ops        = &ns16550_ops,
};
