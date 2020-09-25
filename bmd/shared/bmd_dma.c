/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_dma.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/cdk_assert.h>

/*
 * Function:
 *	bmd_dma_alloc_coherent
 * Purpose:
 *      Allocate coherent DMA memory
 * Parameters:
 *      unit - Unit number
 *      size - Size of DMA buffer
 *      baddr - (OUT) Physical bus address of DMA buffer
 * Returns:
 *      Logical DMA buffer address or NULL if error.
 * Notes:
 *      This function is used for allocating coherent DMA
 *      memory, which is typically used for storing DMA
 *      descriptors.
 *      On memory architectures that do not provide cache
 *      coherency, this function should return non-cached
 *      memory.
 */
void *
bmd_dma_alloc_coherent(int unit, size_t size, dma_addr_t *baddr)
{
    void *laddr = NULL;

#if BMD_CONFIG_INCLUDE_DMA
    /* Allocate coherent DMA memory */
    laddr = BMD_SYS_DMA_ALLOC_COHERENT(CDK_DEV_DVC(unit), size, baddr); 
#endif
    return laddr; 
}

/*
 * Function:
 *	bmd_dma_free_coherent
 * Purpose:
 *      Free coherent DMA memory
 * Parameters:
 *      unit - Unit number
 *      size - Size of DMA buffer
 *      laddr - Logical DMA buffer address
 *      baddr - (OUT) Physical bus address of DMA buffer
 * Returns:
 *      Nothing.
 * Notes:
 *      This function is used for freeing DMA memory
 *      allocated with bmd_dma_alloc_coherent.
 */
void
bmd_dma_free_coherent(int unit, size_t size, void *laddr, dma_addr_t baddr)
{
#if BMD_CONFIG_INCLUDE_DMA
    /* Free coherent DMA memory */
    BMD_SYS_DMA_FREE_COHERENT(CDK_DEV_DVC(unit), size, laddr, baddr); 
#endif
}

#if BMD_CONFIG_INCLUDE_DMA_CACHE_CONTROL

/* 
 * Function:
 *      bmd_dma_cache_flush
 * Purpose:
 *      Flush block of DMA memory
 * Parameters:
 *      addr - Address of memory block to flush
 *      len - Size of block
 * Returns:
 *      Nothing
 */
void
bmd_dma_cache_flush(void *addr, size_t len)
{
    BMD_SYS_DMA_CACHE_FLUSH(addr, len);
}

/* 
 * Function:
 *      bmd_dma_cache_inval
 * Purpose:
 *      Invalidate block of DMA memory
 * Parameters:
 *      addr - Address of memory block to invalidate
 *      len - Size of block
 * Returns:
 *      Nothing
 */
void
bmd_dma_cache_inval(void *addr, size_t len)
{
    BMD_SYS_DMA_CACHE_INVAL(addr, len);
}

#endif

