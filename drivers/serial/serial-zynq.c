#include <string.h>
#include <stdlib.h>

#include <rtems/malloc.h>

#include <bsp/irq-generic.h>
#include <bsp/utility.h>
#include <bsp.h>

#include "common.h"
#include "dm.h"
#include "dm/device.h"
#include "dm/of_access.h"
#include "dm/serial.h"
#include "dm/read.h"
#include "dm/of_irq.h"

#define ZYNQ_UART_FIFO_DEPTH 64

struct zynq_uart_regs;
struct zynq_uart_platdata {
    volatile struct zynq_uart_regs *regs;
    int irq;
};

struct zynq_uart_regs {
    uint32_t control;
#define ZYNQ_UART_CONTROL_STPBRK BSP_BIT32(8)
#define ZYNQ_UART_CONTROL_STTBRK BSP_BIT32(7)
#define ZYNQ_UART_CONTROL_RSTTO BSP_BIT32(6)
#define ZYNQ_UART_CONTROL_TXDIS BSP_BIT32(5)
#define ZYNQ_UART_CONTROL_TXEN BSP_BIT32(4)
#define ZYNQ_UART_CONTROL_RXDIS BSP_BIT32(3)
#define ZYNQ_UART_CONTROL_RXEN BSP_BIT32(2)
#define ZYNQ_UART_CONTROL_TXRES BSP_BIT32(1)
#define ZYNQ_UART_CONTROL_RXRES BSP_BIT32(0)
    uint32_t mode;
#define ZYNQ_UART_MODE_CHMODE(val) BSP_FLD32(val, 8, 9)
#define ZYNQ_UART_MODE_CHMODE_GET(reg) BSP_FLD32GET(reg, 8, 9)
#define ZYNQ_UART_MODE_CHMODE_SET(reg, val) BSP_FLD32SET(reg, val, 8, 9)
#define ZYNQ_UART_MODE_CHMODE_NORMAL 0x00U
#define ZYNQ_UART_MODE_CHMODE_AUTO_ECHO 0x01U
#define ZYNQ_UART_MODE_CHMODE_LOCAL_LOOPBACK 0x02U
#define ZYNQ_UART_MODE_CHMODE_REMOTE_LOOPBACK 0x03U
#define ZYNQ_UART_MODE_NBSTOP(val) BSP_FLD32(val, 6, 7)
#define ZYNQ_UART_MODE_NBSTOP_GET(reg) BSP_FLD32GET(reg, 6, 7)
#define ZYNQ_UART_MODE_NBSTOP_SET(reg, val) BSP_FLD32SET(reg, val, 6, 7)
#define ZYNQ_UART_MODE_NBSTOP_STOP_1 0x00U
#define ZYNQ_UART_MODE_NBSTOP_STOP_1_5 0x01U
#define ZYNQ_UART_MODE_NBSTOP_STOP_2 0x02U
#define ZYNQ_UART_MODE_PAR(val) BSP_FLD32(val, 3, 5)
#define ZYNQ_UART_MODE_PAR_GET(reg) BSP_FLD32GET(reg, 3, 5)
#define ZYNQ_UART_MODE_PAR_SET(reg, val) BSP_FLD32SET(reg, val, 3, 5)
#define ZYNQ_UART_MODE_PAR_EVEN 0x00U
#define ZYNQ_UART_MODE_PAR_ODD 0x01U
#define ZYNQ_UART_MODE_PAR_SPACE 0x02U
#define ZYNQ_UART_MODE_PAR_MARK 0x03U
#define ZYNQ_UART_MODE_PAR_NONE 0x04U
#define ZYNQ_UART_MODE_CHRL(val) BSP_FLD32(val, 1, 2)
#define ZYNQ_UART_MODE_CHRL_GET(reg) BSP_FLD32GET(reg, 1, 2)
#define ZYNQ_UART_MODE_CHRL_SET(reg, val) BSP_FLD32SET(reg, val, 1, 2)
#define ZYNQ_UART_MODE_CHRL_8 0x00U
#define ZYNQ_UART_MODE_CHRL_7 0x02U
#define ZYNQ_UART_MODE_CHRL_6 0x03U
#define ZYNQ_UART_MODE_CLKS BSP_BIT32(0)
    uint32_t irq_en;
    uint32_t irq_dis;
    uint32_t irq_mask;
    uint32_t irq_sts;
#define ZYNQ_UART_TOVR BSP_BIT32(12)
#define ZYNQ_UART_TNFUL BSP_BIT32(11)
#define ZYNQ_UART_TTRIG BSP_BIT32(10)
#define ZYNQ_UART_DMSI BSP_BIT32(9)
#define ZYNQ_UART_TIMEOUT BSP_BIT32(8)
#define ZYNQ_UART_PARE BSP_BIT32(7)
#define ZYNQ_UART_FRAME BSP_BIT32(6)
#define ZYNQ_UART_ROVR BSP_BIT32(5)
#define ZYNQ_UART_TFUL BSP_BIT32(4)
#define ZYNQ_UART_TEMPTY BSP_BIT32(3)
#define ZYNQ_UART_RFUL BSP_BIT32(2)
#define ZYNQ_UART_REMPTY BSP_BIT32(1)
#define ZYNQ_UART_RTRIG BSP_BIT32(0)
    uint32_t baud_rate_gen;
#define ZYNQ_UART_BAUD_RATE_GEN_CD(val) BSP_FLD32(val, 0, 15)
#define ZYNQ_UART_BAUD_RATE_GEN_CD_GET(reg) BSP_FLD32GET(reg, 0, 15)
#define ZYNQ_UART_BAUD_RATE_GEN_CD_SET(reg, val) BSP_FLD32SET(reg, val, 0, 15)
    uint32_t rx_timeout;
#define ZYNQ_UART_RX_TIMEOUT_RTO(val) BSP_FLD32(val, 0, 7)
#define ZYNQ_UART_RX_TIMEOUT_RTO_GET(reg) BSP_FLD32GET(reg, 0, 7)
#define ZYNQ_UART_RX_TIMEOUT_RTO_SET(reg, val) BSP_FLD32SET(reg, val, 0, 7)
    uint32_t rx_fifo_trg_lvl;
#define ZYNQ_UART_RX_FIFO_TRG_LVL_RTRIG(val) BSP_FLD32(val, 0, 5)
#define ZYNQ_UART_RX_FIFO_TRG_LVL_RTRIG_GET(reg) BSP_FLD32GET(reg, 0, 5)
#define ZYNQ_UART_RX_FIFO_TRG_LVL_RTRIG_SET(reg, val) BSP_FLD32SET(reg, val, 0, 5)
    uint32_t modem_ctrl;
#define ZYNQ_UART_MODEM_CTRL_FCM BSP_BIT32(5)
#define ZYNQ_UART_MODEM_CTRL_RTS BSP_BIT32(1)
#define ZYNQ_UART_MODEM_CTRL_DTR BSP_BIT32(0)
    uint32_t modem_sts;
#define ZYNQ_UART_MODEM_STS_FCMS BSP_BIT32(8)
#define ZYNQ_UART_MODEM_STS_DCD BSP_BIT32(7)
#define ZYNQ_UART_MODEM_STS_RI BSP_BIT32(6)
#define ZYNQ_UART_MODEM_STS_DSR BSP_BIT32(5)
#define ZYNQ_UART_MODEM_STS_CTS BSP_BIT32(4)
#define ZYNQ_UART_MODEM_STS_DDCD BSP_BIT32(3)
#define ZYNQ_UART_MODEM_STS_TERI BSP_BIT32(2)
#define ZYNQ_UART_MODEM_STS_DDSR BSP_BIT32(1)
#define ZYNQ_UART_MODEM_STS_DCTS BSP_BIT32(0)
    uint32_t channel_sts;
#define ZYNQ_UART_CHANNEL_STS_TNFUL BSP_BIT32(14)
#define ZYNQ_UART_CHANNEL_STS_TTRIG BSP_BIT32(13)
#define ZYNQ_UART_CHANNEL_STS_FDELT BSP_BIT32(12)
#define ZYNQ_UART_CHANNEL_STS_TACTIVE BSP_BIT32(11)
#define ZYNQ_UART_CHANNEL_STS_RACTIVE BSP_BIT32(10)
#define ZYNQ_UART_CHANNEL_STS_TFUL BSP_BIT32(4)
#define ZYNQ_UART_CHANNEL_STS_TEMPTY BSP_BIT32(3)
#define ZYNQ_UART_CHANNEL_STS_RFUL BSP_BIT32(2)
#define ZYNQ_UART_CHANNEL_STS_REMPTY BSP_BIT32(1)
#define ZYNQ_UART_CHANNEL_STS_RTRIG BSP_BIT32(0)
    uint32_t tx_rx_fifo;
#define ZYNQ_UART_TX_RX_FIFO_FIFO(val) BSP_FLD32(val, 0, 7)
#define ZYNQ_UART_TX_RX_FIFO_FIFO_GET(reg) BSP_FLD32GET(reg, 0, 7)
#define ZYNQ_UART_TX_RX_FIFO_FIFO_SET(reg, val) BSP_FLD32SET(reg, val, 0, 7)
    uint32_t baud_rate_div;
#define ZYNQ_UART_BAUD_RATE_DIV_BDIV(val) BSP_FLD32(val, 0, 7)
#define ZYNQ_UART_BAUD_RATE_DIV_BDIV_GET(reg) BSP_FLD32GET(reg, 0, 7)
#define ZYNQ_UART_BAUD_RATE_DIV_BDIV_SET(reg, val) BSP_FLD32SET(reg, val, 0, 7)
    uint32_t flow_delay;
#define ZYNQ_UART_FLOW_DELAY_FDEL(val) BSP_FLD32(val, 0, 5)
#define ZYNQ_UART_FLOW_DELAY_FDEL_GET(reg) BSP_FLD32GET(reg, 0, 5)
#define ZYNQ_UART_FLOW_DELAY_FDEL_SET(reg, val) BSP_FLD32SET(reg, val, 0, 5)
    uint32_t reserved_3c[2];
    uint32_t tx_fifo_trg_lvl;
#define ZYNQ_UART_TX_FIFO_TRG_LVL_TTRIG(val) BSP_FLD32(val, 0, 5)
#define ZYNQ_UART_TX_FIFO_TRG_LVL_TTRIG_GET(reg) BSP_FLD32GET(reg, 0, 5)
#define ZYNQ_UART_TX_FIFO_TRG_LVL_TTRIG_SET(reg, val) BSP_FLD32SET(reg, val, 0, 5)
};


static int zynq_uart_calc_baudrate(uint32_t baudrate, uint32_t *brgr,
    uint32_t *bauddiv, uint32_t  modereg)                   
{
    uint32_t inputclk = ZYNQ_CLOCK_UART;
    uint32_t best_error = 0xFFFFFFFF;
    uint32_t brgr_value;    /* Calculated value for baud rate generator */
    uint32_t calcbaudrate;  /* Calculated baud rate */
    uint32_t bauderror;     /* Diff between calculated and requested baud rate */
    uint32_t percenterror;
    uint32_t bdiv;
    
    /*
    * Make sure the baud rate is not impossilby large.
    * Fastest possible baud rate is Input Clock / 2.
    */
    if ((baudrate * 2) > inputclk) 
        return -EINVAL;

    /*
    * Check whether the input clock is divided by 8
    */
    if(modereg & ZYNQ_UART_MODE_CLKS)
        inputclk = inputclk / 8;

    /*
    * Determine the Baud divider. It can be 4to 254.
    * Loop through all possible combinations
    */
    for (bdiv = 4; bdiv < 255; bdiv++) {
        /*
        * Calculate the value for BRGR register
        */
        brgr_value = inputclk / (baudrate * (bdiv + 1));

        /*
        * Calculate the baud rate from the BRGR value
        */
        calcbaudrate = inputclk / (brgr_value * (bdiv + 1));

        /*
        * Avoid unsigned integer underflow
        */
        if (baudrate > calcbaudrate) 
            bauderror = baudrate - calcbaudrate;
        else
            bauderror = calcbaudrate - baudrate;

        /*
        * Find the calculated baud rate closest to requested baud rate.
        */
        if (best_error > bauderror) {
            *brgr = brgr_value;
            *bauddiv = bdiv;
            best_error = bauderror;
        }
    }

    /*
    * Make sure the best error is not too large.
    */
    percenterror = (best_error * 100) / baudrate;
    #define XUARTPS_MAX_BAUD_ERROR_RATE		 3	/* max % error allowed */
    if (XUARTPS_MAX_BAUD_ERROR_RATE < percenterror) 
        return -EINVAL;
    return 0;
}

static void zynq_uart_isr(void *arg)
{
    struct udevice *dev = (struct udevice *)arg;
    struct zynq_uart_platdata *platdata = dev_get_plat(dev);
    volatile struct zynq_uart_regs *regs = platdata->regs;
    char buffer[ZYNQ_UART_FIFO_DEPTH];
    uint32_t channel_sts;

    if ((regs->irq_sts & (ZYNQ_UART_TIMEOUT | ZYNQ_UART_RTRIG)) != 0) {
        int rxlen = 0;
        regs->irq_sts = ZYNQ_UART_TIMEOUT | ZYNQ_UART_RTRIG;
        do {
            buffer[rxlen++] = (char)ZYNQ_UART_TX_RX_FIFO_FIFO_GET(regs->tx_rx_fifo);
            channel_sts = regs->channel_sts;
        } while ((channel_sts & ZYNQ_UART_CHANNEL_STS_REMPTY) == 0);
        serial_class_enqueue(dev, buffer, rxlen);
    } else {
        channel_sts = regs->channel_sts;
    }
    serial_class_dequeue(dev);
}

static int zynq_uart_open(struct udevice *dev)
{
    struct zynq_uart_platdata *platdata = dev_get_plat(dev);
    volatile struct zynq_uart_regs *regs = platdata->regs;
    uint32_t brgr = 0x3e;
    uint32_t bauddiv = 0x6;

    zynq_uart_calc_baudrate(ZYNQ_UART_DEFAULT_BAUD, &brgr, &bauddiv, regs->mode);
    regs->control &= ~(ZYNQ_UART_CONTROL_RXEN | ZYNQ_UART_CONTROL_TXEN);
    regs->control = ZYNQ_UART_CONTROL_RXDIS
                  | ZYNQ_UART_CONTROL_TXDIS;
    regs->mode = ZYNQ_UART_MODE_CHMODE(ZYNQ_UART_MODE_CHMODE_NORMAL)
                | ZYNQ_UART_MODE_PAR(ZYNQ_UART_MODE_PAR_NONE)
                | ZYNQ_UART_MODE_CHRL(ZYNQ_UART_MODE_CHRL_8);
    regs->baud_rate_gen = ZYNQ_UART_BAUD_RATE_GEN_CD(brgr);
    regs->baud_rate_div = ZYNQ_UART_BAUD_RATE_DIV_BDIV(bauddiv);

    /* A Tx/Rx logic reset must be issued after baud rate manipulation */
    regs->control = ZYNQ_UART_CONTROL_RXDIS
                  | ZYNQ_UART_CONTROL_TXDIS
                  | ZYNQ_UART_CONTROL_RXRES
                  | ZYNQ_UART_CONTROL_TXRES;
    regs->rx_timeout = 32;
    regs->rx_fifo_trg_lvl = ZYNQ_UART_FIFO_DEPTH / 2;
    regs->irq_dis = 0xffffffff;
    regs->irq_sts = 0xffffffff;
    regs->irq_en = ZYNQ_UART_RTRIG | ZYNQ_UART_TIMEOUT;
    regs->control = ZYNQ_UART_CONTROL_RXEN
                  | ZYNQ_UART_CONTROL_TXEN
                  | ZYNQ_UART_CONTROL_RSTTO;
    return 0;
}

static int zynq_uart_release(struct udevice *dev)
{
    struct zynq_uart_platdata *platdata = dev_get_plat(dev);
    volatile struct zynq_uart_regs *regs = platdata->regs;

    regs->irq_dis = 0xffffffff;
    regs->irq_sts = 0xffffffff;
    regs->control = 0;
    return 0;
}

static int zynq_uart_tx_empty(struct udevice *dev)
{
    struct zynq_uart_platdata *platdata = dev_get_plat(dev);

    return platdata->regs->channel_sts & ZYNQ_UART_CHANNEL_STS_TEMPTY;
}

static ssize_t zynq_uart_fifo_write(struct udevice *dev, 
    const char *buf, size_t size)
{
    struct zynq_uart_platdata *platdata = dev_get_plat(dev);
    volatile struct zynq_uart_regs *regs = platdata->regs;
    size_t out = 0;

    while (out < size) {
        if (regs->channel_sts & ZYNQ_UART_CHANNEL_STS_TFUL)
            break;
        regs->tx_rx_fifo = buf[out++];
    }
    return out;
}

static void zynq_uart_tx_irqena(struct udevice *dev)
{
    struct zynq_uart_platdata *platdata = dev_get_plat(dev);

    platdata->regs->irq_sts = ZYNQ_UART_TEMPTY;
    platdata->regs->irq_en = ZYNQ_UART_TEMPTY;
}

static void zynq_uart_tx_irqdis(struct udevice *dev)
{
    struct zynq_uart_platdata *platdata = dev_get_plat(dev);

    platdata->regs->irq_dis = ZYNQ_UART_TEMPTY;
}

static int zynq_uart_set_termios(struct udevice *dev, const struct termios *term)
{
    struct zynq_uart_platdata *platdata = dev_get_plat(dev);
    volatile struct zynq_uart_regs *regs = platdata->regs;
    uint32_t brgr = 0;
    uint32_t bauddiv = 0;
    uint32_t mode = 0;
    int32_t baud;
    int rc;

    /*
    * Determine the baud rate
    */
    baud = rtems_termios_baud_to_number(term->c_ospeed);
    if (baud > 0) {
        rc = zynq_uart_calc_baudrate(baud, &brgr, &bauddiv, regs->mode);
        if (rc != 0)
            return rc;
    }

    /*
    * Configure the mode register
    */
    mode |= ZYNQ_UART_MODE_CHMODE(ZYNQ_UART_MODE_CHMODE_NORMAL);

    /*
    * Parity
    */
    mode |= ZYNQ_UART_MODE_PAR(ZYNQ_UART_MODE_PAR_NONE);
    if (term->c_cflag & PARENB) {
        if (!(term->c_cflag & PARODD)) 
            mode |= ZYNQ_UART_MODE_PAR(ZYNQ_UART_MODE_PAR_ODD);
        else
            mode |= ZYNQ_UART_MODE_PAR(ZYNQ_UART_MODE_PAR_EVEN);
    }

    /*
    * Character Size
    */
    switch (term->c_cflag & CSIZE) {
    case CS5:
        return -ENOTSUP;
    case CS6:
        mode |= ZYNQ_UART_MODE_CHRL(ZYNQ_UART_MODE_CHRL_6);
        break;
    case CS7:
        mode |= ZYNQ_UART_MODE_CHRL(ZYNQ_UART_MODE_CHRL_7);
        break;
    case CS8:
    default:
        mode |= ZYNQ_UART_MODE_CHRL(ZYNQ_UART_MODE_CHRL_8);
        break;
    }

    /*
    * Stop Bits
    */
    if (term->c_cflag & CSTOPB)
        mode |= ZYNQ_UART_MODE_NBSTOP(ZYNQ_UART_MODE_NBSTOP_STOP_2);
    else
        mode |= ZYNQ_UART_MODE_NBSTOP(ZYNQ_UART_MODE_NBSTOP_STOP_1);
    regs->control &= ~(ZYNQ_UART_CONTROL_RXEN | ZYNQ_UART_CONTROL_TXEN);
    regs->mode = mode;
    if (baud > 0) {
        regs->baud_rate_gen = ZYNQ_UART_BAUD_RATE_GEN_CD(brgr);
        regs->baud_rate_div = ZYNQ_UART_BAUD_RATE_DIV_BDIV(bauddiv);
    }
    regs->control |= ZYNQ_UART_CONTROL_RXEN | ZYNQ_UART_CONTROL_TXEN;
    return 0;
}

static int zynq_uart_putc_poll(struct udevice *dev, const char ch)
{
    struct zynq_uart_platdata *platdata = dev_get_plat(dev);
    volatile struct zynq_uart_regs *regs = platdata->regs;

    while (regs->channel_sts & ZYNQ_UART_CHANNEL_STS_TFUL);
    regs->tx_rx_fifo = ch;
    return 0;
}

static int zynq_uart_of_to_platdata(struct udevice *dev)
{
    struct zynq_uart_platdata *platdata = dev_get_plat(dev);
    fdt_addr_t addr, irq;

    addr = dev_read_addr_index(dev, 0);
    if (addr == FDT_ADDR_T_NONE)
        return -EINVAL;
    platdata->regs = (volatile struct zynq_uart_regs *)addr;
    irq = dev_read_irq_index(dev, 0, NULL);
    if (irq == FDT_ADDR_T_NONE)
        return -EINVAL;
    platdata->irq = irq;
    return 0;
}

static int zynq_uart_probe(struct udevice *dev)
{
    struct zynq_uart_platdata *platdata = dev_get_plat(dev);
    rtems_status_code sc;

    sc = rtems_interrupt_handler_install(platdata->irq, "UART",
        RTEMS_INTERRUPT_SHARED, zynq_uart_isr, dev);
    if (sc != RTEMS_SUCCESSFUL) {
        printk("Error(%s): Install UART interrupt failed:%s\n", __func__, 
            rtems_status_text(sc));
        return rtems_status_code_to_errno(sc);
    }
    return 0;
}

static int zynq_uart_remove(struct udevice *dev)
{
    struct zynq_uart_platdata *platdata = dev_get_plat(dev);

    rtems_interrupt_handler_remove(platdata->irq, zynq_uart_isr, dev);
    return 0;
}

static const struct udevice_id zynq_uart_ids[] = {
    {.compatible = "xlnx,xuartps"},
    {NULL}
};

static const struct dm_serial_ops zynq_serial_ops = {
    .open          = zynq_uart_open,
    .release       = zynq_uart_release,
    .tx_empty      = zynq_uart_tx_empty,
    .fill_fifo     = zynq_uart_fifo_write,
    .tx_irqena     = zynq_uart_tx_irqena,
    .tx_irqdis     = zynq_uart_tx_irqdis,
    .set_termios   = zynq_uart_set_termios,
#ifdef CONFIG_FLOWCTRL
    .set_mctrl     = zynq_uart_set_mctrl,
#endif
    .putc_poll     = zynq_uart_putc_poll
};

DM_DRIVER(zynq) = {
    .name       = "zynq_uart",
    .id         = UCLASS_SERIAL,
    .of_match   = of_match_ptr(zynq_uart_ids),
    .probe      = zynq_uart_probe,
    .remove     = of_match_ptr(zynq_uart_remove),
    .of_to_plat = zynq_uart_of_to_platdata,
    .plat_auto  = sizeof(struct zynq_uart_platdata),
    .ops        = &zynq_serial_ops,
};