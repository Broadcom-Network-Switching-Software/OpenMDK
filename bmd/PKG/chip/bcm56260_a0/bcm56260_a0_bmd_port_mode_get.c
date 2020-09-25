#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56260_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/chip/bcm56260_a0_defs.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm56260_a0_bmd.h"
#include "bcm56260_a0_internal.h"

int
bcm56260_a0_bmd_port_mode_get(int unit, int port, 
                              bmd_port_mode_t *mode, uint32_t* flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int get_phy_mode = 0;
    int intf;
    int xlmac_speed_mode;
    XLMAC_MODEr_t xlmac_mode;
    XLMAC_CTRLr_t mac_ctrl;
    XPORT_CONFIGr_t xport_cfg;
    XLPORT_CONFIGr_t xlport_cfg;
    cdk_pbmp_t xlport_pbmp, mxqport_pbmp;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    *mode = bmdPortModeDisabled;
    *flags = 0;

    if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
        *flags |= BMD_PORT_MODE_F_LINK_UP;
    }

    ioerr += READ_XLMAC_CTRLr(unit, port, &mac_ctrl);
    if (XLMAC_CTRLr_RX_ENf_GET(mac_ctrl) != 0) {
        *mode = bmdPortMode10000fd;
        if (ioerr == 0 && XLMAC_CTRLr_LOCAL_LPBKf_GET(mac_ctrl) != 0) {
            *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
        } else {
            get_phy_mode = 1;
        }
        bcm56260_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
        bcm56260_a0_xlport_pbmp_get(unit, &xlport_pbmp);
        if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
            ioerr += READ_XLPORT_CONFIGr(unit, port, &xlport_cfg);
            if (XLPORT_CONFIGr_HIGIG_MODEf_GET(xlport_cfg)) {
                if (XLPORT_CONFIGr_HIGIG2_MODEf_GET(xlport_cfg)) {
                    *flags |= BMD_PORT_MODE_F_HIGIG2;
                } else {
                    *flags |= BMD_PORT_MODE_F_HIGIG;
                }
            }
        } else {
            ioerr += READ_XPORT_CONFIGr(unit, port, &xport_cfg);
            if (XPORT_CONFIGr_HIGIG_MODEf_GET(xport_cfg)) {
                if (XPORT_CONFIGr_HIGIG2_MODEf_GET(xport_cfg)) {
                    *flags |= BMD_PORT_MODE_F_HIGIG2;
                } else {
                    *flags |= BMD_PORT_MODE_F_HIGIG;
                }
            }
        }
        

        ioerr += READ_XLMAC_MODEr(unit, port, &xlmac_mode);
        xlmac_speed_mode = XLMAC_MODEr_SPEED_MODEf_GET(xlmac_mode);
        switch (xlmac_speed_mode) {
        case COMMAND_CONFIG_SPEED_10:
            *mode = bmdPortMode10fd;
            break;
        case COMMAND_CONFIG_SPEED_100:
            *mode = bmdPortMode100fd;
            break;
        case COMMAND_CONFIG_SPEED_2500:
            *mode = bmdPortMode2500fd;
            break;
        case COMMAND_CONFIG_SPEED_10000:
            *mode = bmdPortMode10000fd;
            break;
        default:
            *mode = bmdPortMode1000fd;
            break;
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
                }
            }
        }
    }

    return ioerr ? CDK_E_IO : rv; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56260_A0 */
