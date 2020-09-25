#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56624_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/chip/bcm56624_a0_defs.h>
#include <cdk/arch/xgs_chip.h>

#include "bcm56624_a0_bmd.h"
#include "bcm56624_a0_internal.h"

int
bcm56624_a0_bmd_port_mode_get(int unit, int port, 
                              bmd_port_mode_t *mode, uint32_t* flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int get_phy_mode = 0;
    COMMAND_CONFIGr_t command_cfg;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    *mode = bmdPortModeDisabled;
    *flags = 0;

    if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
        *flags |= BMD_PORT_MODE_F_LINK_UP;
    }

    if (BMD_PORT_PROPERTIES(unit, port) & (BMD_PORT_HG | BMD_PORT_XE)) {
#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
        MAC_CTRLr_t mac_ctrl;
        XPORT_CONFIGr_t xport_cfg;

        ioerr += READ_MAC_CTRLr(unit, port, &mac_ctrl);
        if (MAC_CTRLr_RXENf_GET(mac_ctrl) != 0) {
            *mode = bmdPortMode10000fd;
            if (ioerr == 0 && MAC_CTRLr_LCLLOOPf_GET(mac_ctrl) != 0) {
                *flags |= BMD_PORT_MODE_F_MAC_LOOPBACK;
            } else {
                get_phy_mode = 1;
            }
            ioerr += READ_XPORT_CONFIGr(unit, port, &xport_cfg);
            if (XPORT_CONFIGr_HIGIG_MODEf_GET(xport_cfg)) {
                if (XPORT_CONFIGr_HIGIG2_MODEf_GET(xport_cfg)) {
                    *flags |= BMD_PORT_MODE_F_HIGIG2;
                } else {
                    *flags |= BMD_PORT_MODE_F_HIGIG;
                }
            }
        }
#endif
    }
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
    if (get_phy_mode) {
        rv = bmd_port_mode_from_phy(unit, port, mode, flags);
    }

    return ioerr ? CDK_E_IO : rv; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56624_A0 */
