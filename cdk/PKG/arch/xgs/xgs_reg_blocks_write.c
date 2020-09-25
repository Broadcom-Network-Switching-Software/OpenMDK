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

#include <cdk/arch/xgs_chip.h>
#include <cdk/arch/xgs_reg.h>

/*******************************************************************************
 *
 * Write to internal block-based register(s).
 * If port is negative, write to all blocks containing this register,
 * otherwise write only to the block that contains port.
 */

int
cdk_xgs_reg_blocks_write(int unit, uint32_t blktypes, int port,
                         uint32_t addr, void *vptr, int size)
{
    int i; 
    int rv = CDK_E_PORT;
    cdk_pbmp_t pbmp;
    const cdk_xgs_block_t *blkp = CDK_XGS_INFO(unit)->blocks; 
    
    CDK_ASSERT(blkp); 
    
    /* Iterate over all physical blocks of this type */
    for (i = 0; i < CDK_XGS_INFO(unit)->nblocks; i++, blkp++) {
        if ((1 << blkp->type) & blktypes) {
            CDK_PBMP_ASSIGN(pbmp, blkp->pbmps);
            CDK_PBMP_AND(pbmp, CDK_XGS_INFO(unit)->valid_pbmps);
            /* Skip unused blocks and invalid ports */
            if ((port < 0  && CDK_PBMP_NOT_NULL(pbmp)) ||
                CDK_PBMP_MEMBER(pbmp, port)) {
                rv = cdk_xgs_reg_block_write(unit, blkp->blknum,
                                             addr, vptr, size); 
                if (CDK_FAILURE(rv)) {
                    break;
                }
            }
        }
    }   
    return rv; 
}
