/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Access function for fiber registers on Broadcom Gigabit PHYs.
 */

#include <phy/phy_brcm_1000x.h>

int
phy_brcm_1000x_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data)
{
    int ioerr = 0;
    uint32_t reg_addr = addr & 0x1f;
    uint32_t blk_sel;;

    /* Map fiber registers */
    ioerr += PHY_BUS_WRITE(pc, 0x1c, 0x7c00);
    ioerr += PHY_BUS_READ(pc, 0x1c, &blk_sel);
    ioerr += PHY_BUS_WRITE(pc, 0x1c, blk_sel | 0x8001);

    /* Read requested fiber register */
    ioerr += PHY_BUS_READ(pc, reg_addr, data);

    /* Map copper registers */
    ioerr += PHY_BUS_WRITE(pc, 0x1c, (blk_sel & 0x7ffe) | 0x8000);

    return ioerr;
}
