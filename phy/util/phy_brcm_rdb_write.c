/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Access function for shadowed registers on Broadcom Gigabit PHYs.
 */

#include <phy/phy_brcm_rdb.h>


int
phy_brcm_rdb_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data)
{
    int ioerr = 0;
    uint32_t regaddr = addr & 0xffff;

    ioerr += PHY_BUS_WRITE(pc, 0x17, 0x0f7e);
    ioerr += PHY_BUS_WRITE(pc, 0x15, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, 0x1e, regaddr);
    ioerr += PHY_BUS_WRITE(pc, 0x1f, data);
    ioerr += PHY_BUS_WRITE(pc, 0x1e, 0x0087);
    ioerr += PHY_BUS_WRITE(pc, 0x1f, 0x8000);

    return ioerr;
}
