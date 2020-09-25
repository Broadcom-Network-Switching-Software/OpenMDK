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
 *      phy_speed_get
 * Purpose:     
 *      Get the current operating speed. If autoneg is enabled, then
 *      operating mode is returned, otherwise forced mode is returned.
 * Parameters:
 *      pc - PHY control structure
 *      speed - (OUT) current link speed
 * Returns:     
 *      CDK_E_xxx
 * Notes: 
 *      Returns a speed of 0 if autonegotiation is not complete.
 */
int
ge_phy_speed_get(phy_ctrl_t *pc, uint32_t *speed)
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
            *speed = 0;
            return CDK_E_NONE;
        }
        return ge_phy_autoneg_gcd(pc, speed, NULL);
    } else {
        /* 
         * Auto-negotiation disabled.
         * Simply pick up the values we force in CTRL register.
         */
        switch (MII_CTRL_SS(ctrl)) {
        case MII_CTRL_SS_10:
            *speed = 10;
            break;
        case MII_CTRL_SS_100:
            *speed = 100;
            break;
        case MII_CTRL_SS_1000:
            *speed = 1000;
            break;
        default:
            return CDK_E_UNAVAIL;
        }
    }

    return CDK_E_NONE;
}
