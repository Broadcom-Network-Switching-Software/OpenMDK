/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <bmd/bmd_phy_ctrl.h>

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy.h>

int (*phy_init_cb)(phy_ctrl_t *pc);

/*
 * Register PHY init callback function
 */
int 
bmd_phy_init_cb_register(int (*init_cb)(phy_ctrl_t *pc))
{
    phy_init_cb = init_cb;

    return CDK_E_NONE;
}

#endif /* BMD_CONFIG_INCLUDE_PHY */
