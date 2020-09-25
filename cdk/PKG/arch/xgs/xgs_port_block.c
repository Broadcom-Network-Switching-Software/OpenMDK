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
cdk_xgs_port_block(int unit, int port, cdk_xgs_pblk_t *dst, 
                   int blktype)
{
    /* Get the block association for a port number */
    int i, p; 
    int blk_port, blk_index = 0; 

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 
    
    /* Foreach block type in the chip */
    for (i = 0; i < CDK_XGS_INFO(unit)->nblocks; i++) {  
        const cdk_xgs_block_t *blkp = CDK_XGS_INFO(unit)->blocks+i; 

        if (blkp->type != blktype) {
            continue;
        }

        /* Track physical port number within block */
        blk_port = 0; 

        /* Foreach port in these blocks */
        CDK_PBMP_ITER(blkp->pbmps, p) {
            /* Skip if port is not valid for this chip */
            if (!CDK_PBMP_MEMBER(CDK_XGS_INFO(unit)->valid_pbmps, p)) {
                blk_port++;
                continue;
            }
            if (p == port) {
                /* Port belongs to this block */
                dst->btype = blkp->type; 

                /* Instance of this blocktype */
                dst->bindex = blk_index; 

                /* Physical port number in the block as well */
                dst->bport = blk_port; 

                /* Physical block number */
                dst->block = blkp->blknum;
                    
                return 0;
            }
            blk_port++;
        }
        blk_index++;
    }
    return -1;
}
