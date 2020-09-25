/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Access function for shadowed registers on Broadcom Gigabit PHYs.
 */

#include <phy/phy_brcm_xe.h>

/* Shadow registers reside at offset 0xfff0 in clause 45 device 7 */
#define _SHDW(_regad) ((_regad) | 0x7fff0)

int
phy_brcm_xe_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data)
{
    int ioerr = 0;
    uint32_t reg_addr, reg_bank;

    if (addr & PHY_REG_ACC_BRCM_XE_SHADOW) {
        reg_addr = addr & 0x1f;
        reg_bank = (addr >> 8) & 0xffff;

        switch(reg_addr) {
        case 0x15:
            ioerr += PHY_BUS_WRITE(pc, _SHDW(0x17), reg_bank);
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
        default:
            break;
        }
        ioerr += PHY_BUS_WRITE(pc, _SHDW(reg_addr), data);
    } else {
        ioerr += PHY_BUS_WRITE(pc, addr & 0x1fffff, data);
    }

    return ioerr;
}
