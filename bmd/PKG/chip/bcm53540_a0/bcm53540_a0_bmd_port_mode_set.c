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
#include <bmd/bmd_device.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm53540_a0_defs.h>

#include "bcm53540_a0_bmd.h"
#include "bcm53540_a0_internal.h"

#define DRAIN_WAIT_MSEC                 500
#define NUM_COS                         8

/* Supported HiGig encapsulations */
#define HG_FLAGS  (BMD_PORT_MODE_F_HIGIG | BMD_PORT_MODE_F_HIGIG2 | BMD_PORT_MODE_F_HGLITE)

int
bcm53540_a0_bmd_port_mode_set(int unit, int port,
                              bmd_port_mode_t mode, uint32_t flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int mac_lb = (flags & BMD_PORT_MODE_F_MAC_LOOPBACK) ? 1 : 0;
    int phy_lb = (flags & BMD_PORT_MODE_F_PHY_LOOPBACK) ? 1 : 0;
    int duplex = 1;
    int speed = 0;
    int sp_sel = COMMAND_CONFIG_SPEED_1000;
    int pref_intf = 0;
    int speed_max;
    int cnt, idx, tot_cnt_cos_cell;
    int lport, mport;
    uint32_t pbmp;
    cdk_pbmp_t gpbmp;
    EPC_LINK_BMAP_64r_t epc_link;
    MMU_FC_RX_ENr_t mmu_fc_g, mmu_fc_s;
    FLUSH_CONTROLr_t flush_ctrl;
    COSLCCOUNTr_t coslccount;
    COMMAND_CONFIGr_t command_cfg;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    bcm53540_a0_gport_pbmp_get(unit, &gpbmp);

    speed_max = bcm53540_a0_port_speed_max(unit, port);

    if (flags & HG_FLAGS) {
        pref_intf = BMD_PHY_IF_HIGIG;
    }

    if (BMD_PORT_PROPERTIES(unit, port) & (BMD_PORT_HG | BMD_PORT_XE)) {
        switch (mode) {
        case bmdPortModeAuto:
        case bmdPortModeDisabled:
            speed = speed_max;
            break;
        case bmdPortMode1000KX:
            if (!(flags & BMD_PORT_MODE_F_INTERNAL)) {
                return CDK_E_PARAM;
            }
            speed = 1000;
            pref_intf = BMD_PHY_IF_KX;
            break;
        default:
            break;
        }
    }

    if (speed == 0) {
        if (speed_max >= 10000) {
            return CDK_E_PARAM;
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
        case bmdPortModeAuto:
        case bmdPortModeDisabled:
        case bmdPortMode1000fd:
        case bmdPortMode1000hd:
            speed = 1000;
            sp_sel = COMMAND_CONFIG_SPEED_1000;
            break;
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
        case bmdPortMode2500fd:
            speed = 2500;
            sp_sel = COMMAND_CONFIG_SPEED_2500;
            break;
        default:
            return CDK_E_PARAM;
        }
    }

    if (speed > speed_max) {
        return CDK_E_PARAM;
    }

    lport = P2L(unit, port);
    mport = P2M(unit, port);

    if ((flags & BMD_PORT_MODE_F_INTERNAL) == 0) {
        /* Set preferred line interface */
        rv = bmd_phy_line_interface_set(unit, port, pref_intf);

        /* Stop CPU and MMU from scheduling packets to the port */
        BMD_PORT_STATUS_CLR(unit, port, BMD_PST_LINK_UP);
        ioerr += READ_EPC_LINK_BMAP_64r(unit, &epc_link);
        pbmp = EPC_LINK_BMAP_64r_PORT_BITMAPf_GET(epc_link);
        EPC_LINK_BMAP_64r_PORT_BITMAPf_SET(epc_link, pbmp & ~(1 << port));
        ioerr += WRITE_EPC_LINK_BMAP_64r(unit, epc_link);

        /* Clear the MMU pause state*/
        ioerr += READ_MMU_FC_RX_ENr(unit, mport, &mmu_fc_g);
        MMU_FC_RX_ENr_CLR(mmu_fc_s);
        ioerr += WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_s);

        /* Drain all packets from the TX pipeline */
        if (CDK_PBMP_MEMBER(gpbmp, port)) {
            ioerr += READ_FLUSH_CONTROLr(unit, port, &flush_ctrl);
            FLUSH_CONTROLr_FLUSHf_SET(flush_ctrl, 1);
            ioerr += WRITE_FLUSH_CONTROLr(unit, port, flush_ctrl);
        }

        cnt = DRAIN_WAIT_MSEC / 10;
        while (--cnt >= 0) {
            tot_cnt_cos_cell = 0;
            for (idx = 0; idx < NUM_COS; idx++) {
                ioerr += READ_COSLCCOUNTr(unit, lport, idx, &coslccount);
                if (COSLCCOUNTr_GET(coslccount) != 0) {
                    tot_cnt_cos_cell++;
                }
            }
            if (tot_cnt_cos_cell == 0) {
                break;
            }
            BMD_SYS_USLEEP(10000);
        }
        if (cnt < 0) {
            CDK_WARN(("bcm53540_a0_bmd_port_mode_set[%d]: "
                      "drain failed on port %d\n", unit, port));
        }

        /* Bring the TX pipeline out of flush */
        if (CDK_PBMP_MEMBER(gpbmp, port)) {
            ioerr += READ_FLUSH_CONTROLr(unit, port, &flush_ctrl);
            FLUSH_CONTROLr_FLUSHf_SET(flush_ctrl, 0);
            ioerr += WRITE_FLUSH_CONTROLr(unit, port, flush_ctrl);
        }

        /* Restore the MMU pause state*/
        ioerr += WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_g);
    }

    /* Disable MACs (Rx only) */
    if (CDK_PBMP_MEMBER(gpbmp, port)) {
        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
        COMMAND_CONFIGr_SW_RESETf_SET(command_cfg, 1);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
        COMMAND_CONFIGr_RX_ENAf_SET(command_cfg, 0);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);
    }

    /* Update PHYs before MAC */
    if (mode == bmdPortModeDisabled) {
        rv = bmd_phy_mode_set(unit, port, "viper", BMD_PHY_MODE_DISABLED, 1);
    } else {
        rv = bmd_phy_mode_set(unit, port, "viper", BMD_PHY_MODE_DISABLED, 0);
        if (CDK_SUCCESS(rv)) {
            rv = bmd_port_mode_to_phy(unit, port, mode, flags, speed, duplex);
        }
    }

    /* Let PHYs know that we disable the MAC */
    if (CDK_SUCCESS(rv)) {
        rv = bmd_phy_notify_mac_enable(unit, port, 0);
    }

    if (mode == bmdPortModeDisabled) {
        BMD_PORT_STATUS_SET(unit, port, BMD_PST_FORCE_LINK);
    } else {
        if (CDK_PBMP_MEMBER(gpbmp, port)) {
            ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
            COMMAND_CONFIGr_ETH_SPEEDf_SET(command_cfg, sp_sel);
            COMMAND_CONFIGr_HD_ENAf_SET(command_cfg, !duplex);
            ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

            /* Set MAC loopback mode */
            ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
            COMMAND_CONFIGr_LOOP_ENAf_SET(command_cfg, mac_lb);
            COMMAND_CONFIGr_ENA_EXT_CONFIGf_SET(command_cfg, 0);
            ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

            /* Enable MAC */
            ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
            COMMAND_CONFIGr_RX_ENAf_SET(command_cfg, 1);
            COMMAND_CONFIGr_TX_ENAf_SET(command_cfg, 1);
            ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

            /* Bring the MAC out of reset */
            ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
            COMMAND_CONFIGr_SW_RESETf_SET(command_cfg, 0);
            ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);
        }

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

#endif /* CDK_CONFIG_INCLUDE_BCM53540_A0 */
