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

#include <cdk/arch/xgsm_chip.h>
#include <cdk/arch/xgsm_reg.h>

/*******************************************************************************
 *
 * Read from internal 64-bit block-based register(s).
 * If port is negative, read from any block containing this register,
 * otherwise read only from the block that contains port.
 */

int
cdk_xgsm_reg64_blocks_read(int unit, uint32_t blkacc, int port,
                           uint32_t offset, int idx, void *data)
{
    return cdk_xgsm_reg_blocks_read(unit, blkacc, port, offset, idx, data, 2);
}
