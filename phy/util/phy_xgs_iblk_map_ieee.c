/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Use for mapping IEEE register before calling generic PHY functions.
 * This is required for newer combo cores.
 */

#include <phy/phy_xgs_iblk.h>

int
phy_xgs_iblk_map_ieee(phy_ctrl_t *pc)
{
    PHY_CTRL_CHECK(pc);

    return PHY_BUS_WRITE(pc, 0x1f, 0);
}

