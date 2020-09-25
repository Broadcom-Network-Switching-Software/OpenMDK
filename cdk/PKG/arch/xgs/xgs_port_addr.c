/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Common XGS chip functions.
 */

#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_debug.h>

/*
 * Calculate address of port-based register
 */
uint32_t
cdk_xgs_port_addr(int unit, int port, uint32_t offset)
{
    int idx, p, blk_port = 0;
    int block = CDK_XGS_ADDR2BLOCK(offset);
    int btype = -1;
    const cdk_xgs_block_t *blkp = CDK_XGS_INFO(unit)->blocks;

    /*
     * Determine which block this port belongs to.
     * Note that if block = CMIC block this may be a register
     * that exists in more than one port block type.
     */
    for (idx = 0; idx < CDK_XGS_INFO(unit)->nblocks; idx++) {  
        if (block == blkp->blknum) {
            btype = blkp->type;
        }
        if ((block == CDK_XGS_INFO(unit)->cmic_block || blkp->type == btype) &&
            CDK_PBMP_MEMBER(blkp->pbmps, port)) {
            block = blkp->blknum;
            break;
        }
        blkp++;
    }

    /* Get the physical port number within this block */
    CDK_PBMP_ITER(blkp->pbmps, p) {
        if (p == port) {
            return cdk_xgs_blockport_addr(unit, block, blk_port, offset);
        }
        blk_port++;
    }

    /*
     * If we get here then something is not right, but we do not 
     * want to assert because we could have been called from the 
     * CLI with a raw address.
     */
    CDK_WARN(("cdk_xgs_port_addr[%d]: invalid port %d "
              "for offset 0x%08"PRIx32"\n",
              unit, port, offset));

    return offset; 
}

