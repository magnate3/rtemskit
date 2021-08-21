/* SPDX-License-Identifier: GPL-2.0 */
#ifndef BASE_SCATTERLIST_H_
#define BASE_SCATTERLIST_H_

#include "linux/types.h"

#ifdef __cplusplus
extern "C"{
#endif

struct scatterlist {
	unsigned int	length;
	dma_addr_t	dma_address;
};


/*
 * These macros should be used after a dma_map_sg call has been done
 * to get bus addresses of each of the SG entries and their lengths.
 * You should only work with the number of sg entries dma_map_sg
 * returns, or alternatively stop on the first sg_dma_len(sg) which
 * is 0.
 */
#define sg_dma_address(sg)	((sg)->dma_address)
#define sg_dma_len(sg)		((sg)->length)
#define sg_next(sg)             ((sg) + 1)

/*
 * Loop over each sg element, following the pointer to a new list if necessary
 */
#define for_each_sg(sglist, sg, nr, __i)	\
	for (__i = 0, sg = (sglist); __i < (nr); __i++, sg = sg_next(sg))

#define sg_init_table(sg, n)


#ifdef __cplusplus
}
#endif
#endif /* BASE_SCATTERLIST_H_ */

