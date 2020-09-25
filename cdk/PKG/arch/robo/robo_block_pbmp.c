/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Common ROBO chip functions.
 */

#include <cdk/cdk_assert.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_chip.h>
#include <cdk/arch/robo_chip.h>

int
cdk_robo_block_pbmp(int unit, int blktype, cdk_pbmp_t *pbmp)
{
    const cdk_robo_block_t *blkp = CDK_ROBO_INFO(unit)->blocks; 
    int idx;

    CDK_ASSERT(blkp); 

    CDK_PBMP_CLEAR(*pbmp);
    
    /* Iterate over all physical blocks of this type */
    for (idx = 0; idx < CDK_ROBO_INFO(unit)->nblocks; idx++, blkp++) {
        if (blkp->type == blktype) {
            CDK_PBMP_OR(*pbmp, blkp->pbmps);
            /* NOTE: Use CDK device port bimap here */
            CDK_PBMP_AND(*pbmp, CDK_DEV(unit)->valid_pbmps);
        }
    }

    return 0; 
}
