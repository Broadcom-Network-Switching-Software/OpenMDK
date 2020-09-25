/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Common XGS chip functions.
 */

#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>

/*
 * Calculate address of port-based register
 */
uint32_t
cdk_xgsm_port_addr(int unit, uint32_t blkacc, int port,
                   uint32_t offset, int idx, uint32_t *adext)
{
    int bdx, p, blk_port = 0;
    int block = -1;
    const cdk_xgsm_block_t *blkp = CDK_XGSM_INFO(unit)->blocks;

    /*
     * Determine which block this port belongs to.
     * Note that if block = CMIC block this may be a register
     * that exists in more than one port block type.
     */
    for (bdx = 0; bdx < CDK_XGSM_INFO(unit)->nblocks; bdx++) {  
        if ((blkacc & (1 << blkp->type)) &&
            CDK_PBMP_MEMBER(blkp->pbmps, port)) {
            block = blkp->blknum;
            break;
        }
        blkp++;
    }

    /* Get the physical port number within this block */
    CDK_PBMP_ITER(blkp->pbmps, p) {
        if (p == port) {
            /* Construct address extension from access type and block */
            *adext = CDK_XGSM_BLKACC2ADEXT(blkacc);
            CDK_XGSM_ADEXT_BLOCK_SET(*adext, block);
            return cdk_xgsm_blockport_addr(unit, block, blk_port, offset, idx);
        }
        blk_port++;
    }

    /*
     * If we get here then something is not right, but we do not 
     * want to assert because we could have been called from the 
     * CLI with a raw address.
     */
    CDK_WARN(("cdk_xgsm_port_addr[%d]: invalid port %d "
              "for offset 0x%08"PRIx32"\n",
              unit, port, offset));

    return offset; 
}

