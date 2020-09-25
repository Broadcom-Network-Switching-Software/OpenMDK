/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS register access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_reg.h>

/*******************************************************************************
 *
 * Write to internal block-based register(s).
 * If port is negative, write to all blocks containing this register,
 * otherwise write only to the block that contains port.
 */

int
cdk_xgsd_reg_blocks_write(int unit, uint32_t blkacc, int port,
                          uint32_t offset, int idx, void *vptr, int size)
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
                rv = cdk_xgsd_reg_block_write(unit, blkacc, blkp->blknum,
                                              offset, idx, vptr, size); 
                if (CDK_FAILURE(rv)) {
                    break;
                }
            }
        }
    }   
    return rv; 
}
