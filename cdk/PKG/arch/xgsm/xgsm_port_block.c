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
#include <cdk/arch/xgsm_chip.h>

int
cdk_xgsm_port_block(int unit, int port, cdk_xgsm_pblk_t *dst, 
                    int blktype)
{
    /* Get the block association for a port number */
    int i, p; 
    int blk_port, blk_index = 0; 

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 
    
    /* Foreach block in the chip */
    for (i = 0; i < CDK_XGSM_INFO(unit)->nblocks; i++) {  
        const cdk_xgsm_block_t *blkp = CDK_XGSM_INFO(unit)->blocks+i; 

        if (blkp->type != blktype) {
            continue;
        }

        /* Track physical port number within block */
        blk_port = 0; 

        /* Foreach port in these blocks */
        CDK_PBMP_ITER(blkp->pbmps, p) {
            if (p == port) {
                /* Port belongs to this block */
                dst->btype = blkp->type; 

                /* Instance of this blocktype */
                dst->bindex = blk_index; 

                /* Physical port number in the block as well */
                dst->bport = blk_port; 

                /* Physical block number */
                dst->block = blkp->blknum;
                    
                if (CDK_PBMP_MEMBER(CDK_XGSM_INFO(unit)->valid_pbmps, port)) {
                    return 0;
                }
                /* Physical ports (ptype = 0) must be valid for device */
                return (blkp->ptype == 0) ? -1 : 0;
            }
            blk_port++;
        }
        blk_index++;
    }
    return -1;
}
