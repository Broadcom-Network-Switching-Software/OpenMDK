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
 *      ge_phy_duplex_get
 * Purpose:     
 *      Get the current operating duplex mode. If autoneg is enabled, 
 *      then operating mode is returned, otherwise forced mode is returned.
 * Parameters:
 *      pc - PHY control structure
 *      duplex - (OUT) non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 * Notes: 
 *      Returns a half duplex if autonegotiation is not complete.
 */
int
ge_phy_duplex_get(phy_ctrl_t *pc, int *duplex)
{
    int ioerr = 0;
    uint32_t ctrl, stat;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);
    ioerr += PHY_BUS_READ(pc, MII_STAT_REG, &stat);

    if (ioerr) {
        return CDK_E_IO;
    }

    if (ctrl & MII_CTRL_AE) {
        /* Auto-negotiation enabled */
        if (!(stat & MII_STAT_AN_DONE)) {
            /* Auto-neg NOT complete */
            *duplex = 0;
            return CDK_E_NONE;
        }
        return ge_phy_autoneg_gcd(pc, NULL, duplex);
    } else {
        /* Auto-negotiation disabled */
        *duplex = (ctrl & MII_CTRL_FD) ? 1 : 0;
    }

    return CDK_E_NONE;
}

