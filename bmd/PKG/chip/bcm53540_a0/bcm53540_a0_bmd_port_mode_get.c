/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53540_A0 == 1

#include <bmd/bmd.h>
#include <bmdi/bmd_port_mode.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm53540_a0_defs.h>
#include "bcm53540_a0_bmd.h"
#include "bcm53540_a0_internal.h"
int
bcm53540_a0_bmd_port_mode_get(int unit, int port,
                              bmd_port_mode_t *mode, uint32_t* flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int get_phy_mode = 0;
    int intf;
    COMMAND_CONFIGr_t command_cfg;
    cdk_pbmp_t gpbmp;

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

    bcm53540_a0_gport_pbmp_get(unit, &gpbmp);
    if (CDK_PBMP_MEMBER(gpbmp, port)) {
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
    }

    if (get_phy_mode) {
        rv = bmd_port_mode_from_phy(unit, port, mode, flags);
        if (CDK_SUCCESS(bmd_phy_line_interface_get(unit, port, &intf))) {
            if (*mode == bmdPortMode1000fd) {
                if (intf == BMD_PHY_IF_KX) {
                    *mode = bmdPortMode1000KX;
                }
            }
        }
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53540_A0 */
