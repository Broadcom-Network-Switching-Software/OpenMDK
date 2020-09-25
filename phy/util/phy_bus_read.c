/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <phy/phy.h>

int
phy_bus_read(phy_ctrl_t *pc, uint32_t reg, uint32_t *data)
{
    /* Read raw PHY data */
    return pc->bus->read(pc->unit, PHY_CTRL_BUS_ADDR(pc), reg, data);
}
