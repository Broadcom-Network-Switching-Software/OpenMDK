/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56260_B0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>

#include <bmdi/bmd_port_mode.h>
#include <bmd/bmd_phy_ctrl.h>

#include <cdk/chip/bcm56260_b0_defs.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm56260_b0_bmd.h"
#include "bcm56260_b0_internal.h"

#define NUM_COS                         8

/* Supported HiGig encapsulations */
#define HG_FLAGS  (BMD_PORT_MODE_F_HIGIG | BMD_PORT_MODE_F_HIGIG2 | BMD_PORT_MODE_F_HGLITE)

int
bcm56260_b0_bmd_port_mode_set(int unit, int port,
                              bmd_port_mode_t mode, uint32_t flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int mac_lb = (flags & BMD_PORT_MODE_F_MAC_LOOPBACK) ? 1 : 0;
    int phy_lb = (flags & BMD_PORT_MODE_F_PHY_LOOPBACK) ? 1 : 0;
    int duplex = 1;
    int sp_sel = COMMAND_CONFIG_SPEED_1000;
    int lport;
    int ppport;
    uint32_t speed = 0;
    uint32_t speed_max;
    int pref_intf = 0;
    uint32_t pbm;
    EPC_LINK_BMAPm_t epc_link;
    XLMAC_CTRLr_t xlmac_ctrl;
    cdk_pbmp_t mxqport_pbmp, xlport_pbmp, pbmp_all;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);
    speed_max = bcm56260_a0_port_speed_max(unit, port);
    if (flags & HG_FLAGS) {
        if (speed_max <= 10000) {
            /* HiGig not supported on single lane ports */
            return CDK_E_PARAM;
        }
        pref_intf = BMD_PHY_IF_HIGIG;
    }
    bcm56260_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
    bcm56260_a0_xlport_pbmp_get(unit, &xlport_pbmp);
    pbmp_all = mxqport_pbmp;
    CDK_PBMP_OR(pbmp_all, xlport_pbmp);
    if (CDK_PBMP_MEMBER(pbmp_all, port)) {
        switch (mode) {
        case bmdPortModeAuto:
        case bmdPortModeDisabled:
            speed = speed_max;
            break;
        case bmdPortMode1000fd:
            speed = 1000;
            sp_sel = COMMAND_CONFIG_SPEED_1000;
            break;
        case bmdPortMode2500fd:
            speed = 2500;
            sp_sel = COMMAND_CONFIG_SPEED_2500;
            break;
        case bmdPortMode10000fd:
        case bmdPortMode10000XFI:
        case bmdPortMode10000KR:
            speed = 10000;
            sp_sel = COMMAND_CONFIG_SPEED_10000;
            break;
        case bmdPortMode10000SFI:
            speed = 10000;
            pref_intf = BMD_PHY_IF_SFI;
            break;
        case bmdPortMode20000fd:
            speed = 20000;
            break;
        case bmdPortMode40000fd:
            speed = 40000;
            break;
        case bmdPortMode40000KR:
            speed = 40000;
            pref_intf = BMD_PHY_IF_KR;
            break;
        case bmdPortMode40000CR:
            speed = 40000;
            pref_intf = BMD_PHY_IF_CR;
            break;
        case bmdPortMode100000CR:
            speed = 100000;
            pref_intf = BMD_PHY_IF_CR;
            break;
#if BMD_CONFIG_INCLUDE_HIGIG == 1
        case bmdPortMode11000fd:
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
        case bmdPortMode25000fd:
            if (flags & HG_FLAGS) {
                speed = 25000;
            }
            break;
        case bmdPortMode30000fd:
            if (flags & HG_FLAGS) {
                speed = 30000;
            }
            break;
        case bmdPortMode42000fd:
            speed = 42000;
            break;
        case bmdPortMode127000fd:
            speed = 127000;
            break;
#endif
        default:
            break;
        }
    }
    /* If no XAUI mode was selected, check SerDes modes */
    if (speed == 0) {
        if (speed_max > 10000) {
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
            speed = 1000;
            sp_sel = COMMAND_CONFIG_SPEED_1000;
            break;
        case bmdPortMode2500fd:
            speed = 2500;
            sp_sel = COMMAND_CONFIG_SPEED_2500;
            break;
        case bmdPortModeAuto:
            break;
        case bmdPortModeDisabled:
            break;
        default:
            return CDK_E_PARAM;
        }
    }
    if (speed > speed_max) {
        return CDK_E_PARAM;
    }
    ppport = P2PP(unit, port);
    lport = P2L(unit, port);
    if ((flags & BMD_PORT_MODE_F_INTERNAL) == 0) {
        /* Set preferred line interface */
        bmd_phy_line_interface_set(unit, port, pref_intf);

        /* Stop CPU and MMU from scheduling packets to the port */
        BMD_PORT_STATUS_CLR(unit, port, BMD_PST_LINK_UP);
        ioerr += READ_EPC_LINK_BMAPm(unit, 0, &epc_link);
        pbm = EPC_LINK_BMAPm_GET(epc_link, (ppport >> 5));
        pbm &= ~LSHIFT32(1, ppport & 0x1f);
        EPC_LINK_BMAPm_SET(epc_link, (ppport >> 5), pbm);
        ioerr += WRITE_EPC_LINK_BMAPm(unit, 0, epc_link);

        bcm56260_b0_mac_xl_drain_cells(unit, port);

#if BMD_CONFIG_INCLUDE_HIGIG == 1
        if (BMD_PORT_PROPERTIES(unit, port) & ((BMD_PORT_HG) | (BMD_PORT_XE))) {
            bmd_port_mode_t cur_mode;
            uint32_t cur_flags;

            /*
             * If HiGig/Ethernet encapsulation changes, we need
             * to reinitialize the serdes.
             */
            rv = bcm56260_b0_bmd_port_mode_get(unit, port,
                                               &cur_mode, &cur_flags);

            if (CDK_SUCCESS(rv) && ((flags ^ cur_flags) & HG_FLAGS)) {
                if (flags & HG_FLAGS) {
                    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_HG;
                } else {
                    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
                }
            }
        }
#endif
    }

   /* Update PHYs before MAC */
    if (CDK_SUCCESS(rv)) {
        rv = bmd_port_mode_to_phy(unit, port, mode, flags, speed, duplex);
    }

    /* Let PHYs know that we disable the MAC */
    if (CDK_SUCCESS(rv)) {
        rv = bmd_phy_notify_mac_enable(unit, port, 0);
    }

    ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 0);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);
    BMD_SYS_USLEEP(1000);

    if (mode == bmdPortModeDisabled) {
        BMD_PORT_STATUS_SET(unit, port, BMD_PST_FORCE_LINK);
    } else {
        int hg, hg2;
        int phy_mode, num_lanes;
        XPORT_MODE_REGr_t xport_mode;
        XLMAC_MODEr_t xlmac_mode;
        EGR_PHYSICAL_PORTm_t egr_port;
        ING_PHYSICAL_PORT_TABLEm_t ing_port;
        XLPORT_CONFIGr_t xlport_cfg;
        XPORT_CONFIGr_t xport_cfg;
        ICONTROL_OPCODE_BITMAPm_t opcode_bmap;
        EGR_VLAN_CONTROL_3r_t vctrl3;
        XLMAC_EEE_CTRLr_t xlmac_eee_ctrl;
        XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
        XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
        int phy_mode_xl, core_mode_xl;
        XLPORT_MODE_REGr_t xlport_mode_reg;

        /* Get the phy_mode and num_lanes flag */
        if (CDK_PBMP_MEMBER(mxqport_pbmp, port)) {
            if (CDK_SUCCESS(rv)) {
                rv = bcm56260_a0_mxq_phy_mode_get(unit, port, speed_max, &phy_mode, &num_lanes);

                ioerr += READ_XPORT_MODE_REGr(unit, &xport_mode, port);
                XPORT_MODE_REGr_PHY_PORT_MODEf_SET(xport_mode, phy_mode);
                XPORT_MODE_REGr_PORT_GMII_MII_ENABLEf_SET(xport_mode, ((speed >= 10000) ? 0 : 1));
                ioerr += WRITE_XPORT_MODE_REGr(unit, xport_mode, port);
            }
        }

        if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
            if (CDK_SUCCESS(rv)) {
                rv = bcm56260_a0_xl_phy_core_port_mode(unit, port,
                                        &phy_mode_xl, &core_mode_xl);

                ioerr += READ_XLPORT_MODE_REGr(unit, &xlport_mode_reg, port);
                XLPORT_MODE_REGr_EGR_1588_TIMESTAMPING_CMIC_48_ENf_SET(xlport_mode_reg, 1);
                XLPORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(xlport_mode_reg, core_mode_xl);
                XLPORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(xlport_mode_reg, phy_mode_xl);
                ioerr += WRITE_XLPORT_MODE_REGr(unit, xlport_mode_reg, port);
            }
        }

        /* Set encapsulation */
        hg = hg2 = 0;
#if BMD_CONFIG_INCLUDE_HIGIG == 1
        if (flags & HG_FLAGS) {
            hg = 1;
            if ((flags & BMD_PORT_MODE_F_HIGIG2) || (flags & BMD_PORT_MODE_F_HGLITE)) {
                hg2 = 1;
            }
        }
#endif /* BMD_CONFIG_INCLUDE_HIGIG */

        if (CDK_PBMP_MEMBER(pbmp_all, port)) {
            ioerr += READ_XLMAC_MODEr(unit, port, &xlmac_mode);
            XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, sp_sel);
            if (speed <= 10000) {
                XLMAC_MODEr_HDR_MODEf_SET(xlmac_mode, 0);
            } else {
                XLMAC_MODEr_HDR_MODEf_SET(xlmac_mode, hg2 ? 2 : hg);
            }
            ioerr += WRITE_XLMAC_MODEr(unit, port, xlmac_mode);

            if (CDK_PBMP_MEMBER(mxqport_pbmp, port)) {
                if ((flags & BMD_PORT_MODE_F_INTERNAL) == 0) {
                    bcm56260_b0_mac_xl_drain_cells(unit, port);
                }
            }

            BMD_SYS_USLEEP(10000);

            ioerr += READ_EGR_PHYSICAL_PORTm(unit, lport, &egr_port);
            EGR_PHYSICAL_PORTm_PORT_TYPEf_SET(egr_port, hg);
            EGR_PHYSICAL_PORTm_HIGIG2f_SET(egr_port, hg2);
            ioerr += WRITE_EGR_PHYSICAL_PORTm(unit, lport, egr_port);

            ioerr += READ_ING_PHYSICAL_PORT_TABLEm(unit, lport, &ing_port);
            ING_PHYSICAL_PORT_TABLEm_PORT_TYPEf_SET(ing_port, hg);
            ING_PHYSICAL_PORT_TABLEm_HIGIG2f_SET(ing_port, hg2);
            ioerr += WRITE_ING_PHYSICAL_PORT_TABLEm(unit, lport, ing_port);
        }

        if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
            ioerr += READ_XLPORT_CONFIGr(unit, port, &xlport_cfg);
            XLPORT_CONFIGr_HIGIG_MODEf_SET(xlport_cfg, hg);
            XLPORT_CONFIGr_HIGIG2_MODEf_SET(xlport_cfg, hg2);
            ioerr += WRITE_XLPORT_CONFIGr(unit, port, xlport_cfg);
        } else {
            ioerr += READ_XPORT_CONFIGr(unit, port, &xport_cfg);
            XPORT_CONFIGr_HIGIG_MODEf_SET(xport_cfg, hg);
            XPORT_CONFIGr_HIGIG2_MODEf_SET(xport_cfg, hg2);
            ioerr += WRITE_XPORT_CONFIGr(unit, port, xport_cfg);
        }

        ICONTROL_OPCODE_BITMAPm_SET(opcode_bmap, 0, hg ? 0x1 : 0x0);
        ICONTROL_OPCODE_BITMAPm_SET(opcode_bmap, 1, 0x0);
        ioerr += WRITE_ICONTROL_OPCODE_BITMAPm(unit, ppport, opcode_bmap);

        /* HiGig ports require special egress tag action */
        ioerr += READ_EGR_VLAN_CONTROL_3r(unit, lport, &vctrl3);
        EGR_VLAN_CONTROL_3r_TAG_ACTION_PROFILE_PTRf_SET(vctrl3, hg ? 1 : 0);
        ioerr += WRITE_EGR_VLAN_CONTROL_3r(unit, lport, vctrl3);

        /* Disable Strip CRC */
        ioerr += READ_XLMAC_RX_CTRLr(unit, port, &xlmac_rx_ctrl);
        XLMAC_RX_CTRLr_RUNT_THRESHOLDf_SET(xlmac_rx_ctrl, 64);
        XLMAC_RX_CTRLr_STRIP_CRCf_SET(xlmac_rx_ctrl, 0);
        ioerr += WRITE_XLMAC_RX_CTRLr(unit, port, xlmac_rx_ctrl);
        BMD_SYS_USLEEP(10000);

        ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
        XLMAC_TX_CTRLr_CRC_MODEf_SET(xlmac_tx_ctrl, 3);
        ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);
        BMD_SYS_USLEEP(10000);

        /* Configure 10G MAC */
        ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
        XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
        ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);
        BMD_SYS_USLEEP(10000);

        ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
        XLMAC_CTRLr_LOCAL_LPBKf_SET(xlmac_ctrl, mac_lb);
        XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 1);
        XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
        XLMAC_CTRLr_EXTENDED_HIG2_ENf_SET(xlmac_ctrl, 1);
        XLMAC_CTRLr_SW_LINK_STATUSf_SET(xlmac_ctrl, 1);
        ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);
        BMD_SYS_USLEEP(10000);

        /* Configure EEE */
        ioerr += READ_XLMAC_EEE_CTRLr(unit, port, &xlmac_eee_ctrl);
        XLMAC_EEE_CTRLr_EEE_ENf_SET(xlmac_eee_ctrl, 0);
        ioerr += WRITE_XLMAC_EEE_CTRLr(unit, port, xlmac_eee_ctrl);
        if (flags & BMD_PORT_MODE_F_EEE) {
            /* Enable IEEE 802.3az EEE */
            XLMAC_EEE_CTRLr_EEE_ENf_SET(xlmac_eee_ctrl, 1);
            ioerr += WRITE_XLMAC_EEE_CTRLr(unit, port, xlmac_eee_ctrl);
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
#endif /* CDK_CONFIG_INCLUDE_BCM56260_B0 */
