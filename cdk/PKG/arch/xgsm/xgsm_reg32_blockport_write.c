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
 * Access 32 bit internal registers by both block and port. 
 */

int
cdk_xgsm_reg32_blockport_write(int unit, uint32_t blkacc, int block, int port,
                               uint32_t offset, int idx, void *data)
{
    uint32_t addr, adext = CDK_XGSM_BLKACC2ADEXT(blkacc);

    CDK_XGSM_ADEXT_BLOCK_SET(adext, block);
    addr = cdk_xgsm_blockport_addr(unit, block, port, offset, idx);

    return cdk_xgsm_reg32_write(unit, adext, addr, data);
}
