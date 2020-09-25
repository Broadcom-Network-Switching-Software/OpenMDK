#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53084_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/chip/bcm53084_a0_defs.h>

#include <cdk/cdk_debug.h>

#include "bcm53084_a0_bmd.h"
#include "bcm53084_a0_internal.h"\

int 
bcm53084_a0_bmd_port_mode_set(
    int unit, int port, bmd_port_mode_t mode, uint32_t flags)
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

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    if (port == 5 || port == CPIC_PORT) {
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
        LOW_POWER_CTRLr_t low_power_ctrl;
        EEE_EN_CTRLr_t eee_en_ctrl;
        int eee_en;
        cdk_pbmp_t eee_en_pbmp;
        cdk_pbmp_t pbmp;

        CDK_ROBO_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPIC, &pbmp);
		if (CDK_PBMP_MEMBER(pbmp, port)) {
            /* Configure EEE */
            ioerr += READ_EEE_EN_CTRLr(unit, &eee_en_ctrl);
            eee_en = EEE_EN_CTRLr_EN_EEEf_GET(eee_en_ctrl);
            CDK_PBMP_WORD_SET(eee_en_pbmp, 0, eee_en);
            CDK_PBMP_PORT_REMOVE(eee_en_pbmp, port);
            eee_en = CDK_PBMP_WORD_GET(eee_en_pbmp, 0);
            EEE_EN_CTRLr_EN_EEEf_SET(eee_en_ctrl, eee_en);
            ioerr += WRITE_EEE_EN_CTRLr(unit, eee_en_ctrl);
    
            if (flags & BMD_PORT_MODE_F_EEE) {
                /* If it is MAC low power mode, do not allow to enable EEE */
                ioerr += READ_LOW_POWER_CTRLr(unit, &low_power_ctrl);
                if (!LOW_POWER_CTRLr_EN_LOW_POWERf_GET(low_power_ctrl)) {
                    /* Enable IEEE 802.3az EEE */
                    CDK_PBMP_PORT_ADD(eee_en_pbmp, port);
                    eee_en = CDK_PBMP_WORD_GET(eee_en_pbmp, 0);
                    EEE_EN_CTRLr_EN_EEEf_SET(eee_en_ctrl, eee_en);
                    ioerr += WRITE_EEE_EN_CTRLr(unit, eee_en_ctrl);							
                } else {
                    CDK_WARN(("Enable EEE in Low Power Mode is not allowed.\n"));
    		    }
            }
        }

        /* Enable MAC if disabled */
        if (mac_disabled) {
            G_PCTLr_RX_DISf_SET(g_pctl, 0);
            G_PCTLr_TX_DISf_SET(g_pctl, 0);
            ioerr += WRITE_G_PCTLr(unit, port, g_pctl);
        } 
        
        if (phy_lb) {
            BMD_PORT_STATUS_SET(unit, port, BMD_PST_LINK_UP | 
				BMD_PST_FORCE_LINK);
        } else {
            BMD_PORT_STATUS_CLR(unit, port, BMD_PST_FORCE_LINK);
        }
    }

    if (CDK_SUCCESS(rv)) {
        rv = bmd_phy_loopback_set(unit, port, phy_lb);
    }

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53084_A0 */
