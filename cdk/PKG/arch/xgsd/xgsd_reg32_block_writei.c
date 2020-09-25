/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS register access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_reg.h>
    
/*******************************************************************************
 *
 * Access 32 bit internal block-based registers
 */

int
cdk_xgsd_reg32_block_writei(int unit, uint32_t blkacc, int block,
                            uint32_t offset, int idx, uint32_t data)
{
    return cdk_xgsd_reg_block_write(unit, blkacc, block, offset, idx, &data, 1);
}
