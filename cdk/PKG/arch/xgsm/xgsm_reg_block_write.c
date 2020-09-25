/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS register access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/arch/xgsm_reg.h>
    
/*******************************************************************************
 *
 * Access internal block-based registers
 */

int
cdk_xgsm_reg_block_write(int unit, uint32_t blkacc, int block,
                         uint32_t offset, int idx, void *data, int size)
{
    uint32_t addr, adext = CDK_XGSM_BLKACC2ADEXT(blkacc);

    addr = cdk_xgsm_blockport_addr(unit, block, 0, offset, idx);

    /* Update address extension with specified block */
    CDK_XGSM_ADEXT_BLOCK_SET(adext, block);

    return cdk_xgsm_reg_write(unit, adext, addr, data, size);
}
