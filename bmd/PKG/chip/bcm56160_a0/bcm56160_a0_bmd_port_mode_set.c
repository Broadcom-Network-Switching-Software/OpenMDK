/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56160_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56160_a0_defs.h>

#include "bcm56160_a0_bmd.h"
#include "bcm56160_a0_internal.h"

#define DRAIN_WAIT_MSEC                 500
#define NUM_COS                         8

/* Supported HiGig encapsulations */
#define HG_FLAGS  (BMD_PORT_MODE_F_HIGIG | BMD_PORT_MODE_F_HIGIG2 | BMD_PORT_MODE_F_HGLITE)

int
bcm56160_a0_bmd_port_mode_set(int unit, int port,
                              bmd_port_mode_t mode, uint32_t flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int mac_lb = (flags & BMD_PORT_MODE_F_MAC_LOOPBACK) ? 1 : 0;
    int phy_lb = (flags & BMD_PORT_MODE_F_PHY_LOOPBACK) ? 1 : 0;
    int duplex = 1;
    int speed = 0;
    int sp_sel = COMMAND_CONFIG_SPEED_10000;
    int pref_intf = 0;
    int speed_max;
    int cnt, idx, tot_cnt_cos_cell;
    int lport, mport;
    int lanes;
    uint32_t pbmp;
    cdk_pbmp_t gpbmp, xlpbmp;
    EPC_LINK_BMAP_64r_t epc_link;
    MMU_FC_RX_ENr_t mmu_fc_g, mmu_fc_s;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    FLUSH_CONTROLr_t flush_ctrl;
    COSLCCOUNTr_t coslccount;
    COMMAND_CONFIGr_t command_cfg;
    XLMAC_CTRLr_t xlmac_ctrl;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    bcm56160_a0_gport_pbmp_get(unit, &gpbmp);
    bcm56160_a0_xlport_pbmp_get(unit, &xlpbmp);

    lanes = bcm56160_a0_port_num_lanes(unit, port);
    speed_max = bcm56160_a0_port_speed_max(unit, port);

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
        case bmdPortMode5000fd:
            if (lanes == 4) {
                return CDK_E_PARAM;
            }
            speed = 5000;
            break;
        case bmdPortMode10000fd:
        case bmdPortMode10000XFI:
            speed = 10000;
            break;
        case bmdPortMode10000CR:
            speed = 10000;
            pref_intf = BMD_PHY_IF_CR;
        case bmdPortMode10000KR:
            if (!(flags & BMD_PORT_MODE_F_INTERNAL)) {
                return CDK_E_PARAM;
            }
            speed = 10000;
            pref_intf = BMD_PHY_IF_KR;
            break;
        case bmdPortMode10000SFI:
            speed = 10000;
            pref_intf = BMD_PHY_IF_SFI;
            break;
        case bmdPortMode20000fd:
            speed = 20000;
            break;
        case bmdPortMode20000KR:
            if (!(flags & BMD_PORT_MODE_F_INTERNAL)) {
                return CDK_E_PARAM;
            }
            speed = 20000;
            pref_intf = BMD_PHY_IF_KR;
            break;
        case bmdPortMode25000fd:
        case bmdPortMode25000XFI:
            if (lanes == 4) {
                return CDK_E_PARAM;
            }
            speed = 25000;
            break;
        case bmdPortMode40000fd:
            speed = 40000;
            break;
        case bmdPortMode40000KR:
            if (!(flags & BMD_PORT_MODE_F_INTERNAL)) {
                return CDK_E_PARAM;
            }
            speed = 40000;
            pref_intf = BMD_PHY_IF_KR;
            break;
        case bmdPortMode40000CR:
            speed = 40000;
            pref_intf = BMD_PHY_IF_CR;
            break;
        case bmdPortMode40000SR:
            speed = 40000;
            pref_intf = BMD_PHY_IF_SR;
            break;
#if BMD_CONFIG_INCLUDE_HIGIG == 1
        case bmdPortMode11000fd:
            if (lanes == 4) {
                return CDK_E_PARAM;
            }
            if (flags & HG_FLAGS) {
                speed = 11000;
            }
            break;
        case bmdPortMode12000fd:
            if (flags & HG_FLAGS) {
                speed = 12000;
            }
            break;
        case bmdPortMode13000fd:
            if (flags & HG_FLAGS) {
                speed = 13000;
            }
            break;
        case bmdPortMode15000fd:
            if (flags & HG_FLAGS) {
                speed = 15000;
            }
            break;
        case bmdPortMode16000fd:
            if (flags & HG_FLAGS) {
                speed = 16000;
            }
            break;
        case bmdPortMode21000fd:
            if (flags & HG_FLAGS) {
                speed = 21000;
            }
            break;
        case bmdPortMode30000fd:
            if (flags & HG_FLAGS) {
                speed = 30000;
            }
            break;
        case bmdPortMode42000fd:
            if (flags & HG_FLAGS) {
                speed = 42000;
            }
            break;
#endif
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
        if (CDK_PBMP_MEMBER(xlpbmp, port)) {
            ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
            XLMAC_TX_CTRLr_DISCARDf_SET(xlmac_tx_ctrl, 1);
            XLMAC_TX_CTRLr_EP_DISCARDf_SET(xlmac_tx_ctrl, 1);
            ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);
        } else if (CDK_PBMP_MEMBER(gpbmp, port)) {
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
            CDK_WARN(("bcm56160_a0_bmd_port_mode_set[%d]: "
                      "drain failed on port %d\n", unit, port));
        }

        /* Bring the TX pipeline out of flush */
        if (CDK_PBMP_MEMBER(xlpbmp, port)) {
            ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
            XLMAC_TX_CTRLr_DISCARDf_SET(xlmac_tx_ctrl, 0);
            XLMAC_TX_CTRLr_EP_DISCARDf_SET(xlmac_tx_ctrl, 0);
            ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);
        } else if (CDK_PBMP_MEMBER(gpbmp, port)) {
            ioerr += READ_FLUSH_CONTROLr(unit, port, &flush_ctrl);
            FLUSH_CONTROLr_FLUSHf_SET(flush_ctrl, 0);
            ioerr += WRITE_FLUSH_CONTROLr(unit, port, flush_ctrl);
        }

        /* Restore the MMU pause state*/
        ioerr += WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_g);

#if BMD_CONFIG_INCLUDE_HIGIG == 1 || BMD_CONFIG_INCLUDE_XE == 1
        if (CDK_PBMP_MEMBER(xlpbmp, port)) {
            bmd_port_mode_t cur_mode;
            uint32_t cur_flags;
            /*
             * If HiGig/Ethernet encapsulation changes, we need
             * to reinitialize from scratch.
             */
            rv = bcm56160_a0_bmd_port_mode_get(unit, port,
                                               &cur_mode, &cur_flags);
            if (CDK_SUCCESS(rv) &&
                ((flags ^ cur_flags) & HG_FLAGS)) {
                if (flags & HG_FLAGS) {
                    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_HG;
                } else {
                    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
                }

                if (CDK_SUCCESS(rv)) {
                    rv = bcm56160_a0_xlport_init(unit, port);
                }
            }
        }
#endif
    }

    /* Disable MACs (Rx only) */
    if (CDK_PBMP_MEMBER(xlpbmp, port)) {
        ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
        XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 0);
        ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);
    } else if (CDK_PBMP_MEMBER(gpbmp, port)) {
        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
        COMMAND_CONFIGr_SW_RESETf_SET(command_cfg, 1);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
        COMMAND_CONFIGr_RX_ENAf_SET(command_cfg, 0);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);
    }

    /* Update PHYs before MAC */
    if (mode == bmdPortModeDisabled) {
        rv = bmd_phy_mode_set(unit, port, "tsce", BMD_PHY_MODE_DISABLED, 1);
        rv = bmd_phy_mode_set(unit, port, "qtce", BMD_PHY_MODE_DISABLED, 1);
    } else {
        rv = bmd_phy_mode_set(unit, port, "tsce", BMD_PHY_MODE_DISABLED, 0);
        rv = bmd_phy_mode_set(unit, port, "qtce", BMD_PHY_MODE_DISABLED, 0);

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
        if (CDK_PBMP_MEMBER(xlpbmp, port)) {
            int hg, hg2, encap;
            PORT_TABm_t port_tab;
            ICONTROL_OPCODE_BITMAPr_t opcode_bmap;
            EGR_VLAN_CONTROL_3r_t vctrl3;
            EGR_PORT_64r_t egr_port;
            XLPORT_CONFIGr_t xlport_cfg;
            XLPORT_ENABLE_REGr_t  xlport_enable;
            XLMAC_MODEr_t xlmac_mode;
            XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
            XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
            PGW_XL_CONFIGr_t pgw_xl_cfg;

            ioerr += READ_XLPORT_ENABLE_REGr(unit, &xlport_enable, port);
            XLPORT_ENABLE_REGr_PORT0f_SET(xlport_enable, 1);
            XLPORT_ENABLE_REGr_PORT1f_SET(xlport_enable, 1);
            XLPORT_ENABLE_REGr_PORT2f_SET(xlport_enable, 1);
            XLPORT_ENABLE_REGr_PORT3f_SET(xlport_enable, 1);
            ioerr += WRITE_XLPORT_ENABLE_REGr(unit, xlport_enable, port);

            /* Set encapsulation */
            hg = hg2 = 0;
            encap = 0;
#if BMD_CONFIG_INCLUDE_HIGIG == 1
            if (flags & HG_FLAGS) {
                hg = 1;
                encap = 1;
                if ((flags & BMD_PORT_MODE_F_HIGIG2) || (flags & BMD_PORT_MODE_F_HGLITE)) {
                    hg2 = 1;
                    encap = 2;
                }
            }
#endif /* BMD_CONFIG_INCLUDE_HIGIG */

            ioerr += READ_XLMAC_RX_CTRLr(unit, port, &xlmac_rx_ctrl);
            XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 1);
            if ((speed < 10000) || (flags & HG_FLAGS)) {
                XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 0);
            }
            ioerr += WRITE_XLMAC_RX_CTRLr(unit, port, xlmac_rx_ctrl);

            ioerr += READ_XLMAC_RX_LSS_CTRLr(unit, port, &xlmac_rx_lss_ctrl);
            XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, 0);
            XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, 0);
            if ((speed < 5000) || (mac_lb == 1)) {
                XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, 1);
                XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, 1);
            }
            ioerr += WRITE_XLMAC_RX_LSS_CTRLr(unit, port, xlmac_rx_lss_ctrl);

            ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
            XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, 0xc);
            if (flags & HG_FLAGS) {
                XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, 0x8);
            }
            ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);

            ioerr += READ_XLMAC_MODEr(unit, port, &xlmac_mode);
            XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, sp_sel);
            XLMAC_MODEr_HDR_MODEf_SET(xlmac_mode, encap);
            ioerr += WRITE_XLMAC_MODEr(unit, port, xlmac_mode);

            ioerr += READ_PORT_TABm(unit, lport, &port_tab);
            PORT_TABm_PORT_TYPEf_SET(port_tab, hg);
            PORT_TABm_HIGIG2f_SET(port_tab, hg2);
            ioerr += WRITE_PORT_TABm(unit, lport, port_tab);

            ioerr += READ_EGR_PORT_64r(unit, lport, &egr_port);
            EGR_PORT_64r_PORT_TYPEf_SET(egr_port, hg);
            EGR_PORT_64r_HIGIG2f_SET(egr_port, hg2);
            ioerr += WRITE_EGR_PORT_64r(unit, lport, egr_port);

            ioerr += READ_PGW_XL_CONFIGr(unit, port, &pgw_xl_cfg);
            PGW_XL_CONFIGr_HIGIG_MODEf_SET(pgw_xl_cfg, hg);
            ioerr += WRITE_PGW_XL_CONFIGr(unit, port, pgw_xl_cfg);

            ioerr += READ_XLPORT_CONFIGr(unit, port, &xlport_cfg);
            XLPORT_CONFIGr_HIGIG_MODEf_SET(xlport_cfg, hg);
            XLPORT_CONFIGr_HIGIG2_MODEf_SET(xlport_cfg, hg2);
            ioerr += WRITE_XLPORT_CONFIGr(unit, port, xlport_cfg);

            ICONTROL_OPCODE_BITMAPr_SET(opcode_bmap, hg ? 1 : 0);
            ioerr += WRITE_ICONTROL_OPCODE_BITMAPr(unit, lport, opcode_bmap);

            /* HiGig ports require special egress tag action */
            ioerr += READ_EGR_VLAN_CONTROL_3r(unit, lport, &vctrl3);
            EGR_VLAN_CONTROL_3r_TAG_ACTION_PROFILE_PTRf_SET(vctrl3, hg ? 1 : 0);
            ioerr += WRITE_EGR_VLAN_CONTROL_3r(unit, lport, vctrl3);

            ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
            XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 1);
            XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
            XLMAC_CTRLr_LOCAL_LPBKf_SET(xlmac_ctrl, mac_lb);
            XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(xlmac_ctrl, 0);
            if (flags & HG_FLAGS) {
                XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(xlmac_ctrl, 1);
            }
            ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);
        } else if (CDK_PBMP_MEMBER(gpbmp, port)) {
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

#endif /* CDK_CONFIG_INCLUDE_BCM56160_A0 */
