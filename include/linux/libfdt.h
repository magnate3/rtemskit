/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _INCLUDE_LIBFDT_H_
#define _INCLUDE_LIBFDT_H_

#ifndef USE_HOSTCC
#include <linux/libfdt_env.h>
#endif
#include <libfdt.h>

/* U-Boot local hacks */
extern struct fdt_header *working_fdt;  /* Pointer to the working fdt */
extern const void *_dm_fdt_blob;

static inline const void *dm_fdt_blob(void)
{
    return _dm_fdt_blob;
}

#endif /* _INCLUDE_LIBFDT_H_ */
