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
 *      ge_phy_autoneg_get
 * Purpose:     
 *      Get the current autonegotiation state.
 * Parameters:
 *      pc - PHY control structure
 *      autoneg - (OUT) non-zero means that autonegotiation is enabled
 *      autoneg_done - (OUT) non-zero means that autonegotiation is complete
 * Returns:     
 *      CDK_E_xxx
 */
int
ge_phy_autoneg_get(phy_ctrl_t *pc, int *autoneg)
{
    uint32_t ctrl;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);
    *autoneg = (ctrl & MII_CTRL_AE) ? 1 : 0;

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
