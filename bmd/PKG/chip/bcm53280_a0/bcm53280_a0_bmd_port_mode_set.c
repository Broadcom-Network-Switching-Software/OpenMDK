#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53280_A0 == 1

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

#include <cdk/chip/bcm53280_a0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm53280_a0_bmd.h"
#include "bcm53280_a0_internal.h"

static void
_mii_write(int unit, int port, uint32_t reg, uint32_t val)
{
    cdk_robo_reg_port_write(unit, port, 0xa000 + (reg << 1), &val, 2);
}

int
bcm53280_a0_bmd_port_mode_set(int unit, int port, 
                              bmd_port_mode_t mode, uint32_t flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int mac_lb = (flags & BMD_PORT_MODE_F_MAC_LOOPBACK) ? 1 : 0;
    int phy_lb = (flags & BMD_PORT_MODE_F_PHY_LOOPBACK) ? 1 : 0;
    int duplex = 1;
    int speed = 1000;
    int sp_sel = SPDSTS_SPEED_1000;
    int mac_disabled;
    STS_OVERRIDE_Pr_t sts_override_p;
    STS_OVERRIDE_GPr_t sts_override_gp;
    G_PCTLr_t g_pctl;
    TH_PCTLr_t th_pctl;
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    if (port == CPIC_PORT) {
        return (mode == bmdPortModeAuto) ? CDK_E_NONE : CDK_E_PARAM;
    }

    switch (mode) {
    case bmdPortMode10hd:
    case bmdPortMode100hd:
    case bmdPortMode1000hd:
        duplex = 0;
        break;
    default:
        break;
    }
    switch (mode) {
    case bmdPortMode10fd:
    case bmdPortMode10hd:
        speed = 10;
        sp_sel = SPDSTS_SPEED_10;
        break;
    case bmdPortMode100fd:
    case bmdPortMode100hd:
        speed = 100;
        sp_sel = SPDSTS_SPEED_100;
        break;
    case bmdPortMode1000fd:
    case bmdPortMode1000hd:
        if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_FE) {
            return CDK_E_PARAM;
        }
        break;
    case bmdPortModeAuto:
        if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_FE) {
            speed = 100;
            sp_sel = SPDSTS_SPEED_100;
        }
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

    /* Work around for 10 Mbps Rx problem - see errata for details */
    if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_FE && speed == 10) {
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_autoneg_set(unit, port, 1);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_speed_set(unit, port, 100);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_speed_set(unit, port, 10);
        }
        _mii_write(unit, port, 0x1f, 0x008b);
        _mii_write(unit, port, 0x14, 0x4000);
        _mii_write(unit, port, 0x14, 0x0000);
        _mii_write(unit, port, 0x1f, 0x000b);
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_autoneg_set(unit, port, 0);
        }
    }

    /* Configure the MAC */
    if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_FE) {
        ioerr += READ_STS_OVERRIDE_Pr(unit, port, &sts_override_p);
        STS_OVERRIDE_Pr_SW_OVERRIDEf_SET(sts_override_p, 1);
        STS_OVERRIDE_Pr_SPEEDf_SET(sts_override_p, sp_sel);
        STS_OVERRIDE_Pr_DUPLX_MODEf_SET(sts_override_p, duplex);
        ioerr += WRITE_STS_OVERRIDE_Pr(unit, port, sts_override_p);
    } else {
        ioerr += READ_STS_OVERRIDE_GPr(unit, port, &sts_override_gp);
        STS_OVERRIDE_GPr_SW_OVERRIDEf_SET(sts_override_gp, 1);
        STS_OVERRIDE_GPr_SPEEDf_SET(sts_override_gp, sp_sel);
        STS_OVERRIDE_GPr_DUPLX_MODEf_SET(sts_override_gp, duplex);
        ioerr += WRITE_STS_OVERRIDE_GPr(unit, port, sts_override_gp);
    }

    /* Get MAC state */
    if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_FE) {
        ioerr += READ_TH_PCTLr(unit, port, &th_pctl);
        mac_disabled = TH_PCTLr_RX_DISf_GET(th_pctl);
    } else {    
        ioerr += READ_G_PCTLr(unit, port, &g_pctl);
        mac_disabled = G_PCTLr_RX_DISf_GET(g_pctl);
    }
    
    if (mode == bmdPortModeDisabled) {
        /* Disable MAC if enabled */
        if (!mac_disabled) {
            if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_FE) {
                TH_PCTLr_RX_DISf_SET(th_pctl, 1);
                TH_PCTLr_TX_DISf_SET(th_pctl, 1);
                ioerr += WRITE_TH_PCTLr(unit, port, th_pctl);
            } else { 
                G_PCTLr_RX_DISf_SET(g_pctl, 1);
                G_PCTLr_TX_DISf_SET(g_pctl, 1);
                ioerr += WRITE_G_PCTLr(unit, port, g_pctl);
            }
        }
        BMD_PORT_STATUS_CLR(unit, port, BMD_PST_LINK_UP);
        BMD_PORT_STATUS_SET(unit, port, BMD_PST_FORCE_LINK);
    } else {
        /* Enable MAC if disabled */
        if (mac_disabled) {
            if (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_FE) {
                TH_PCTLr_RX_DISf_SET(th_pctl, 0);
                TH_PCTLr_TX_DISf_SET(th_pctl, 0);
                ioerr += WRITE_TH_PCTLr(unit, port, th_pctl);
            } else {
                G_PCTLr_RX_DISf_SET(g_pctl, 0);
                G_PCTLr_TX_DISf_SET(g_pctl, 0);
                ioerr += WRITE_G_PCTLr(unit, port, g_pctl);
            }
        }
        
        /* Update link status */
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
#endif /* CDK_CONFIG_INCLUDE_BCM53280_A0 */
