/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS memory access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/arch/xgsm_mem.h>

/*******************************************************************************
 *
 * Clear memory in all blocks.
 */

int
cdk_xgsm_mem_blocks_clear(int unit, uint32_t blkacc,
                          uint32_t offset, uint32_t si, uint32_t ei, int size)
{
    int i; 
    int rv = CDK_E_PORT;
    cdk_pbmp_t pbmp;
    const cdk_xgsm_block_t *blkp = CDK_XGSM_INFO(unit)->blocks; 
    
    CDK_ASSERT(blkp); 
    
    /* Iterate over all physical blocks of this type */
    for (i = 0; i < CDK_XGSM_INFO(unit)->nblocks; i++, blkp++) {
        if ((1 << blkp->type) & blkacc) {
            CDK_PBMP_ASSIGN(pbmp, blkp->pbmps);
            if (blkp->ptype == 0) {
                CDK_PBMP_AND(pbmp, CDK_XGSM_INFO(unit)->valid_pbmps);
            }
            /* Skip unused blocks and invalid ports */
            if (CDK_PBMP_NOT_NULL(pbmp)) {
                rv = cdk_xgsm_mem_block_clear(unit, blkacc, blkp->blknum,
                                              offset, si, ei, size); 
                if (CDK_FAILURE(rv)) {
                    break;
                }
            }
        }
    }   
    return rv; 
}	
