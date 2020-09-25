/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56670_A0 == 1

#include <bmd/bmd.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/chip/bcm56670_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm56670_a0_bmd.h"
#include "bcm56670_a0_internal.h"

#define DRAIN_WAIT_MSEC         500

#define XMAC_MODE_10M           0
#define XMAC_MODE_100M          1
#define XMAC_MODE_1G            2
#define XMAC_MODE_2G5           3
#define XMAC_MODE_10G_PLUS      4

/* Supported HiGig encapsulations */
#define HG_FLAGS        (BMD_PORT_MODE_F_HIGIG | BMD_PORT_MODE_F_HIGIG2)


void
bcm56670_a0_xlmac_drain_cell(unit, port)
{
    int ioerr = 0;
    int cnt;
    XLMAC_TXFIFO_CELL_CNTr_t xlcell_cnt;

    cnt = DRAIN_WAIT_MSEC / 10;
    while (--cnt >= 0) {
        ioerr += READ_XLMAC_TXFIFO_CELL_CNTr(unit, port, &xlcell_cnt);
        if (XLMAC_TXFIFO_CELL_CNTr_CELL_CNTf_GET(xlcell_cnt) == 0) {
            break;
        }
        BMD_SYS_USLEEP(10000);
    }
    if (cnt < 0) {
        CDK_WARN(("bcm56670_a0_bmd_port_mode_set[%d]: "
                  "drain failed on port %d\n", unit, port));
    }
    BMD_SYS_USLEEP(10000);

    return;
}

void
bcm56670_a0_clmac_drain_cell(unit, port)
{
    int ioerr = 0;
    int cnt;
    CLMAC_TXFIFO_CELL_CNTr_t xlcell_cnt;

    cnt = DRAIN_WAIT_MSEC / 10;
    while (--cnt >= 0) {
        ioerr += READ_CLMAC_TXFIFO_CELL_CNTr(unit, port, &xlcell_cnt);
        if (CLMAC_TXFIFO_CELL_CNTr_CELL_CNTf_GET(xlcell_cnt) == 0) {
            break;
        }
        BMD_SYS_USLEEP(10000);
    }
    if (cnt < 0) {
        CDK_WARN(("bcm56670_a0_bmd_port_mode_set[%d]: "
                  "drain failed on port %d\n", unit, port));
    }
    BMD_SYS_USLEEP(10000);

    return;
}

int
bcm56670_a0_bmd_port_mode_set(int unit, int port,
                              bmd_port_mode_t mode, uint32_t flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int mac_lb = (flags & BMD_PORT_MODE_F_MAC_LOOPBACK) ? 1 : 0;
    int phy_lb = (flags & BMD_PORT_MODE_F_PHY_LOOPBACK) ? 1 : 0;
    int duplex = 1;
    uint32_t speed = 1000;
    int sp_sel = XMAC_MODE_1G;
    int pref_intf = 0;
    int lport;
    int hg, hg2;
    bmd_port_mode_t cur_mode;
    uint32_t cur_flags;
    uint32_t speed_max;
    uint32_t pbm[PBM_LPORT_WORDS];
    EPC_LINK_BMAPm_t epc_link;
    XLMAC_CTRLr_t xlmac_ctrl;
    CLMAC_CTRLr_t clmac_ctrl;
    XLMAC_MODEr_t xlmac_mode;
    CLMAC_MODEr_t clmac_mode;
    PORT_TABm_t port_tab;
    EGR_PORTm_t egr_port;
    EGR_ING_PORTm_t egr_ing_port;
    XLPORT_CONFIGr_t xlport_cfg;
    CLPORT_CONFIGr_t clport_cfg;
    ICONTROL_OPCODE_BITMAPm_t opcode_bmap;
    EGR_VLAN_CONTROL_3m_t vctrl3;
    XLMAC_EEE_CTRLr_t xlmac_eee_ctrl;
    CLMAC_EEE_CTRLr_t clmac_eee_ctrl;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;
    EGR_ENABLEm_t egr_enable;
    cdk_pbmp_t clpbmp, xlpbmp;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    speed_max = bcm56670_a0_port_speed_max(unit, port);
    bcm56670_a0_clport_pbmp_get(unit, &clpbmp);
    bcm56670_a0_xlport_pbmp_get(unit, &xlpbmp);
    if (flags & HG_FLAGS) {
        pref_intf = BMD_PHY_IF_HIGIG;
    }

    if (BMD_PORT_PROPERTIES(unit, port) & (BMD_PORT_HG | BMD_PORT_XE)) {
        switch (mode) {
        case bmdPortModeAuto:
        case bmdPortModeDisabled:
            speed = speed_max;
            break;
        case bmdPortMode10000fd:
         case bmdPortMode10000XFI:
            speed = 10000;
            break;
         case bmdPortMode10000KR:
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
        case bmdPortMode100000fd:
            speed = 100000;
            break;
        case bmdPortMode100000CR:
            speed = 100000;
            pref_intf = BMD_PHY_IF_CR;
            break;
        case bmdPortMode100000KR:
            speed = 100000;
            pref_intf = BMD_PHY_IF_KR;
            break;
        case bmdPortMode100000SR:
            speed = 100000;
            pref_intf = BMD_PHY_IF_SR;
            break;
#if BMD_CONFIG_INCLUDE_HIGIG == 1
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
        case bmdPortMode30000fd:
            if (flags & HG_FLAGS) {
                speed = 30000;
            }
            break;
#endif
        default:
            break;
        }
    }

    if (speed_max >= 100000 && speed < 100000) {
         /* Support 100G speed set for 100G triple-core now */
         return CDK_E_PARAM;
    }

    /* If no XAUI mode was selected, check SerDes modes */
    if (speed == 1000) {
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
            sp_sel = XMAC_MODE_10M;
            break;
        case bmdPortMode100fd:
        case bmdPortMode100hd:
            speed = 100;
            sp_sel = XMAC_MODE_100M;
            break;
        case bmdPortMode1000fd:
        case bmdPortMode1000hd:
            speed = 1000;
            sp_sel = XMAC_MODE_1G;
            break;
        case bmdPortMode2500fd:
            speed = 2500;
            sp_sel = XMAC_MODE_2G5;
        case bmdPortModeAuto:
            break;
        case bmdPortModeDisabled:
            break;
        default:
            return CDK_E_PARAM;
        }
    }

    if (speed >= 10000) {
        sp_sel = XMAC_MODE_10G_PLUS;
    }

    if (speed > speed_max) {
        return CDK_E_PARAM;
    }

    if (speed_max > 10000 && speed < 1000) {
        return CDK_E_PARAM;
    }

    lport = P2L(unit, port);

    if ((flags & BMD_PORT_MODE_F_INTERNAL) == 0) {
        /* Set preferred line interface */
        bmd_phy_line_interface_set(unit, port, pref_intf);

        /* Stop CPU and MMU from scheduling packets to the port */
        BMD_PORT_STATUS_CLR(unit, port, BMD_PST_LINK_UP);
        ioerr += READ_EPC_LINK_BMAPm(unit, 0, &epc_link);
        EPC_LINK_BMAPm_PORT_BITMAPf_GET(epc_link, pbm);
        PBM_PORT_REMOVE(pbm, lport);
        EPC_LINK_BMAPm_PORT_BITMAPf_SET(epc_link, pbm);
        ioerr += WRITE_EPC_LINK_BMAPm(unit, 0, epc_link);

        if (mac_lb || phy_lb) {
            if (CDK_PBMP_MEMBER(clpbmp, port)) {
                bcm56670_a0_clmac_drain_cell(unit, port);
            } else if (CDK_PBMP_MEMBER(xlpbmp, port)) {
                bcm56670_a0_xlmac_drain_cell(unit, port);
            }
        }

        BMD_SYS_USLEEP(10000);
        /*
         * If HiGig/Ethernet encapsulation changes, we need
         * to reinitialize the warpcore.
         */
        rv = bcm56670_a0_bmd_port_mode_get(unit, port,
                                           &cur_mode, &cur_flags);
        if (CDK_SUCCESS(rv) &&
            ((flags ^ cur_flags) & HG_FLAGS)) {
            if (flags & HG_FLAGS) {
                BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_HG;
            } else {
                BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
            }
            if (CDK_PBMP_MEMBER(clpbmp, port)) {
                rv = bcm56670_a0_clport_init(unit, port);
            } else if (CDK_PBMP_MEMBER(xlpbmp, port)) {
                rv = bcm56670_a0_xlport_init(unit, port);
            }
        }
    }

    /* Update PHYs before MAC */
    if (CDK_SUCCESS(rv)) {
        rv = bmd_port_mode_to_phy(unit, port, mode, flags, speed, duplex);
    }

    /* Let PHYs know that we disable the MAC */
    if (CDK_SUCCESS(rv)) {
        rv = bmd_phy_notify_mac_enable(unit, port, 0);
    }

    if (CDK_PBMP_MEMBER(clpbmp, port)) {
        /* Reset the CLMAC */
        ioerr += READ_CLMAC_CTRLr(unit, port, &clmac_ctrl);
        CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 1);
        ioerr += WRITE_CLMAC_CTRLr(unit, port, clmac_ctrl);

        /* Disable CLMACs (Rx only) */
        ioerr += READ_CLMAC_CTRLr(unit, port, &clmac_ctrl);
        CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, 0);
        ioerr += WRITE_CLMAC_CTRLr(unit, port, clmac_ctrl);
    } else if (CDK_PBMP_MEMBER(xlpbmp, port)) {
        /* Reset the MAC */
        ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
        XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 1);
        ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

        /* Disable MACs (Rx only) */
        ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
        XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 0);
        ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);
    }

    if (mode == bmdPortModeDisabled) {
        BMD_PORT_STATUS_SET(unit, port, BMD_PST_FORCE_LINK | BMD_PST_DISABLED);
    } else {
        BMD_PORT_STATUS_CLR(unit, port, BMD_PST_DISABLED);
        if (BMD_PORT_PROPERTIES(unit, port) & (BMD_PORT_HG | BMD_PORT_XE)) {
            /* Set encapsulation */
            hg = hg2 = 0;
#if BMD_CONFIG_INCLUDE_HIGIG == 1
            if (flags & HG_FLAGS) {
                hg = 1;
                if (flags & BMD_PORT_MODE_F_HIGIG2) {
                    hg2 = 1;
                }
            }
#endif
            if (CDK_PBMP_MEMBER(clpbmp, port)) {
                ioerr += READ_CLMAC_MODEr(unit, port, &clmac_mode);
                ioerr += READ_CLPORT_CONFIGr(unit, port, &clport_cfg);
            } else if (CDK_PBMP_MEMBER(xlpbmp, port)) {
                ioerr += READ_XLMAC_MODEr(unit, port, &xlmac_mode);
                ioerr += READ_XLPORT_CONFIGr(unit, port, &xlport_cfg);
            }
            ioerr += READ_PORT_TABm(unit, lport, &port_tab);
            ioerr += READ_EGR_PORTm(unit, lport, &egr_port);
            ioerr += READ_EGR_ING_PORTm(unit, lport, &egr_ing_port);
            ioerr += READ_ICONTROL_OPCODE_BITMAPm(unit, lport, &opcode_bmap);

            /* MAC header mode */
            if (CDK_PBMP_MEMBER(clpbmp, port)) {
                CLMAC_MODEr_HDR_MODEf_SET(clmac_mode, hg2 ? 2 : hg);
            } else if (CDK_PBMP_MEMBER(xlpbmp, port)) {
                XLMAC_MODEr_HDR_MODEf_SET(xlmac_mode, hg2 ? 2 : hg);
            }

            /* Set IEEE vs HiGig */
            PORT_TABm_PORT_TYPEf_SET(port_tab, hg);
            EGR_PORTm_PORT_TYPEf_SET(egr_port, hg);
            EGR_ING_PORTm_PORT_TYPEf_SET(egr_ing_port, hg);
            if (CDK_PBMP_MEMBER(clpbmp, port)) {
                CLPORT_CONFIGr_HIGIG_MODEf_SET(clport_cfg, hg);
            } else if (CDK_PBMP_MEMBER(xlpbmp, port)) {
                XLPORT_CONFIGr_HIGIG_MODEf_SET(xlport_cfg, hg);
            }
            ICONTROL_OPCODE_BITMAPm_SET(opcode_bmap, 0, hg ? 0x1 : 0x0);

            /* Set HiGig vs. HiGig2 */
            PORT_TABm_HIGIG2f_SET(port_tab, hg2);
            EGR_PORTm_HIGIG2f_SET(egr_port, hg2);
            EGR_ING_PORTm_HIGIG2f_SET(egr_ing_port, hg2);

            if (CDK_PBMP_MEMBER(clpbmp, port)) {
                CLPORT_CONFIGr_HIGIG2_MODEf_SET(clport_cfg, hg2);
                ioerr += WRITE_CLMAC_MODEr(unit, port, clmac_mode);
                ioerr += WRITE_CLPORT_CONFIGr(unit, port, clport_cfg);
            } else if (CDK_PBMP_MEMBER(xlpbmp, port)) {
                XLPORT_CONFIGr_HIGIG2_MODEf_SET(xlport_cfg, hg2);
                ioerr += WRITE_XLMAC_MODEr(unit, port, xlmac_mode);
                ioerr += WRITE_XLPORT_CONFIGr(unit, port, xlport_cfg);
            }

            ioerr += WRITE_PORT_TABm(unit, lport, port_tab);
            ioerr += WRITE_EGR_PORTm(unit, lport, egr_port);
            ioerr += WRITE_EGR_ING_PORTm(unit, lport, egr_ing_port);
            ioerr += WRITE_ICONTROL_OPCODE_BITMAPm(unit, lport, opcode_bmap);

            /* HiGig ports require special egress tag action */
            ioerr += READ_EGR_VLAN_CONTROL_3m(unit, lport, &vctrl3);
            EGR_VLAN_CONTROL_3m_TAG_ACTION_PROFILE_PTRf_SET(vctrl3, hg ? 1 : 0);
            ioerr += WRITE_EGR_VLAN_CONTROL_3m(unit, lport, vctrl3);

            if (CDK_PBMP_MEMBER(clpbmp, port)) {
                /* Configure MAC mode */
                ioerr += READ_CLMAC_MODEr(unit, port, &clmac_mode);
                CLMAC_MODEr_SPEED_MODEf_SET(clmac_mode, sp_sel);
                ioerr += WRITE_CLMAC_MODEr(unit, port, clmac_mode);

                /* Configure EEE */
                ioerr += READ_CLMAC_EEE_CTRLr(unit, port, &clmac_eee_ctrl);
                if (flags & BMD_PORT_MODE_F_EEE) {
                    /* Enable IEEE 802.3az EEE */
                    CLMAC_EEE_CTRLr_EEE_ENf_SET(clmac_eee_ctrl, 1);
                } else {
                    CLMAC_EEE_CTRLr_EEE_ENf_SET(clmac_eee_ctrl, 0);
                }
                ioerr += WRITE_CLMAC_EEE_CTRLr(unit, port, clmac_eee_ctrl);
            } else if (CDK_PBMP_MEMBER(xlpbmp, port)) {
                /* Configure MAC mode */
                ioerr += READ_XLMAC_MODEr(unit, port, &xlmac_mode);
                XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, sp_sel);
                ioerr += WRITE_XLMAC_MODEr(unit, port, xlmac_mode);

                /* Configure EEE */
                ioerr += READ_XLMAC_EEE_CTRLr(unit, port, &xlmac_eee_ctrl);
                if (flags & BMD_PORT_MODE_F_EEE) {
                    /* Enable IEEE 802.3az EEE */
                    XLMAC_EEE_CTRLr_EEE_ENf_SET(xlmac_eee_ctrl, 1);
                } else {
                    XLMAC_EEE_CTRLr_EEE_ENf_SET(xlmac_eee_ctrl, 0);
                }
                ioerr += WRITE_XLMAC_EEE_CTRLr(unit, port, xlmac_eee_ctrl);
            }

            /* Reset EP credit before de-assert SOFT_RESET */
            ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, port, &egr_port_credit_reset);
            EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
            ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port, egr_port_credit_reset);
            BMD_SYS_USLEEP(1000);
            ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, port, &egr_port_credit_reset);
            EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
            ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port, egr_port_credit_reset);
            BMD_SYS_USLEEP(1000);

            ioerr += bcm56670_a0_egr_buf_reset(unit, port, 0);
            ioerr += READ_EGR_ENABLEm(unit, port, &egr_enable);
            EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
            ioerr += WRITE_EGR_ENABLEm(unit, port, egr_enable);

            /* Bring the MAC out of reset */
            if (CDK_PBMP_MEMBER(clpbmp, port)) {
                ioerr += READ_CLMAC_CTRLr(unit, port, &clmac_ctrl);
                CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 0);
                CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, 1);
                CLMAC_CTRLr_TX_ENf_SET(clmac_ctrl, 1);
                CLMAC_CTRLr_LOCAL_LPBKf_SET(clmac_ctrl, mac_lb);
                if (flags & HG_FLAGS) {
                    CLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(clmac_ctrl, 1);
                } else {
                    CLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(clmac_ctrl, 0);
                }
                ioerr += WRITE_CLMAC_CTRLr(unit, port, clmac_ctrl);
            } else if (CDK_PBMP_MEMBER(xlpbmp, port)) {
                ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
                XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
                XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 1);
                XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
                XLMAC_CTRLr_LOCAL_LPBKf_SET(xlmac_ctrl, mac_lb);
                if (flags & HG_FLAGS) {
                    XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(xlmac_ctrl, 1);
                } else {
                    XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(xlmac_ctrl, 0);
                }
                ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);
            }
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
#endif /* CDK_CONFIG_INCLUDE_BCM56670_A0 */

