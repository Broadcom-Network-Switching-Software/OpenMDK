/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * This function accepts a 32-bit generic PHY register address
 * and performs a register write accoding to the access method
 * contained in the PHY register address.
 */

#include <phy/phy_reg.h>
#include <phy/phy_brcm_shadow.h>
#include <phy/phy_brcm_1000x.h>
#include <phy/phy_xgs_iblk.h>
#include <phy/phy_xaui_iblk.h>
#include <phy/phy_aer_iblk.h>
#include <phy/phy_brcm_xe.h>
#include <phy/phy_tsc_iblk.h>
#include <phy/phy_brcm_rdb.h>

int
phy_reg_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data)
{
    switch (PHY_REG_ACCESS_METHOD(addr)) {
    case PHY_REG_ACC_RAW:
        return PHY_BUS_WRITE(pc, addr, data);
    case PHY_REG_ACC_BRCM_SHADOW:
        return phy_brcm_shadow_write(pc, addr, data);
    case PHY_REG_ACC_BRCM_1000X:
        return phy_brcm_1000x_write(pc, addr, data);
    case PHY_REG_ACC_XGS_IBLK:
        return phy_xgs_iblk_write(pc, addr, data);
    case PHY_REG_ACC_XAUI_IBLK:
        return phy_xaui_iblk_write(pc, addr, data);
    case PHY_REG_ACC_AER_IBLK:
        return phy_aer_iblk_write(pc, addr, data);
    case PHY_REG_ACC_BRCM_XE:
        return phy_brcm_xe_write(pc, addr, data);
    case PHY_REG_ACC_TSC_IBLK:
        return phy_tsc_iblk_write(pc, addr, data);
    case PHY_REG_ACC_BRCM_RDB:
        return phy_brcm_rdb_write(pc, addr, data);
    default:
        break;
    }

    /* Unknown access method */
    return -1;
}
