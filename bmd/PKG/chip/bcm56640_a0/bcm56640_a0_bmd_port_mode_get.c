#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56640_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/chip/bcm56640_a0_defs.h>

#include "bcm56640_a0_bmd.h"
#include "bcm56640_a0_internal.h"

int 
bcm56640_a0_bmd_port_mode_get(int unit, int port, 
                              bmd_port_mode_t *mode, uint32_t *flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int get_phy_mode = 0;
    int intf;
    XMAC_CTRLr_t mac_ctrl;
    XMAC_MODEr_t mac_mode;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    *mode = bmdPortModeDisabled;
    *flags = 0;

    if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
        *flags |= BMD_PORT_MODE_F_LINK_UP;
    }

    ioerr += READ_XMAC_CTRLr(unit, port, &mac_ctrl);
    if (XMAC_CTRLr_RX_ENf_GET(mac_ctrl) != 0) {
        *mode = bmdPortMode10000fd;
        if (ioerr == 0 && XMAC_CTRLr_LINE_LOCAL_LPBKf_GET(mac_ctrl) != 0) {
            *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
        } else {
            get_phy_mode = 1;
        }
        ioerr += READ_XMAC_MODEr(unit, port, &mac_mode);
        switch (XMAC_MODEr_HDR_MODEf_GET(mac_mode)) {
        case XMAC_MODE_HDR_HIGIG:
            *flags |= BMD_PORT_MODE_F_HIGIG;
            break;
        case XMAC_MODE_HDR_HIGIG2:
            *flags |= BMD_PORT_MODE_F_HIGIG2;
            break;
        default:
            break;
        }
        switch (XMAC_MODEr_SPEED_MODEf_GET(mac_mode)) {
        case XMAC_MODE_SPEED_10:
            *mode = bmdPortMode10fd;
            break;
        case XMAC_MODE_SPEED_100:
            *mode = bmdPortMode100fd;
            break;
        case XMAC_MODE_SPEED_1000:
            *mode = bmdPortMode1000fd;
            break;
        case XMAC_MODE_SPEED_2500:
            *mode = bmdPortMode2500fd;
            break;
        default:
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
        } else if (*mode == bmdPortMode40000fd) {
            if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
                if (intf == BMD_PHY_IF_KR) {
                    *mode = bmdPortMode40000KR;
                } else if (intf == BMD_PHY_IF_CR) {
                    *mode = bmdPortMode40000CR;
                }
            }
        }
    }

    return ioerr ? CDK_E_IO : rv; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56640_A0 */

