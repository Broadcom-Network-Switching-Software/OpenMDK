/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/cdk_assert.h>

/*
 * Function:
 *	bmd_port_mode_to_phy
 * Purpose:
 *	Apply PHY settings according to BMD port mode and flags.
 * Parameters:
 *	unit - BMD device
 *	port - port number
 *	mode - BMD port mode
 *	flags - BMD port mode flags
 *	speed - speed resolved from BMD port mode
 *	duplex - duplex resolved from BMD port mode
 * Returns:
 *      CDK_XXX
 * Notes:
 *      This is a helper function for the bmd_port_mode_set API.
 *      The speed and duplex parameters are provided since they
 *      usually have been derived by the bmd_port_mode_set API
 *      prior to calling this function.
 */
int
bmd_port_mode_to_phy(int unit, int port, bmd_port_mode_t mode,
                     uint32_t flags, uint32_t speed, int duplex)
{
    int rv = CDK_E_NONE;
    int phy_lb = (flags & BMD_PORT_MODE_F_PHY_LOOPBACK) ? 1 : 0;
    int rem_lb = (flags & BMD_PORT_MODE_F_REMOTE_LOOPBACK) ? 1 : 0;
    int autoneg = (mode == bmdPortModeAuto) ? 1 : 0;
    int cur_phy_lb = 0;

    if ((flags & BMD_PORT_MODE_F_INTERNAL) == 0) {

        /* Force a link change event */
        BMD_PORT_STATUS_SET(unit, port, BMD_PST_FORCE_UPDATE);

        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_autoneg_set(unit, port, autoneg);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_loopback_get(unit, port, &cur_phy_lb);
        }
        if (phy_lb != cur_phy_lb) {
            if (CDK_SUCCESS(rv)) {
                rv = bmd_phy_loopback_set(unit, port, phy_lb);
            }
        }
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_remote_loopback_set(unit, port, rem_lb);
        }
        if (CDK_SUCCESS(rv)) {
            int eee_mode = BMD_PHY_M_EEE_OFF;
            if (flags & BMD_PORT_MODE_F_AUTOGREEEN) {
                /* Enable AutoGrEEEn mode */
                eee_mode = BMD_PHY_M_EEE_AUTO;
            } else if (flags & BMD_PORT_MODE_F_EEE) {
                /* Enable native EEE mode */
                eee_mode = BMD_PHY_M_EEE_802_3;
            }
            rv = bmd_phy_eee_set(unit, port, eee_mode);
        }
    }
    /* Read from PHY in case autoneg is not supported */
    if (CDK_SUCCESS(rv) && autoneg) {
        rv = bmd_phy_autoneg_get(unit, port, &autoneg);
    }
    if (!autoneg) {
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_speed_set(unit, port, speed);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_duplex_set(unit, port, duplex);
        }
    }

    return rv;
}
