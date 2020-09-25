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

uint32_t
cdk_xgs_blockport_addr(int unit, int block, int port, uint32_t offset)
{
    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 
    
    /* The translation can be specified per-chip if necessary */
    if (CDK_XGS_INFO(unit)->block_port_addr) {
        return CDK_XGS_INFO(unit)->block_port_addr(block, port, offset); 
    }
    else {
        /* Default block/port calculations. */
        return ((block * 0x100000) | (port * 0x1000) | (offset & ~0xf00000)); 
    }
}
