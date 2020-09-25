/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53570_B0 == 1

#include <bmd/bmd.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/chip/bcm53570_b0_defs.h>
#include <cdk/cdk_debug.h>
#include "bcm53570_b0_bmd.h"
#include "bcm53570_b0_internal.h"

int
bcm53570_b0_bmd_port_mode_get(int unit, int port,
                              bmd_port_mode_t *mode, uint32_t *flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int get_phy_mode = 0;
    int intf;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLPORT_CONFIGr_t xlport_cfg;
    CLMAC_CTRLr_t clmac_ctrl;
    CLPORT_CONFIGr_t clport_cfg;
    COMMAND_CONFIGr_t command_cfg;
    cdk_pbmp_t clpbmp, gpbmp;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    *mode = bmdPortModeDisabled;
    *flags = 0;

    if (BMD_PORT_STATUS(unit, port) & BMD_PST_DISABLED) {
        return CDK_E_NONE;
    }

    if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
        *flags |= BMD_PORT_MODE_F_LINK_UP;
    }

    bcm53570_b0_gport_pbmp_get(unit, &gpbmp);
    bcm53570_b0_clport_pbmp_get(unit, &clpbmp);
    if (CDK_PBMP_MEMBER(clpbmp, port)) {
        ioerr += READ_CLMAC_CTRLr(unit, port, &clmac_ctrl);
        *mode = bmdPortMode25000fd;
        if (CLMAC_CTRLr_LOCAL_LPBKf_GET(clmac_ctrl) != 0) {
            *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
        } else {
            get_phy_mode = 1;
        }

        ioerr += READ_CLPORT_CONFIGr(unit, port, &clport_cfg);
        if (CLPORT_CONFIGr_HIGIG_MODEf_GET(clport_cfg)) {
            if (CLPORT_CONFIGr_HIGIG2_MODEf_GET(clport_cfg)) {
                *flags |= BMD_PORT_MODE_F_HIGIG2;
            } else {
                *flags |= BMD_PORT_MODE_F_HIGIG;
            }
        }
    } else if (CDK_PBMP_MEMBER(gpbmp, port)) {
        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
        if (ioerr == 0 && COMMAND_CONFIGr_RX_ENAf_GET(command_cfg)) {
            switch (COMMAND_CONFIGr_ETH_SPEEDf_GET(command_cfg)) {
            case COMMAND_CONFIG_SPEED_10:
                *mode = bmdPortMode10fd;
                break;
            case COMMAND_CONFIG_SPEED_100:
                *mode = bmdPortMode100fd;
                break;
            case COMMAND_CONFIG_SPEED_2500:
                *mode = bmdPortMode2500fd;
                break;
            default:
                *mode = bmdPortMode1000fd;
                break;
            }
            if (COMMAND_CONFIGr_LOOP_ENAf_GET(command_cfg) == 1) {
                *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
            } else {
                get_phy_mode = 1;
            }
        }
    } else {
        ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
        *mode = bmdPortMode2500fd;
        if (XLMAC_CTRLr_LOCAL_LPBKf_GET(xlmac_ctrl) != 0) {
            *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
        } else {
            get_phy_mode = 1;
        }

        ioerr += READ_XLPORT_CONFIGr(unit, port, &xlport_cfg);
        if (XLPORT_CONFIGr_HIGIG_MODEf_GET(xlport_cfg)) {
            if (XLPORT_CONFIGr_HIGIG2_MODEf_GET(xlport_cfg)) {
                *flags |= BMD_PORT_MODE_F_HIGIG2;
            } else {
                *flags |= BMD_PORT_MODE_F_HIGIG;
            }
        }
    }

    if (get_phy_mode) {
        rv = bmd_port_mode_from_phy(unit, port, mode, flags);
        if (*mode == bmdPortMode10000fd) {
            if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                if (intf == BMD_PHY_IF_XFI) {
                    *mode = bmdPortMode10000XFI;
                } else if (intf == BMD_PHY_IF_SFI) {
                    *mode = bmdPortMode10000SFI;
                } else if (intf == BMD_PHY_IF_KR) {
                    *mode = bmdPortMode10000KR;
                } else if (intf == BMD_PHY_IF_CR) {
                    *mode = bmdPortMode10000CR;
                }
            }
        } else if (*mode == bmdPortMode20000fd) {
            if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                if (intf == BMD_PHY_IF_KR) {
                    *mode = bmdPortMode20000KR;
                }
            }
        } else if (*mode == bmdPortMode25000fd) {
            if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                if (intf == BMD_PHY_IF_XFI) {
                    *mode = bmdPortMode25000XFI;
                }
            }
        } else if (*mode == bmdPortMode42000fd) {
            if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                if (intf == BMD_PHY_IF_KR) {
                    *mode = bmdPortMode42000KR;
                }
            }
        } else if (*mode == bmdPortMode40000fd) {
            if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                if (intf == BMD_PHY_IF_CR) {
                    *mode = bmdPortMode40000CR;
                } else if (intf == BMD_PHY_IF_KR) {
                    *mode = bmdPortMode40000KR;
                } else if (intf == BMD_PHY_IF_SR) {
                    *mode = bmdPortMode40000SR;
                }
            }
        } else if (*mode == bmdPortMode53000fd) {
            if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                if (intf == BMD_PHY_IF_KR) {
                    *mode = bmdPortMode53000KR;
                }
            }
        } else if (*mode == bmdPortMode50000fd) {
            if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                if (intf == BMD_PHY_IF_KR) {
                    *mode = bmdPortMode50000KR;
                }
            }
        }
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53570_B0 */

