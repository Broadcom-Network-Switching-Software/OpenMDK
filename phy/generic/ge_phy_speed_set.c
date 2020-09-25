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
 *      ge_phy_speed_set
 * Purpose:     
 *      Set the current operating speed.
 * Parameters:
 *      pc - PHY control structure
 *      speed - new link speed
 * Returns:     
 *      CDK_E_xxx
 * Notes: 
 *      Only takes effect if autonegotiation is disabled.
 */
int
ge_phy_speed_set(phy_ctrl_t *pc, uint32_t speed)
{
    uint32_t ctrl;
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    if (speed == 0) {
        return CDK_E_NONE;
    }

    ioerr += PHY_BUS_READ(pc, MII_CTRL_REG, &ctrl);

    ctrl &= ~(MII_CTRL_SS_LSB | MII_CTRL_SS_MSB);
    switch(speed) {
    case 10:
	ctrl |= MII_CTRL_SS_10;
	break;
    case 100:
	ctrl |= MII_CTRL_SS_100;
	break;
    case 1000:	
	ctrl |= MII_CTRL_SS_1000;
	break;
    default:
	return CDK_E_PARAM;
    }

    /* Ensure that loopback is disabled while changing speed */
    ioerr += PHY_BUS_WRITE(pc, MII_CTRL_REG, ctrl & ~(MII_CTRL_LE));

    /* Restore loopback setting if needed */
    if (ctrl & MII_CTRL_LE) {
        ioerr += PHY_BUS_WRITE(pc, MII_CTRL_REG, ctrl);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
