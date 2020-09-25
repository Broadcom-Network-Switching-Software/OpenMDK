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
 *	bmd_port_mode_from_phy
 * Purpose:
 *	Determine BMD port mode based on PHY status/configuration.
 * Parameters:
 *	unit - BMD device
 *	port - port number
 *	mode - (OUT) BMD port mode
 *	flags - (OUT) BMD port mode flags
 * Returns:
 *      CDK_XXX
 * Notes:
 *      This is a helper function for the bmd_port_mode_get API.
 *      The flags parameter is assumed to have been initialized
 *      by the caller.
 */
int
bmd_port_mode_from_phy(int unit, int port,
                       bmd_port_mode_t *mode, uint32_t *flags)
{
    int rv = CDK_E_NONE;
    int an, lb, duplex, eee_mode;
    uint32_t speed;

    rv = bmd_phy_loopback_get(unit, port, &lb);
    if (CDK_SUCCESS(rv) && lb) {
        *flags |= BMD_PORT_MODE_F_PHY_LOOPBACK;
    }

    rv = bmd_phy_remote_loopback_get(unit, port, &lb);
    if (CDK_SUCCESS(rv) && lb) {
        *flags |= BMD_PORT_MODE_F_REMOTE_LOOPBACK;
    }

    rv = bmd_phy_autoneg_get(unit, port, &an);
    if (CDK_SUCCESS(rv) && an) {
        *flags |= BMD_PORT_MODE_F_AUTONEG;
    }

    rv = bmd_phy_speed_get(unit, port, &speed);
    if (CDK_SUCCESS(rv)) {
        if (an && (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) == 0) {
            speed = 0;
        }
        rv = bmd_phy_duplex_get(unit, port, &duplex);
        if (CDK_SUCCESS(rv)) {
            rv = bmd_port_mode_from_speed_duplex(speed, duplex, mode);
        }
    }

    rv = bmd_phy_eee_get(unit, port, &eee_mode);
    if (CDK_SUCCESS(rv)) {
        if (eee_mode == BMD_PHY_M_EEE_AUTO) {
            *flags |= BMD_PORT_MODE_F_AUTOGREEEN;
        } else if (eee_mode == BMD_PHY_M_EEE_802_3) {
            *flags |= BMD_PORT_MODE_F_EEE;
        }
    }

    return rv;
}
