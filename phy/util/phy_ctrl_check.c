/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Generic PHY driver.
 */

#include <phy/phy.h>

int
phy_ctrl_check(phy_ctrl_t *pc)
{
    if (pc == 0 ||
        pc->bus == 0 ||
        pc->bus->read == 0 ||
        pc->bus->write == 0) {
        return -1;
    }
    return 0;
}
