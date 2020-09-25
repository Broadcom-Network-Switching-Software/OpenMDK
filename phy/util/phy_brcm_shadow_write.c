/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Access function for shadowed registers on Broadcom Gigabit PHYs.
 */

#include <phy/phy_brcm_shadow.h>

int
phy_brcm_shadow_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data)
{
    int ioerr = 0;
    uint32_t reg_addr = addr & 0x1f;
    uint32_t reg_bank = (addr >> 8) & 0xffff;
    uint32_t reg_dev = (addr >> 24) & 0xf;

    switch(reg_addr) {
    case 0x0e:
        ioerr += PHY_BUS_WRITE(pc, 0x0d, reg_dev);
        ioerr += PHY_BUS_WRITE(pc, 0x0e, reg_bank);
        ioerr += PHY_BUS_WRITE(pc, 0x0d, reg_dev | 0x4000);
        break;
    case 0x15:
        ioerr += PHY_BUS_WRITE(pc, 0x17, reg_bank);
        break;
    case 0x18:
        if (reg_bank == 0x0007) {
            data |= 0x8000;
        }
        data = (data & ~(0x0007)) | reg_bank;
        break;
    case 0x1c:
        data = 0x8000 | (reg_bank << 10) | (data & 0x03ff);
        break;
    case 0x1d:
        /* Read-only register */
    default:
        break;
    }
    ioerr += PHY_BUS_WRITE(pc, reg_addr, data);

    return ioerr;
}
