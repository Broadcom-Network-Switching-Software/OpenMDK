#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53125_A0 == 1

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

#include <cdk/chip/bcm53125_a0_defs.h>
#include <cdk/arch/robo_chip.h>

#include "bcm53125_a0_bmd.h"
#include "bcm53125_a0_internal.h"

int
bcm53125_a0_bmd_port_mode_set(int unit, int port, 
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
    STS_OVERRIDE_GMIIPr_t sts_override_gp;
    G_PCTLr_t g_pctl;
    IMP_CTLr_t impstl;
    int i;
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

    bcm53125_a0_int_cpu_lock(unit, 1);
    
    /* Update PHYs before MAC */
    if (CDK_SUCCESS(rv)) {
        rv = bmd_port_mode_to_phy(unit, port, mode, flags, speed, duplex);
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

    if (CDK_SUCCESS(rv)) {
        rv = bmd_phy_loopback_set(unit, port, phy_lb);
    }
        
    /* It takes time to wait for link up */
    for (i = 0; i < 100; i++) {
        ioerr += READ_IMP_CTLr(unit, &impstl);
        if (IMP_CTLr_RX_BCST_ENf_GET(impstl) == 0) {
            break;
        }
    }

    ioerr += READ_G_PCTLr(unit, port, &g_pctl);
    G_PCTLr_RX_DISf_SET(g_pctl, 0);
    G_PCTLr_TX_DISf_SET(g_pctl, 0);
    ioerr += WRITE_G_PCTLr(unit, port, g_pctl);

    READ_IMP_CTLr(unit, &impstl);
    IMP_CTLr_RX_UCST_ENf_SET(impstl, 1);
    IMP_CTLr_RX_MCST_ENf_SET(impstl, 1);
    IMP_CTLr_RX_BCST_ENf_SET(impstl, 1);
    IMP_CTLr_RX_DISf_SET(impstl, 0);
    IMP_CTLr_TX_DISf_SET(impstl, 0);
    WRITE_IMP_CTLr(unit, impstl);

    bcm53125_a0_int_cpu_lock(unit, 0);
    
    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53125_A0 */
