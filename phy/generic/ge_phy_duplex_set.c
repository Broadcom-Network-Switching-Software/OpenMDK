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
 *      ge_phy_duplex_set
 * Purpose:     
 *      Set the current operating duplex.
 * Parameters:
 *      pc - PHY control structure
 *      duplex - non-zero indicates full duplex, zero indicates half
 * Returns:     
 *      CDK_E_xxx
 * Notes: 
 *      Only takes effect if autonegotiation is disabled.
 */
int
ge_phy_duplex_set(phy_ctrl_t *pc, int duplex)
{
    int ioerr = 0;
    uint32_t ctrl;

    PHY_CTRL_CHECK(pc);

    ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);

    if (duplex) {
	ctrl |= MII_CTRL_FD;
    } else {
	ctrl &= ~MII_CTRL_FD;
    }

    ioerr += PHY_BUS_WRITE(pc, MII_CTRL_REG, ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
