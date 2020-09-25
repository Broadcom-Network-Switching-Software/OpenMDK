/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Generic PHY driver.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>

/*
 * Function:    
 *      ge_phy_autoneg_set
 * Purpose:     
 *      Set the current operating an.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - non-zero enables autoneg, zero disables autoneg
 * Returns:     
 *      CDK_E_xxx
 */
int
ge_phy_autoneg_set(phy_ctrl_t *pc, int autoneg)
{
    uint32_t ctrl;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);

    if (autoneg) {
        /* Enable and restart autonegotiation (self-clearing bit) */
        ctrl |= MII_CTRL_AE | MII_CTRL_RAN;
    } else {
        ctrl &= ~MII_CTRL_AE;
    }

    ioerr += PHY_BUS_WRITE(pc, MII_CTRL_REG, ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
