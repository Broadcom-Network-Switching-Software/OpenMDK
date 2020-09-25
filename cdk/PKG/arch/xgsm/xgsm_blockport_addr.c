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

uint32_t
cdk_xgsm_blockport_addr(int unit, int block, int blkport,
                        uint32_t offset, int idx)
{
    uint32_t addr;

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 

    /* Neutralize idx if unused */
    if (idx < 0) {
        idx = 0;
    }
    
    /* The translation can be specified per-chip if necessary */
    if (CDK_XGSM_INFO(unit)->block_port_addr) {
        addr = CDK_XGSM_INFO(unit)->block_port_addr(block, blkport, offset, idx);
    }
    else if (blkport < 0) {
        /* Default block/port calculation for memories */
        addr = offset; 
        addr += idx;
    } else {
        /* Default block/port calculation for registers */
        addr = (offset | blkport); 
        addr += (idx * 0x100);
    }
    return addr;
}
