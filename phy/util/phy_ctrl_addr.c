/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <phy/phy.h>

uint32_t
phy_ctrl_addr(phy_ctrl_t *pc, int adjust)
{
    uint32_t phy_addr;

    /* Initialize PHY address if necessary */
    if ((PHY_CTRL_FLAGS(pc) & PHY_F_ADDR_VALID) == 0) {
        if (pc->bus->phy_addr != NULL) {
            pc->addr = pc->bus->phy_addr(pc->port);
        }
        PHY_CTRL_FLAGS(pc) |= PHY_F_ADDR_VALID;
    }

    /* Default address */
    phy_addr = pc->addr;

    /* Adjust according to selected instance */
    if (adjust) {
        phy_addr += PHY_CTRL_ADDR_OFFSET(pc);
    }

    return phy_addr;
}
