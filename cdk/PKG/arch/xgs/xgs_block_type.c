/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Common XGS chip functions.
 */

#include <cdk/cdk_assert.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_chip.h>
#include <cdk/arch/xgs_chip.h>

int
cdk_xgs_block_type(int unit, int block, int *blktype, int *n)
{
    /* Calculate the blocktype and instance for a physical block number */
    int i; 

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 
    
    /* Get block type */
    for (i = 0; i < CDK_XGS_INFO(unit)->nblocks; i++) {
        const cdk_xgs_block_t *blkp = CDK_XGS_INFO(unit)->blocks+i; 
        if (blkp->blknum == block) {
            if (blktype) {
                *blktype = blkp->type; 
            }
            if (n) {
                /* Get instance if requested */
                *n = 0;
                while (i > 0) {
                    i--;
                    blkp--;
                    if (blkp->type == *blktype) {
                        (*n)++;
                    }
                }
            }
            return 0;
        }
    }
    return -1; 
}
