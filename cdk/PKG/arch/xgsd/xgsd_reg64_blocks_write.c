/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS register access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_reg.h>

/*******************************************************************************
 *
 * Write to internal 64-bit block-based register(s).
 * If port is negative, write to all blocks containing this register,
 * otherwise write only to the block that contains port.
 */

int
cdk_xgsd_reg64_blocks_write(int unit, uint32_t blkacc, int port,
                            uint32_t offset, int idx, void *data)
{
    return cdk_xgsd_reg_blocks_write(unit, blkacc, port, offset, idx, data, 2);
}
