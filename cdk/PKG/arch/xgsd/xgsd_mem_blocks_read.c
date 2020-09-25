/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS memory access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_mem.h>

/*******************************************************************************
 *
 * Read from block-based memory.
 * If port is negative, read from any block containing this memory,
 * otherwise read only from the block that contains port.
 */

int
cdk_xgsd_mem_blocks_read(int unit, uint32_t blkacc, int port, uint32_t offset,
                         int idx, void *entry_data, int size)
{
    int i; 
    int rv = CDK_E_PORT;
    cdk_pbmp_t pbmp;
    const cdk_xgsd_block_t *blkp = CDK_XGSD_INFO(unit)->blocks; 
    
    CDK_ASSERT(blkp); 
    
    /* Iterate over all physical blocks of this type */
    for (i = 0; i < CDK_XGSD_INFO(unit)->nblocks; i++, blkp++) {
        if ((1 << blkp->type) & blkacc) {
            CDK_PBMP_ASSIGN(pbmp, blkp->pbmps);
            if (blkp->ptype == 0) {
                CDK_PBMP_AND(pbmp, CDK_XGSD_INFO(unit)->valid_pbmps);
            }
            /* Skip unused blocks */
            if ((port < 0  && CDK_PBMP_NOT_NULL(pbmp)) ||
                CDK_PBMP_MEMBER(blkp->pbmps, port)) {
                rv = cdk_xgsd_mem_block_read(unit, blkacc, blkp->blknum,
                                             offset, idx, entry_data, size); 
                break;
            }
        }
    }   
    return rv; 
}	
