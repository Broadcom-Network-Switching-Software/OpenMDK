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
cdk_xgsm_block_number(int unit, int blktype, int bindex)
{
    /* Calculate the physical block number for a given blocktype instance */
    int i; 

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 
    
    for (i = 0; i < CDK_XGSM_INFO(unit)->nblocks; i++) {
        if (CDK_XGSM_INFO(unit)->blocks[i].type == blktype) {
            if (bindex == 0) {
                return CDK_XGSM_INFO(unit)->blocks[i].blknum;
            }
            bindex--;
        }
    }
    /* The requested block number is not valid */
    return -1; 
}
