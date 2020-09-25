/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <phy/phy_reg.h>
#include <phy/phy_brcm_shadow.h>
#include <phy/phy_brcm_1000x.h>
#include <phy/phy_xgs_iblk.h>
#include <phy/phy_xaui_iblk.h>
#include <phy/phy_aer_iblk.h>

int
phy_bus_write(phy_ctrl_t *pc, uint32_t reg, uint32_t data)
{
    /* Write raw PHY data */
    return pc->bus->write(pc->unit, PHY_CTRL_BUS_ADDR(pc), reg, data);
}
