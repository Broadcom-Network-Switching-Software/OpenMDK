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
 *      ge_phy_loopback_set
 * Purpose:     
 *      Set the current operating loopback.
 * Parameters:
 *      pc - PHY control structure
 *      enable - (OUT) non-zero indicates PHY loopback enabled
 * Returns:     
 *      CDK_E_xxx
 */
int
ge_phy_loopback_get(phy_ctrl_t *pc, int *enable)
{
    uint32_t ctrl;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);

    *enable = (ctrl & MII_CTRL_LE) != 0;

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
