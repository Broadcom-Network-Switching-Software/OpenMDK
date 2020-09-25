/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS internal PHY write function with register block support.
 *
 * This function accepts a 32-bit PHY register address and will
 * map the PHY register block according to the upper 24 bits, e.g. 
 * address 0x0112 designates register 0x12 in block 0x01.
 */

#include <cdk/cdk_device.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_miim.h>

int 
cdk_xgsd_miim_iblk_write(int unit, uint32_t phy_addr, uint32_t reg, uint32_t val)
{
    int ioerr = 0;
    uint32_t blk;

    /* Select block */
    ioerr += cdk_xgsd_miim_read(unit, phy_addr, 0x1f, &blk);
    if ((reg >> 8) ^ blk) {
        ioerr += cdk_xgsd_miim_write(unit, phy_addr, 0x1f, reg >> 8);
    }

    /* Write register value */
    ioerr += cdk_xgsd_miim_write(unit, phy_addr, reg & 0x1f, val);

    /* Restore block */
    if ((reg >> 8) ^ blk) {
        ioerr += cdk_xgsd_miim_write(unit, phy_addr, 0x1f, blk);
    }
    
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
