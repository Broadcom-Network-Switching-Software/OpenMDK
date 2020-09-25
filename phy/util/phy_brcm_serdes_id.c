/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Read PHY ID from internal SerDes PHY.
 *
 * For block mapped Broadcom SerDes PHYs the PHY ID is usually
 * only visible on page zero, however we want avoid writing any
 * register unless we know we probe a Broadcom SerDes PHY.
 */

#include <phy/phy_brcm_serdes_id.h>

int
phy_brcm_serdes_id(phy_ctrl_t *pc, uint32_t *phyid0, uint32_t *phyid1)
{
    int ioerr = 0;

    PHY_CTRL_CHECK(pc);

    if (phyid0 == NULL || phyid1 == 0) {
        return -1;
    }

    /* Use direct bus access for reading PHY IDs */
    ioerr += PHY_BUS_READ(pc, MII_PHY_ID0_REG, phyid0);
    ioerr += PHY_BUS_READ(pc, MII_PHY_ID1_REG, phyid1);

    /* If not a valid PHY ID, try resetting the block register */
    if ((*phyid0 | *phyid1) == 0) {
        ioerr += PHY_BUS_WRITE(pc, 0x1f, 0);
        ioerr += PHY_BUS_READ(pc, MII_PHY_ID0_REG, phyid0);
        ioerr += PHY_BUS_READ(pc, MII_PHY_ID1_REG, phyid1);
    }

    return ioerr;
}
