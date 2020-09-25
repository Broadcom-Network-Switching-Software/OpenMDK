/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Common XGS chip functions.
 */

#include <cdk/arch/xgs_chip.h>

/*
 * Calculate address of block-based register
 */
uint32_t
cdk_xgs_block_addr(int unit, int block, uint32_t offset)
{
    return cdk_xgs_blockport_addr(unit, block, 0, offset); 
}

