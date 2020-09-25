/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BMD_DMA_H__
#define __BMD_DMA_H__

#include <bmd_config.h>

extern void *
bmd_dma_alloc_coherent(int unit, size_t size, dma_addr_t *baddr);

extern void
bmd_dma_free_coherent(int unit, size_t size, void *laddr, dma_addr_t baddr);

/*
 * The DMA cache control macros below are defined to allow simple cache
 * management without modifying the BMD source code.
 *
 * The DMA memory allocated by the BMD should be coherent either by system
 * architecture (cache-snooping) or explicitly (uncached).  In cases where
 * it is suspected that DMA memory is still being cached, the below macros
 * can be activated to call system-supplied cache flush and invalidate 
 * functions.
 */

#if BMD_CONFIG_INCLUDE_DMA_CACHE_CONTROL

extern void
bmd_dma_cache_flush(void *addr, size_t len);

extern void
bmd_dma_cache_inval(void *addr, size_t len);

#define BMD_DMA_CACHE_FLUSH(_a, _l) bmd_dma_cache_flush(_a, _l)
#define BMD_DMA_CACHE_INVAL(_a, _l) bmd_dma_cache_inval(_a, _l)

#else

#define BMD_DMA_CACHE_FLUSH(_a, _l)
#define BMD_DMA_CACHE_INVAL(_a, _l)

#endif

#endif /* __BMD_DMA_H__ */
