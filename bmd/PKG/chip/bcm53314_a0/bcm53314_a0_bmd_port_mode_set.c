#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53314_A0 == 1

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

#include <cdk/chip/bcm53314_a0_defs.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm53314_a0_bmd.h"
#include "bcm53314_a0_internal.h"

#define DRAIN_WAIT_MSEC                 500

int
bcm53314_a0_bmd_port_mode_set(int unit, int port, 
                              bmd_port_mode_t mode, uint32_t flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int mac_lb = (flags & BMD_PORT_MODE_F_MAC_LOOPBACK) ? 1 : 0;
    int phy_lb = (flags & BMD_PORT_MODE_F_PHY_LOOPBACK) ? 1 : 0;
    int duplex = 1;
    int speed = 1000;
    int sp_sel = COMMAND_CONFIG_SPEED_1000;
    int cnt;
    uint32_t pbmp;
    EPC_LINK_BMAPr_t epc_link;
    FLUSH_CONTROLr_t flush_ctrl;
    COSLCCOUNTr_t lccount;
    COMMAND_CONFIGr_t command_config;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

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
        sp_sel = COMMAND_CONFIG_SPEED_10;
        break;
    case bmdPortMode100fd:
    case bmdPortMode100hd:
        speed = 100;
        sp_sel = COMMAND_CONFIG_SPEED_100;
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

    if ((flags & BMD_PORT_MODE_F_INTERNAL) == 0) {

        /* Stop CPU and MMU from scheduling packets to the port */
        BMD_PORT_STATUS_CLR(unit, port, BMD_PST_LINK_UP);
        ioerr += READ_EPC_LINK_BMAPr(unit, &epc_link);
        pbmp = EPC_LINK_BMAPr_PORT_BITMAPf_GET(epc_link);
        EPC_LINK_BMAPr_PORT_BITMAPf_SET(epc_link, pbmp & ~(1 << port));
        ioerr += WRITE_EPC_LINK_BMAPr(unit, epc_link);

        /* Drain all packets from the Tx pipeline */
        ioerr += READ_FLUSH_CONTROLr(unit, port, &flush_ctrl);
        FLUSH_CONTROLr_FLUSHf_SET(flush_ctrl, 1);
        ioerr += WRITE_FLUSH_CONTROLr(unit, port, flush_ctrl);
        cnt = DRAIN_WAIT_MSEC / 10;
        while (--cnt >= 0) {
            ioerr += READ_COSLCCOUNTr(unit, port, 0, &lccount);
            if (COSLCCOUNTr_GET(lccount) == 0) {
                break;
            }
            BMD_SYS_USLEEP(10000);
        }
        if (cnt < 0) {
            CDK_WARN(("bcm53314_a0_bmd_port_mode_set[%d]: "
                      "drain failed on port %d\n", unit, port));
        }
        FLUSH_CONTROLr_FLUSHf_SET(flush_ctrl, 0);
        ioerr += WRITE_FLUSH_CONTROLr(unit, port, flush_ctrl);
    }

    /* MAC loopback has no clock, so we also set PHY loopback */
    if (mac_lb) {
        flags |= BMD_PORT_MODE_F_PHY_LOOPBACK;
    }
    /* Update PHYs before MAC */
    if (CDK_SUCCESS(rv)) {
        rv = bmd_port_mode_to_phy(unit, port, mode, flags, speed, duplex);
    }

    /* Let PHYs know that we disable the MAC */
    if (CDK_SUCCESS(rv)) {
        rv = bmd_phy_notify_mac_enable(unit, port, 0);
    }

    /* Reset the MAC */
    ioerr += READ_COMMAND_CONFIGr(unit, port, &command_config);
    COMMAND_CONFIGr_SW_RESETf_SET(command_config, 1);
    ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_config);

    /* Disable MACs (Rx only) */
    ioerr += READ_COMMAND_CONFIGr(unit, port, &command_config);
    COMMAND_CONFIGr_RX_ENAf_SET(command_config, 0);

    /* Set speed */
    COMMAND_CONFIGr_ETH_SPEEDf_SET(command_config, sp_sel);

    /* Set duplex */
    COMMAND_CONFIGr_HD_ENAf_SET(command_config, !duplex);

    ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_config);

    /* Bring the MAC out of reset */
    ioerr += READ_COMMAND_CONFIGr(unit, port, &command_config);
    COMMAND_CONFIGr_SW_RESETf_SET(command_config, 0);
    ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_config);

    /* Set MAC loopback mode */
    ioerr += READ_COMMAND_CONFIGr(unit, port, &command_config);
    COMMAND_CONFIGr_LOOP_ENAf_SET(command_config, mac_lb);
    ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_config);

    if (mode == bmdPortModeDisabled) {
        BMD_PORT_STATUS_SET(unit, port, BMD_PST_FORCE_LINK);
    } else {
        /* Enable MAC TX / RX */
        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_config);
        COMMAND_CONFIGr_SW_RESETf_SET(command_config, 1);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_config);

        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_config);
        COMMAND_CONFIGr_RX_ENAf_SET(command_config, 1);
        COMMAND_CONFIGr_TX_ENAf_SET(command_config, 1);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_config);

        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_config);
        COMMAND_CONFIGr_SW_RESETf_SET(command_config, 0);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_config);

        if (mac_lb || phy_lb) {
            BMD_PORT_STATUS_SET(unit, port, BMD_PST_LINK_UP | BMD_PST_FORCE_LINK);
        } else {
            BMD_PORT_STATUS_CLR(unit, port, BMD_PST_FORCE_LINK);
        }

        /* Let PHYs know that the MAC has been enabled */
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_notify_mac_enable(unit, port, 1);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53314_A0 */
