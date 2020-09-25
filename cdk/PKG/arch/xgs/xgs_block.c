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

const cdk_xgs_block_t * 
cdk_xgs_block(int unit, int blktype)
{
    /* Get the chip's block structure */
    int i; 

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 
    
    for (i = 0; i < CDK_XGS_INFO(unit)->nblocks; i++) {
        if (CDK_XGS_INFO(unit)->blocks[i].type == blktype) {
            return CDK_XGS_INFO(unit)->blocks+i;
        }
    }
    return NULL; 
}
