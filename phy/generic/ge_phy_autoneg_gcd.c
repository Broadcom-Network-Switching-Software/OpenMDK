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
 *      ge_phy_autoneg_gcd (greatest common denominator).
 * Purpose:    
 *      Determine the current greatest common denominator between 
 *      two ends of a link.
 * Parameters:
 *      pc - PHY control structure
 *      speed - (OUT) greatest common speed
 *      duplex - (OUT) greatest common duplex
 * Returns:    
 *      CDK_E_xxx.
 */
int
ge_phy_autoneg_gcd(phy_ctrl_t *pc, uint32_t *speed, int *duplex)
{
    int t_speed, t_duplex;
    int ioerr = 0;
    uint32_t mii_ana, mii_anp, mii_stat;
    uint32_t mii_gb_stat, mii_esr, mii_gb_ctrl;

    PHY_CTRL_CHECK(pc);

    mii_gb_stat = 0;
    mii_gb_ctrl = 0;

    ioerr += PHY_BUS_READ(pc, MII_ANA_REG, &mii_ana);
    ioerr += PHY_BUS_READ(pc, MII_ANP_REG, &mii_anp);
    ioerr += PHY_BUS_READ(pc, MII_STAT_REG, &mii_stat);

    if (mii_stat & MII_STAT_ES) {    /* Supports extended status */
        /*
         * If the PHY supports extended status, check if it is 1000MB
         * capable.  If it is, check the 1000Base status register to see
         * if 1000MB negotiated.
         */
        ioerr += PHY_BUS_READ(pc, MII_ESR_REG, &mii_esr);

        if (mii_esr & (MII_ESR_1000_X_FD | MII_ESR_1000_X_HD | 
                       MII_ESR_1000_T_FD | MII_ESR_1000_T_HD)) {
            ioerr += PHY_BUS_READ(pc, MII_GB_STAT_REG, &mii_gb_stat);
            ioerr += PHY_BUS_READ(pc, MII_GB_CTRL_REG, &mii_gb_ctrl);
        }
    }

    /*
     * At this point, if we did not see Gig status, one of mii_gb_stat or 
     * mii_gb_ctrl will be 0. This will cause the first 2 cases below to 
     * fail and fall into the default 10/100 cases.
     */

    mii_ana &= mii_anp;

    if ((mii_gb_ctrl & MII_GB_CTRL_ADV_1000FD) &&
        (mii_gb_stat & MII_GB_STAT_LP_1000FD)) {
        t_speed  = 1000;
        t_duplex = 1;
    } else if ((mii_gb_ctrl & MII_GB_CTRL_ADV_1000HD) &&
               (mii_gb_stat & MII_GB_STAT_LP_1000HD)) {
        t_speed  = 1000;
        t_duplex = 0;
    } else if (mii_ana & MII_ANA_FD_100) {         /* [a] */
        t_speed = 100;
        t_duplex = 1;
    } else if (mii_ana & MII_ANA_T4) {            /* [b] */
        t_speed = 100;
        t_duplex = 0;
    } else if (mii_ana & MII_ANA_HD_100) {        /* [c] */
        t_speed = 100;
        t_duplex = 0;
    } else if (mii_ana & MII_ANA_FD_10) {        /* [d] */
        t_speed = 10;
        t_duplex = 1 ;
    } else if (mii_ana & MII_ANA_HD_10) {        /* [e] */
        t_speed = 10;
        t_duplex = 0;
    } else {
        return CDK_E_FAIL;
    }

    if (speed) {
        *speed  = t_speed;
    }
    if (duplex) {
        *duplex = t_duplex;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
