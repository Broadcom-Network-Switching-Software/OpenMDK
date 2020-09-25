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
 *      ge_phy_adv_local_get
 * Purpose:     
 *      Set the advertised speed capabilities
 * Parameters:
 *      pc - PHY control structure
 *      *abil - abilities advertised
 * Returns:     
 *      CDK_E_xxx
 */
int 
ge_phy_adv_local_get(phy_ctrl_t *pc, uint32_t *abil)
{
    int ioerr = 0;
    uint32_t ctrl, adv;

    *abil = 0;

    /* 
     * Get advertised Gigabit capabilities.
     */
    ioerr += PHY_BUS_READ(pc, MII_GB_CTRL_REG, &ctrl);

    if (ctrl & MII_GB_CTRL_ADV_1000HD) *abil |= PHY_ABIL_1000MB_HD;
    if (ctrl & MII_GB_CTRL_ADV_1000FD) *abil |= PHY_ABIL_1000MB_FD;

    ioerr += PHY_BUS_READ(pc, MII_ANA_REG, &adv);

    /*
     * Get advertised 10/100 capabilities.
     */

    if (adv & MII_ANA_HD_10)  *abil |= PHY_ABIL_10MB_HD;
    if (adv & MII_ANA_FD_10)  *abil |= PHY_ABIL_10MB_FD;
    if (adv & MII_ANA_HD_100) *abil |= PHY_ABIL_100MB_HD;
    if (adv & MII_ANA_FD_100) *abil |= PHY_ABIL_100MB_FD;

    switch (adv & (MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE)) {
        case MII_ANA_PAUSE:
            *abil |= PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX;
            break;
        case MII_ANA_ASYM_PAUSE:
            *abil |= PHY_ABIL_PAUSE_TX;
            break;
        case MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE:
            *abil |= PHY_ABIL_PAUSE_RX;
            break;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

