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

#include "bcm53084_a0_bmd.h"
#include "bcm53084_a0_internal.h"

int 
bcm53084_a0_bmd_port_mode_get(
    int unit, int port, bmd_port_mode_t *mode, uint32_t *flags)
{
    int ioerr = 0;
    G_PCTLr_t g_pctl;
    SPDSTSr_t spdsts;
    LNKSTSr_t lnksts;
    STS_OVERRIDE_IMPr_t sts_override_imp;
    STS_OVERRIDE_P5r_t sts_override_p5;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    *mode = bmdPortModeDisabled;
    *flags = 0;

    if (port == 5 || port == CPIC_PORT) {
        ioerr += READ_SPDSTSr(unit, &spdsts);
        switch ((SPDSTSr_GET(spdsts) >> (port*2)) & 0x3) {
        case SPDSTS_SPEED_10:
            *mode = bmdPortMode10fd;
            break;
        case SPDSTS_SPEED_100:
            *mode = bmdPortMode100fd;
            break;
        default:
            *mode = bmdPortMode1000fd;
            break;
        }
        ioerr += READ_LNKSTSr(unit, &lnksts);
        if ((LNKSTSr_GET(lnksts) >> port) & 0x1 ) {
            *flags |= BMD_PORT_MODE_F_LINK_UP;
        }

        if (port == 5) {
            ioerr += READ_STS_OVERRIDE_P5r(unit, &sts_override_p5);
            if (STS_OVERRIDE_P5r_SW_OVERRIDEf_GET(sts_override_p5) == 0) {
                *flags |= BMD_PORT_MODE_F_AUTONEG;
            }
        } else {
            ioerr += READ_STS_OVERRIDE_IMPr(unit, &sts_override_imp);
            if (STS_OVERRIDE_IMPr_MII_SW_ORf_GET(sts_override_imp) == 0) {
                *flags |= BMD_PORT_MODE_F_AUTONEG;
            }
        }
        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }

    /* Check if MAC is disabled */
    if (READ_G_PCTLr(unit, port, &g_pctl) != 0) {
        return CDK_E_IO;
    }
    if (G_PCTLr_RX_DISf_GET(g_pctl) == 1) {
        return CDK_E_NONE;
    }

    if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
        *flags |= BMD_PORT_MODE_F_LINK_UP;
    }

    return bmd_port_mode_from_phy(unit, port, mode, flags);
}

#endif /* CDK_CONFIG_INCLUDE_BCM53084_A0 */
