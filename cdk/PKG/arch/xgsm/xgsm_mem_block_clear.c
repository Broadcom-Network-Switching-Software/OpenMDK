/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS memory access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/arch/xgsm_mem.h>

/*******************************************************************************
 *
 * Access block-based memories
 */

int	
cdk_xgsm_mem_block_clear(int unit, uint32_t blkacc, int block,
                         uint32_t offset, uint32_t si, uint32_t ei, int size)
{
    uint32_t addr, adext = CDK_XGSM_BLKACC2ADEXT(blkacc);

    addr = cdk_xgsm_blockport_addr(unit, block, -1, offset, 0);

    /* Update address extension with specified block */
    CDK_XGSM_ADEXT_BLOCK_SET(adext, block);

    return cdk_xgsm_mem_clear(unit, adext, addr, si, ei, size); 
}
