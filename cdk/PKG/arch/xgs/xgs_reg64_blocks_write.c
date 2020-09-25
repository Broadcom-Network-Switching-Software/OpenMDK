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

#include <cdk/arch/xgs_chip.h>
#include <cdk/arch/xgs_reg.h>

/*******************************************************************************
 *
 * Write to internal 64-bit block-based register(s).
 * If port is negative, write to all blocks containing this register,
 * otherwise write only to the block that contains port.
 */

int
cdk_xgs_reg64_blocks_write(int unit, uint32_t blktypes, int port,
                           uint32_t addr, void *vptr)
{
    return cdk_xgs_reg_blocks_write(unit, blktypes, port, addr, vptr, 2);
}
