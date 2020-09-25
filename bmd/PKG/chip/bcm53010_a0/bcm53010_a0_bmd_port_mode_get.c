#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53010_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/chip/bcm53010_a0_defs.h>

#include "bcm53010_a0_internal.h"
#include "bcm53010_a0_bmd.h"

int
bcm53010_a0_bmd_port_mode_get(int unit, int port, 
                             bmd_port_mode_t *mode, uint32_t *flags)
{
    int ioerr = 0;
    G_PCTLr_t g_pctl;
    SPDSTSr_t spdsts;
    LNKSTSr_t lnksts;
    STS_OVERRIDE_IMPr_t sts_override_imp;
	int speed_up_to_2g = 0;
    STS_OVERRIDE_P7r_t stsovip7;
    STS_OVERRIDE_P5r_t stsovip5;
	int spdsts_shift = 0;
	int lnksts_shift = 0;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    *mode = bmdPortModeDisabled;
    *flags = 0;

    if ((port == CPIC_PORT) || (port == 7) || (port == 5)) {
        if (port == 5) {
            spdsts_shift = 10;
            lnksts_shift = 5;
        } else if (port == 7) {
            spdsts_shift = 14;
            lnksts_shift = 7;
        } else {
            /* CPIC port */
            spdsts_shift = 16;
            lnksts_shift = 8;
        }

        ioerr =+ READ_SPDSTSr(unit, &spdsts);
        switch (SPDSTSr_GET(spdsts) >> spdsts_shift) {
        case SPDSTS_SPEED_10:
            *mode = bmdPortMode10fd;
            break;
        case SPDSTS_SPEED_100:
            *mode = bmdPortMode100fd;
            break;
        default:
            if (port == 5) {
                READ_STS_OVERRIDE_P5r(unit, &stsovip5);
                if (STS_OVERRIDE_P5r_GMII_SPEED_UP_2Gf_GET(stsovip5)) {
                    speed_up_to_2g = 1;
				}
            } else if (port == 7) {
                READ_STS_OVERRIDE_P7r(unit, &stsovip7);
                if (STS_OVERRIDE_P7r_GMII_SPEED_UP_2Gf_GET(stsovip7)) {
                    speed_up_to_2g = 1;
				}
            } else { /* CPIC port */
                READ_STS_OVERRIDE_IMPr(unit, &sts_override_imp);
                if (STS_OVERRIDE_IMPr_GMII_SPEED_UP_2Gf_GET(sts_override_imp)) {
                    speed_up_to_2g = 1;
				}
            }
            if (speed_up_to_2g) {
                *mode = bmdPortMode2000fd;
            } else {
                *mode = bmdPortMode1000fd;
            }
            break;
        }
        ioerr += READ_LNKSTSr(unit, &lnksts);
        if ((LNKSTSr_GET(lnksts) >> lnksts_shift) & 0x1) {
            *flags |= BMD_PORT_MODE_F_LINK_UP;
        }
        if (port == 5) {
            ioerr += READ_STS_OVERRIDE_P5r(unit, &stsovip5);
            if (STS_OVERRIDE_P5r_SW_OVERRIDEf_GET(stsovip5) == 0) {
                *flags |= BMD_PORT_MODE_F_AUTONEG;
            }
        } else if (port == 7) {
            ioerr += READ_STS_OVERRIDE_P7r(unit, &stsovip7);
            if (STS_OVERRIDE_P7r_SW_OVERRIDEf_GET(stsovip7) == 0) {
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
#endif /* CDK_CONFIG_INCLUDE_BCM53010_A0 */
