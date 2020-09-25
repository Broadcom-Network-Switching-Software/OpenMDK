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

#define PHY_RESET_POLL_MAX      10

/*
 * Function:    
 *      ge_phy_reset
 * Purpose:     
 *      Reset PHY.
 * Parameters:
 *      pc - PHY control structure
 * Returns:     
 *      CDK_E_xxx
 */
int
ge_phy_reset(phy_ctrl_t *pc)
{
    uint32_t ctrl;
    int cnt;
    int ioerr = 0;
    int rv = CDK_E_NONE;

    PHY_CTRL_CHECK(pc);

    /* Reset PHY */
    ioerr += PHY_BUS_WRITE(pc, MII_CTRL_REG, MII_CTRL_RESET);

    /* Wait for reset completion */
    for (cnt = 0; cnt < PHY_RESET_POLL_MAX; cnt++) {
        ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);
        if ((ctrl & MII_CTRL_RESET) == 0) {
            break;
        }
    }
    if (cnt >= PHY_RESET_POLL_MAX) {
        rv = CDK_E_TIMEOUT;
    }
 
    return ioerr ? CDK_E_IO : rv;
}
