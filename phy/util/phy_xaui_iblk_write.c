/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XAUI internal PHY read function with register block support.
 *
 * This function accepts a 32-bit PHY register address and will
 * map the PHY register block according to the bits [23:8], e.g. 
 * address 0x0112 designates register 0x12 in block 0x01.
 *
 * For IEEE registers access, the IEEE space will be mapped
 * according to the access flags.
 */

#include <phy/phy_xaui_iblk.h>
#include <phy/phy_xgs_iblk.h>

int
phy_xaui_iblk_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data)
{
    int ioerr = 0;
    uint32_t misc_ctrl;

    if ((addr & 0x10) == 0) {
        /* Map Block 0 */
        ioerr += PHY_BUS_WRITE(pc, 0x1f, 0);
        /* Read IEEE mapping */
        ioerr += PHY_BUS_READ(pc, 0x1e, &misc_ctrl);
        misc_ctrl &= ~0x0003;
        if ((addr & PHY_REG_ACC_XAUI_IBLK_CL22) == 0) {
            misc_ctrl |= 0x0001;
        }
        /* Update IEEE mapping */
        ioerr += PHY_BUS_WRITE(pc, 0x1e, misc_ctrl);
    }
    /* Write mapped register */
    ioerr += phy_xgs_iblk_write(pc, addr, data);

    return ioerr;
}
