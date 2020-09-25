/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Common XGSD chip functions.
 */

#include <cdk/cdk_assert.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_chip.h>
#include <cdk/arch/xgsd_chip.h>

const cdk_xgsd_block_t * 
cdk_xgsd_block(int unit, int blktype)
{
    /* Get the chip's block structure */
    int i; 

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 
    
    for (i = 0; i < CDK_XGSD_INFO(unit)->nblocks; i++) {
        if (CDK_XGSD_INFO(unit)->blocks[i].type == blktype) {
            return CDK_XGSD_INFO(unit)->blocks+i;
        }
    }
    return NULL; 
}
