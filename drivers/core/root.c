// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2013 Google, Inc
 *
 * (C) Copyright 2012
 * Pavel Herrmann <morpheus.ibis@gmail.com>
 */

#include <rtems/malloc.h>
#include <rtems/sysinit.h>

#include <common.h>
#include <errno.h>
#include <fdtdec.h>
#include <log.h>
#include <asm-generic/sections.h>
#include <linux/libfdt.h>
#include <dm/acpi.h>
#include <dm/device.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/of.h>
#include <dm/of_access.h>
#include <dm/platdata.h>
#include <dm/read.h>
#include <dm/root.h>
#include <dm/uclass.h>
#include <dm/util.h>
#include <linux/list.h>

#include "base/modinit.h"

struct udevice *_dm_root;
struct list_head _uclass_root_s;
struct list_head *_uclass_root;

#if CONFIG_IS_ENABLED(OF_PLATDATA_RT)
struct udevice_rt *_dm_udevice_rt;
void *_dm_priv_base;
#endif

# if CONFIG_IS_ENABLED(OF_PLATDATA_DRIVER_RT)
struct driver_rt *_dm_driver_rt;
#endif

static struct driver_info root_info = {
	.name = "root_driver",
};

struct udevice *dm_root(void)
{
	if (!_dm_root) {
		dm_warn("Virtual root driver does not exist!\n");
		return NULL;
	}
	return _dm_root;
}

static int dm_setup_inst(void)
{
	DM_ROOT_NON_CONST = DM_DEVICE_GET(root);

	if (IS_ENABLED(CONFIG_OF_PLATDATA_RT)) {
		struct udevice_rt *urt;
		void *base;
		int n_ents;
		uint size;

		/* Allocate the udevice_rt table */
		n_ents = ll_entry_count(struct udevice, udevice);
		urt = rtems_calloc(n_ents, sizeof(struct udevice_rt));
		if (!urt)
			return log_msg_ret("urt", -ENOMEM);
		gd_set_dm_udevice_rt(urt);

		/* Now allocate space for the priv/plat data, and copy it in */
		size = __priv_data_end - __priv_data_start;

		base = rtems_calloc(1, size);
		if (!base)
			return log_msg_ret("priv", -ENOMEM);
		memcpy(base, __priv_data_start, size);
		gd_set_dm_priv_base(base);
	}

	return 0;
}

int dm_init(bool of_live)
{
	int ret;

	if (_dm_root) {
		dm_warn("Virtual root driver already exists!\n");
		return -EINVAL;
	}
	if (IS_ENABLED(CONFIG_OF_PLATDATA_INST)) {
		_uclass_root = &uclass_head;
	} else {
		_uclass_root = &DM_UCLASS_ROOT_S_NON_CONST;
		INIT_LIST_HEAD(DM_UCLASS_ROOT_NON_CONST);
	}

	if (IS_ENABLED(CONFIG_OF_PLATDATA_INST)) {
		ret = dm_setup_inst();
		if (ret) {
			log_debug("dm_setup_inst() failed: %d\n", ret);
			return ret;
		}
	} else {
		ret = device_bind_by_name(NULL, false, &root_info,
					  &DM_ROOT_NON_CONST);
		if (ret)
			return ret;
		if (CONFIG_IS_ENABLED(OF_CONTROL))
			dev_set_ofnode(DM_ROOT_NON_CONST, ofnode_root());
		ret = device_probe(DM_ROOT_NON_CONST);
		if (ret)
			return ret;
	}

	return 0;
}

int dm_uninit(void)
{
	/* Remove non-vital devices first */
	device_remove(dm_root(), DM_REMOVE_NON_VITAL);
	device_remove(dm_root(), DM_REMOVE_NORMAL);
	device_unbind(dm_root());
	_dm_root = NULL;
	return 0;
}

int dm_remove_devices_flags(uint flags)
{
	device_remove(dm_root(), flags);
	return 0;
}

int dm_scan_plat(bool pre_reloc_only)
{
	int ret;

	if (CONFIG_IS_ENABLED(OF_PLATDATA_DRIVER_RT)) {
		struct driver_rt *dyn;
		int n_ents;

		n_ents = ll_entry_count(struct driver_info, driver_info);
		dyn = rtems_calloc(n_ents, sizeof(struct driver_rt));
		if (!dyn)
			return -ENOMEM;
		gd_set_dm_driver_rt(dyn);
	}

	ret = lists_bind_drivers(DM_ROOT_NON_CONST, pre_reloc_only);
	if (ret == -ENOENT) {
		dm_warn("Some drivers were not found\n");
		ret = 0;
	}

	return ret;
}

#if CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)
/**
 * dm_scan_fdt_node() - Scan the device tree and bind drivers for a node
 *
 * This scans the subnodes of a device tree node and and creates a driver
 * for each one.
 *
 * @parent: Parent device for the devices that will be created
 * @node: Node to scan
 * @pre_reloc_only: If true, bind only drivers with the DM_FLAG_PRE_RELOC
 * flag. If false bind all drivers.
 * @return 0 if OK, -ve on error
 */
static int dm_scan_fdt_node(struct udevice *parent, ofnode parent_node,
			    bool pre_reloc_only)
{
	int ret = 0, err = 0;
	ofnode node;

	if (!ofnode_valid(parent_node))
		return 0;

	for (node = ofnode_first_subnode(parent_node);
	     ofnode_valid(node);
	     node = ofnode_next_subnode(node)) {
		const char *node_name = ofnode_get_name(node);

		if (!ofnode_is_enabled(node)) {
			pr_debug("   - ignoring disabled device\n");
			continue;
		}
		err = lists_bind_fdt(parent, node, NULL, pre_reloc_only);
		if (err && !ret) {
			ret = err;
			debug("%s: ret=%d\n", node_name, ret);
		}
	}

	if (ret)
		dm_warn("Some drivers failed to bind\n");

	return ret;
}

int dm_scan_fdt_dev(struct udevice *dev)
{
	return dm_scan_fdt_node(dev, dev_ofnode(dev), false);
}

int dm_scan_fdt(bool pre_reloc_only)
{
	return dm_scan_fdt_node(_dm_root, ofnode_root(), pre_reloc_only);
}

static int dm_scan_fdt_ofnode_path(const char *path, bool pre_reloc_only)
{
	ofnode node;

	node = ofnode_path(path);
	return dm_scan_fdt_node(_dm_root, node, pre_reloc_only);
}

int dm_extended_scan(bool pre_reloc_only)
{
	int ret, i;
	const char * const nodes[] = {
		"/chosen",
		"/clocks",
		"/firmware"
	};

	ret = dm_scan_fdt(pre_reloc_only);
	if (ret) {
		debug("dm_scan_fdt() failed: %d\n", ret);
		return ret;
	}

	/* Some nodes aren't devices themselves but may contain some */
	for (i = 0; i < ARRAY_SIZE(nodes); i++) {
		ret = dm_scan_fdt_ofnode_path(nodes[i], pre_reloc_only);
		if (ret) {
			debug("dm_scan_fdt() scan for %s failed: %d\n",
			      nodes[i], ret);
			return ret;
		}
	}

	return ret;
}
#endif

__weak int dm_scan_other(bool pre_reloc_only)
{
	return 0;
}

#if CONFIG_IS_ENABLED(OF_PLATDATA_INST) && CONFIG_IS_ENABLED(READ_ONLY)
void *dm_priv_to_rw(void *priv)
{
	long offset = priv - (void *)__priv_data_start;

	return gd_dm_priv_base() + offset;
}
#endif

/**
 * dm_scan() - Scan tables to bind devices
 *
 * Runs through the driver_info tables and binds the devices it finds. Then runs
 * through the devicetree nodes. Finally calls dm_scan_other() to add any
 * special devices
 *
 * @pre_reloc_only: If true, bind only nodes with special devicetree properties,
 * or drivers with the DM_FLAG_PRE_RELOC flag. If false bind all drivers.
 */
static int dm_scan(bool pre_reloc_only)
{
	int ret;

	ret = dm_scan_plat(pre_reloc_only);
	if (ret) {
		debug("dm_scan_plat() failed: %d\n", ret);
		return ret;
	}

	if (CONFIG_IS_ENABLED(OF_CONTROL) && !CONFIG_IS_ENABLED(OF_PLATDATA)) {
		ret = dm_extended_scan(pre_reloc_only);
		if (ret) {
			debug("dm_extended_scan() failed: %d\n", ret);
			return ret;
		}
	}

	ret = dm_scan_other(pre_reloc_only);
	if (ret)
		return ret;

	return 0;
}

int dm_init_and_scan(bool pre_reloc_only)
{
	int ret;

	ret = dm_init(CONFIG_IS_ENABLED(OF_LIVE));
	if (ret) {
		debug("dm_init() failed: %d\n", ret);
		return ret;
	}
	if (!CONFIG_IS_ENABLED(OF_PLATDATA_INST)) {
		ret = dm_scan(pre_reloc_only);
		if (ret) {
			log_debug("dm_scan() failed: %d\n", ret);
			return ret;
		}
	}

	return 0;
}

#ifdef CONFIG_ACPIGEN
static int root_acpi_get_name(const struct udevice *dev, char *out_name)
{
	return acpi_copy_name(out_name, "\\_SB");
}

struct acpi_ops root_acpi_ops = {
	.get_name	= root_acpi_get_name,
};
#endif

static void dm_devices_probe(struct udevice *dev)
{
	struct udevice *child;
	
	list_for_each_entry(child, &dev->child_head, sibling_node) {
		device_probe(child);
		dm_devices_probe(child);
	}
}

static void dm_root_init(void)
{
	int ret = dm_init_and_scan(false);
	if (ret)
		rtems_panic("DM initialize failed: %d\n", ret);
}

static int device_drivers_init(void)
{
	dm_devices_probe(_dm_root);
	return 0;
}

module_driver(device_drivers_init, 
	MOD_BASE, FIRST_ORDER);

RTEMS_SYSINIT_ITEM(dm_root_init,
    RTEMS_SYSINIT_BSP_PRE_DRIVERS,
    RTEMS_SYSINIT_ORDER_FIRST
);

/* This is the root driver - all drivers are children of this */
DM_DRIVER(root_driver) = {
	.name	= "root_driver",
	.id	= UCLASS_ROOT,
	ACPI_OPS_PTR(&root_acpi_ops)
};

/* This is the root uclass */
UCLASS_DRIVER(root) = {
	.name	= "root",
	.id	= UCLASS_ROOT,
};
