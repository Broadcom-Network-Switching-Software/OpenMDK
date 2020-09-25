/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS internal PHY write function with register block support.
 *
 * This function accepts a 32-bit PHY register address and will
 * map the PHY register block according to the bits [23:8], e.g. 
 * address 0x0112 designates register 0x12 in block 0x01.
 */

#include <phy/phy_xgs_iblk.h>

int
phy_xgs_iblk_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data)
{
    int ioerr = 0;
    uint32_t devad = PHY_REG_IBLK_DEVAD(addr);
    uint32_t blkaddr;

    if (PHY_CTRL_FLAGS(pc) & PHY_F_CLAUSE45) {
        addr = PHY_XGS_IBLK_TO_C45(addr);
        /* DEVAD 0 is not supported, so use DEVAD 1 instead */
        if (devad == 0) {
            devad = 1;
        }
        ioerr += PHY_BUS_WRITE(pc, addr | (devad << 16), data);
        return ioerr;
    }

    /* Select device if non-zero */
    if (devad) {
        ioerr += PHY_BUS_WRITE(pc, 0x1f, 0xffde);
        ioerr += PHY_BUS_WRITE(pc, 0x1e, devad << 11);
    }

    /* Select block */
    blkaddr = (addr >> 8) & 0xffff;
#if PHY_XGS_IBLK_DBG_BIT15
    blkaddr |= 0x8000;
#endif
    ioerr += PHY_BUS_WRITE(pc, 0x1f, blkaddr);

    /* Write register value */
    ioerr += PHY_BUS_WRITE(pc, addr & 0x1f, data);

    /* Restore device type zero */
    if (devad) {
        ioerr += PHY_BUS_WRITE(pc, 0x1f, 0xffde);
        ioerr += PHY_BUS_WRITE(pc, 0x1e, 0);
    }

    return ioerr;
}
