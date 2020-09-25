#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53101_A0 == 1

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

#include <cdk/chip/bcm53101_a0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm53101_a0_bmd.h"
#include "bcm53101_a0_internal.h"

int
bcm53101_a0_bmd_port_mode_set(int unit, int port, 
                              bmd_port_mode_t mode, uint32_t flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int mac_lb = (flags & BMD_PORT_MODE_F_MAC_LOOPBACK) ? 1 : 0;
    int phy_lb = (flags & BMD_PORT_MODE_F_PHY_LOOPBACK) ? 1 : 0;
    int duplex = 1;
    int speed = 10;
    int sp_sel = SPDSTS_SPEED_10;
    int mac_disabled;
    STS_OVERRIDE_GMIIPr_t sts_override_gp;
    G_PCTLr_t g_pctl;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    if (port == CPIC_PORT) {
        return (mode == bmdPortModeAuto) ? CDK_E_NONE : CDK_E_PARAM;
    }

    switch (mode) {
    case bmdPortMode10hd:
    case bmdPortMode100hd:
        duplex = 0;
        break;
    default:
        break;
    }
    switch (mode) {
    case bmdPortMode10fd:
    case bmdPortMode10hd:
        break;
    case bmdPortMode100fd:
    case bmdPortMode100hd:
        speed = 100;
        sp_sel = SPDSTS_SPEED_100;
        break;
    case bmdPortModeAuto:
        break;
    case bmdPortModeDisabled:
        break;
    default:
        return CDK_E_PARAM;
    }

    /* MAC loopback unsupported */
    if (mac_lb) {
        return CDK_E_PARAM;
    }

    /* Update PHYs before MAC */
    if (CDK_SUCCESS(rv)) {
        rv = bmd_port_mode_to_phy(unit, port, mode, flags, speed, duplex);
    }

    /* Workaround for PHY loopback */
    if (mode != bmdPortModeAuto && phy_lb) {
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_loopback_set(unit, port, 1);
        }
    }

    /* Configure the MAC */
    ioerr += READ_STS_OVERRIDE_GMIIPr(unit, port, &sts_override_gp);
    STS_OVERRIDE_GMIIPr_SW_OVERRIDEf_SET(sts_override_gp, 1);
    STS_OVERRIDE_GMIIPr_SPEEDf_SET(sts_override_gp, sp_sel);
    STS_OVERRIDE_GMIIPr_DUPLX_MODEf_SET(sts_override_gp, duplex);
    ioerr += WRITE_STS_OVERRIDE_GMIIPr(unit, port, sts_override_gp);

    /* Get MAC state */
    ioerr += READ_G_PCTLr(unit, port, &g_pctl);
    mac_disabled = G_PCTLr_RX_DISf_GET(g_pctl);
    if (mode == bmdPortModeDisabled) {
        /* Disable MAC if enabled */
        if (!mac_disabled) {
            G_PCTLr_RX_DISf_SET(g_pctl, 1);
            G_PCTLr_TX_DISf_SET(g_pctl, 1);
            ioerr += WRITE_G_PCTLr(unit, port, g_pctl);
        }
        BMD_PORT_STATUS_CLR(unit, port, BMD_PST_LINK_UP);
        BMD_PORT_STATUS_SET(unit, port, BMD_PST_FORCE_LINK);
    } else {
        /* Enable MAC if disabled */
        if (mac_disabled) {
            G_PCTLr_RX_DISf_SET(g_pctl, 0);
            G_PCTLr_TX_DISf_SET(g_pctl, 0);
            ioerr += WRITE_G_PCTLr(unit, port, g_pctl);
        }
        if (phy_lb) {
            BMD_PORT_STATUS_SET(unit, port, BMD_PST_LINK_UP | BMD_PST_FORCE_LINK);
        } else {
            BMD_PORT_STATUS_CLR(unit, port, BMD_PST_FORCE_LINK);
        }
    }
    /* Force a link change event */
    BMD_PORT_STATUS_SET(unit, port, BMD_PST_FORCE_UPDATE);
    
    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53101_A0 */
