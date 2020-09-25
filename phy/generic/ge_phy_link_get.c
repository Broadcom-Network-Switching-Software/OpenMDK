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
 *      ge_phy_link_get
 * Purpose:     
 *      Get the current link state.
 * Parameters:
 *      pc - PHY control structure
 *      link - (OUT) non-zero means the link is up
 *      autoneg_done - (OUT) non-zero means the auto-negotiation is complete
 * Returns:     
 *      CDK_E_xxx
 */
int
ge_phy_link_get(phy_ctrl_t *pc, int *link, int *autoneg_done)
{
    uint32_t stat;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, MII_STAT_REG, &stat);

    if (link) {
        *link = (stat & MII_STAT_LA) ? 1 : 0;
    }
    if (autoneg_done) {
        *autoneg_done = (stat & MII_STAT_AN_DONE) ? 1 : 0;
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
