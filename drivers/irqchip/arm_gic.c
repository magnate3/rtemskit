#include <errno.h>

#include "dm/of.h"
#include "dm/of_irq.h"
#include "dm/device.h"

#include "dt-bindings/interrupt-controller/irq.h"

static int of_gic_irq_translate(const struct of_phandle_args *ofirq,
    unsigned int *hwirq, unsigned int *type)    
{
	if (ofirq->args_count == 1 && ofirq->args[0] < 16) {
		*hwirq = ofirq->args[0];
		*type = IRQ_TYPE_EDGE_RISING;
		return 0;
	}
	if (ofirq->np) {
		if (ofirq->args_count < 3)
			return -EINVAL;
		switch (ofirq->args[0]) {
		case 0:			/* SPI */
			*hwirq = ofirq->args[1] + 32;
			break;
		case 1:			/* PPI */
			*hwirq = ofirq->args[1] + 16;
			break;
		default:
			return -EINVAL;
		}
		*type = ofirq->args[2] & 0xF;
		return 0;
	}
	return -EINVAL;
}

static const struct udevice_id ids[] = {
    {.compatible = "arm,cortex-a9-gic"},
    {.compatible = "ti,omap4-wugen-mpu"},
    {NULL, }
};

OF_IRQ(gic) = {
    .match_ids = ids,
    .translate = of_gic_irq_translate
};