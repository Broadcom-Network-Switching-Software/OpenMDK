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
 *      ge_phy_adv_local_set
 * Purpose:     
 *      Set the advertised speed capabilities
 * Parameters:
 *      pc - PHY control structure
 *      abil - abilities to be advertised
 * Returns:     
 *      CDK_E_xxx
 */
int 
ge_phy_adv_local_set(phy_ctrl_t *pc, uint32_t abil)
{
    int ioerr = 0;
    uint32_t ctrl, adv;

    /* 
     * Set advertised Gigabit capabilities.
     */
    ioerr += PHY_BUS_READ(pc, MII_GB_CTRL_REG, &ctrl);

    ctrl &= ~(MII_GB_CTRL_ADV_1000HD | MII_GB_CTRL_ADV_1000FD);
    if (abil & PHY_ABIL_1000MB_HD) ctrl |= MII_GB_CTRL_ADV_1000HD;
    if (abil & PHY_ABIL_1000MB_FD) ctrl |= MII_GB_CTRL_ADV_1000FD;

    ioerr += PHY_BUS_WRITE(pc, MII_GB_CTRL_REG, ctrl);

    /*
     * Set advertised 10/100 capabilities.
     */
    adv = MII_ANA_ASF_802_3;
    if (abil & PHY_ABIL_10MB_HD)  adv |= MII_ANA_HD_10;
    if (abil & PHY_ABIL_10MB_FD)  adv |= MII_ANA_FD_10;
    if (abil & PHY_ABIL_100MB_HD) adv |= MII_ANA_HD_100;
    if (abil & PHY_ABIL_100MB_FD) adv |= MII_ANA_FD_100;

    if ((abil & PHY_ABIL_PAUSE) == PHY_ABIL_PAUSE) {
        /* Advertise symmetric pause */
        adv |= MII_ANA_PAUSE;
    } else {
        /*
         * For Asymmetric pause, 
         *   if (Bit 10)
         *       then pause frames flow toward the transceiver
         *       else pause frames flow toward link partner.
         */
        if (abil & PHY_ABIL_PAUSE_TX) {
            adv |= MII_ANA_ASYM_PAUSE;
        } else if (abil & PHY_ABIL_PAUSE_RX) {
            adv |= MII_ANA_ASYM_PAUSE;
            adv |= MII_ANA_PAUSE;
        }
    }

    ioerr += PHY_BUS_WRITE(pc, MII_ANA_REG, adv);

    /* Restart auto-neg if enabled */

    ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);

    if (ctrl & MII_CTRL_AE) {
        ctrl |= MII_CTRL_RAN;
        ioerr += PHY_BUS_WRITE(pc, MII_CTRL_REG, ctrl);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

