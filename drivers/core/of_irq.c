// SPDX-License-Identifier: GPL-2.0+
/*
 *  Derived from arch/i386/kernel/irq.c
 *    Copyright (C) 1992 Linus Torvalds
 *  Adapted from arch/i386 by Gary Thomas
 *    Copyright (C) 1995-1996 Gary Thomas (gdt@linuxppc.org)
 *  Updated and modified by Cort Dougan <cort@fsmlabs.com>
 *    Copyright (C) 1996-2001 Cort Dougan
 *  Adapted for Power Macintosh by Paul Mackerras
 *    Copyright (C) 1996 Paul Mackerras (paulus@cs.anu.edu.au)
 *
 * This file contains the code used to make IRQ descriptions in the
 * device tree to actual irq numbers on an interrupt controller
 * driver.
 */

#define pr_fmt(fmt)	"OF: " fmt

#include <errno.h>

#include "common.h"
#include "log.h"
#include "linux/compat.h"
#include "linux/bug.h"
#include "linux/libfdt.h"
#include "dm/read.h"
#include "dm/device.h"
#include "dm/of_access.h"
#include "dm/of.h"
#include "dm/of_irq.h"

#if !defined(__linux__)
#define OF_IMAP_NO_PHANDLE              (0)
#define OF_IMAP_OLDWORLD_MAC            (0)
#define of_irq_workarounds              (0)
#define of_irq_dflt_pic                 (0)
#define of_irq_parse_oldworld(...) (0)
#endif


static int of_default_irq_translate(const struct of_phandle_args *ofirq,
    unsigned int *hwirq, unsigned int *type)    
{
    if (ofirq->args_count == 1) {
        *hwirq = ofirq->args[0];
        *type = 0;
        return 0;
    }
    if (ofirq->args_count == 2) {
        *hwirq = ofirq->args[0];
        *type = ofirq->args[2];
        return 0;
    }
    printk("%s: Invalid interrupt controller\n", __func__);
    return -EINVAL;
}

static bool of_irq_is_compatible(const struct device_node *np,
    const  struct udevice_id *match_table)
{
    while (match_table->compatible) {
        if (of_device_is_compatible(np, match_table->compatible, NULL, NULL))
            return true;
        match_table++;
    }
    return false;
}

static int of_irq_translate(const struct of_phandle_args *ofirq,
    unsigned int *irq, unsigned int *type)
{
	const struct of_irq_ops *_end = OF_IRQ_END();
	const struct of_irq_ops *ops;

	if (!ofirq->np)
		return -EINVAL;
    
	for (ops = OF_IRQ_START(); ops < _end; ops++) {
        if (of_irq_is_compatible(ofirq->np, ops->match_ids))
            return ops->translate(ofirq, irq, type);
    }
    return of_default_irq_translate(ofirq, irq, type);
}

static struct device_node *of_irq_find_parent(struct device_node *child)
{
	struct device_node *p;
	phandle parent;

	if (!of_node_get(child))
		return NULL;
	do {
		if (of_read_u32(child, "interrupt-parent", &parent)) {
			p = of_get_parent(child);
		} else	{
			if (of_irq_workarounds & OF_IMAP_NO_PHANDLE)
				p = of_node_get(of_irq_dflt_pic);
			else
				p = of_find_node_by_phandle(parent);
		}
		of_node_put(child);
		child = p;
	} while (p && of_get_property(p, "#interrupt-cells", NULL) == NULL);

	return p;
}

static int of_irq_parse_raw(const __be32 *addr, struct of_phandle_args *out_irq)
{
	struct device_node *ipar, *tnode, *old = NULL, *newpar = NULL;
	__be32 initial_match_array[MAX_PHANDLE_ARGS];
	const __be32 *match_array = initial_match_array;
	const __be32 *tmp, *imap, *imask, dummy_imask[] = { 
        [0 ... MAX_PHANDLE_ARGS] = cpu_to_be32(~0) };
	u32 intsize = 1, addrsize, newintsize = 0, newaddrsize = 0;
	int imaplen, match, i, rc = -EINVAL;

#ifdef DEBUG
	of_print_phandle_args("of_irq_parse_raw: ", out_irq);
#endif

	ipar = of_node_get(out_irq->np);

	/* First get the #interrupt-cells property of the current cursor
	 * that tells us how to interpret the passed-in intspec. If there
	 * is none, we are nice and just walk up the tree
	 */
	do {
		if (!of_read_u32(ipar, "#interrupt-cells", &intsize))
			break;
		tnode = ipar;
		ipar = of_irq_find_parent(ipar);
		of_node_put(tnode);
	} while (ipar);
	if (ipar == NULL) {
		pr_debug(" -> no parent found !\n");
		goto fail;
	}

	pr_debug("of_irq_parse_raw: ipar=%pOF, size=%d\n", ipar, intsize);

	if (out_irq->args_count != intsize)
		goto fail;

	/* Look for this #address-cells. We have to implement the old linux
	 * trick of looking for the parent here as some device-trees rely on it
	 */
	old = of_node_get(ipar);
	do {
		tmp = of_get_property(old, "#address-cells", NULL);
		tnode = of_get_parent(old);
		of_node_put(old);
		old = tnode;
	} while (old && tmp == NULL);
	of_node_put(old);
	old = NULL;
	addrsize = (tmp == NULL) ? 2 : be32_to_cpu(*tmp);

	pr_debug(" -> addrsize=%d\n", addrsize);

	/* Range check so that the temporary buffer doesn't overflow */
	if (WARN_ON(addrsize + intsize > MAX_PHANDLE_ARGS)) {
		rc = -EFAULT;
		goto fail;
	}

	/* Precalculate the match array - this simplifies match loop */
	for (i = 0; i < addrsize; i++)
		initial_match_array[i] = addr ? addr[i] : 0;
	for (i = 0; i < intsize; i++)
		initial_match_array[addrsize + i] = cpu_to_be32(out_irq->args[i]);

	/* Now start the actual "proper" walk of the interrupt tree */
	while (ipar != NULL) {      
		/* Now check if cursor is an interrupt-controller and if it is
		 * then we are done
		 */
		if (ofnode_read_bool(np_to_ofnode(ipar), "interrupt-controller")) {
			pr_debug(" -> got it !\n");
			return 0;
		}

		/*
		 * interrupt-map parsing does not work without a reg
		 * property when #address-cells != 0
		 */
		if (addrsize && !addr) {
			pr_debug(" -> no reg passed in when needed !\n");
			goto fail;
		}

		/* Now look for an interrupt-map */
		imap = of_get_property(ipar, "interrupt-map", &imaplen);
		/* No interrupt map, check for an interrupt parent */
		if (imap == NULL) {
			pr_debug(" -> no map, getting parent\n");
			newpar = of_irq_find_parent(ipar);
			goto skiplevel;
		}
		imaplen /= sizeof(u32);

		/* Look for a mask */
		imask = of_get_property(ipar, "interrupt-map-mask", NULL);
		if (!imask)
			imask = dummy_imask;

		/* Parse interrupt-map */
		match = 0;
		while (imaplen > (addrsize + intsize + 1) && !match) {
			/* Compare specifiers */
			match = 1;
			for (i = 0; i < (addrsize + intsize); i++, imaplen--)
				match &= !((match_array[i] ^ *imap++) & imask[i]);

			pr_debug(" -> match=%d (imaplen=%d)\n", match, imaplen);

			/* Get the interrupt parent */
			if (of_irq_workarounds & OF_IMAP_NO_PHANDLE)
				newpar = of_node_get(of_irq_dflt_pic);
			else
				newpar = of_find_node_by_phandle(be32_to_cpup(imap));
			imap++;
			--imaplen;

			/* Check if not found */
			if (newpar == NULL) {
				pr_debug(" -> imap parent not found !\n");
				goto fail;
			}

			if (!of_device_is_available(newpar))
				match = 0;

			/* Get #interrupt-cells and #address-cells of new
			 * parent
			 */
			if (of_read_u32(newpar, "#interrupt-cells",
						 &newintsize)) {
				pr_debug(" -> parent lacks #interrupt-cells!\n");
				goto fail;
			}
			if (of_read_u32(newpar, "#address-cells",
						 &newaddrsize))
				newaddrsize = 0;

			pr_debug(" -> newintsize=%d, newaddrsize=%d\n",
			    newintsize, newaddrsize);

			/* Check for malformed properties */
			if (WARN_ON(newaddrsize + newintsize > MAX_PHANDLE_ARGS)
			    || (imaplen < (newaddrsize + newintsize))) {
				rc = -EFAULT;
				goto fail;
			}

			imap += newaddrsize + newintsize;
			imaplen -= newaddrsize + newintsize;

			pr_debug(" -> imaplen=%d\n", imaplen);
		}
		if (!match)
			goto fail;

		/*
		 * Successfully parsed an interrrupt-map translation; copy new
		 * interrupt specifier into the out_irq structure
		 */
		match_array = imap - newaddrsize - newintsize;
		for (i = 0; i < newintsize; i++)
			out_irq->args[i] = be32_to_cpup(imap - newintsize + i);
		out_irq->args_count = intsize = newintsize;
		addrsize = newaddrsize;

	skiplevel:
		/* Iterate again with new parent */
		out_irq->np = newpar;
		pr_debug(" -> new parent: %pOF\n", newpar);
		of_node_put(ipar);
		ipar = newpar;
		newpar = NULL;
	}
	rc = -ENOENT; /* No interrupt-map found */

 fail:
	of_node_put(ipar);
	of_node_put(newpar);

	return rc;
}

static int of_irq_parse_one(struct device_node *device, int index, 
    struct of_phandle_args *out_irq)
{
	struct device_node *p;
	const __be32 *addr;
	u32 intsize;
	int i, res;

	pr_debug("of_irq_parse_one: dev=%pOF, index=%d\n", device, index);

	/* OldWorld mac stuff is "special", handle out of line */
	if (of_irq_workarounds & OF_IMAP_OLDWORLD_MAC)
		return of_irq_parse_oldworld(device, index, out_irq);

	/* Get the reg property (if any) */
	addr = of_get_property(device, "reg", NULL);

	/* Try the new-style interrupts-extended first */
	res = of_parse_phandle_with_args(device, "interrupts-extended",
					"#interrupt-cells", 0, index, out_irq);
	if (!res)
		return of_irq_parse_raw(addr, out_irq);

	/* Look for the interrupt parent. */
	p = of_irq_find_parent(device);
	if (p == NULL)
		return -EINVAL;

	/* Get size of interrupt specifier */
	if (of_read_u32(p, "#interrupt-cells", &intsize)) {
		res = -EINVAL;
		goto out;
	}

	pr_debug(" parent=%pOF, intsize=%d\n", p, intsize);

	/* Copy intspec into irq structure */
	out_irq->np = p;
	out_irq->args_count = intsize;
	for (i = 0; i < intsize; i++) {
		res = of_read_u32_index(device, "interrupts",
						 (index * intsize) + i,
						 out_irq->args + i);
		if (res)
			goto out;
	}

	pr_debug(" intspec=%d\n", *out_irq->args);


	/* Check if there are any interrupt-map translations to process */
	res = of_irq_parse_raw(addr, out_irq);
 out:
	of_node_put(p);
	return res;
}

int of_irq_get(struct device_node *dev, int index, unsigned int *irq, 
    unsigned int *type)
{
	struct of_phandle_args oirq;
    int rc;

	rc = of_irq_parse_one(dev, index, &oirq);
	if (rc)
		return rc;
	return of_irq_translate(&oirq, irq, type);
}

int of_irq_count(struct device_node *dev)
{
	struct of_phandle_args irq;
	int nr = 0;

	while (of_irq_parse_one(dev, nr, &irq) == 0)
		nr++;

	return nr;
}

fdt_addr_t dev_read_irq_index(const struct udevice *dev, int index, 
    unsigned int *type)
{
	struct device_node *np;
    unsigned int irq = FDT_ADDR_T_NONE;
    unsigned int itype;
    
    if (ofnode_is_np(dev_ofnode(dev))) {
		np = (void *)ofnode_to_np(dev_ofnode(dev));
        int ret = of_irq_get(np, index, &irq, &itype);
        if (ret)
            return FDT_ADDR_T_NONE;
        if (type)
            *type = itype;
    }
    return irq;
}
