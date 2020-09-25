/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56960_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>

#include <bmdi/bmd_port_mode.h>

#include <cdk/chip/bcm56960_a0_defs.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm56960_a0_bmd.h"
#include "bcm56960_a0_internal.h"

#define DRAIN_WAIT_MSEC                 500

/* Supported HiGig encapsulations */
#define HG_FLAGS        (BMD_PORT_MODE_F_HIGIG | BMD_PORT_MODE_F_HIGIG2)

#define MAC_SPEED_MODE_10G_PLUS      4

int
bcm56960_a0_bmd_port_mode_set(int unit, int port, 
                              bmd_port_mode_t mode, uint32_t flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int mac_lb = (flags & BMD_PORT_MODE_F_MAC_LOOPBACK) ? 1 : 0;
    int phy_lb = (flags & BMD_PORT_MODE_F_PHY_LOOPBACK) ? 1 : 0;
    int duplex = 1;
    uint32_t speed = 1000;
    int sp_sel = MAC_SPEED_MODE_10G_PLUS;
    int pref_intf = 0;
    int cnt, is_clport;
    int lport;
    int hg, hg2;
    int lanes;
    bmd_port_mode_t cur_mode;
    uint32_t cur_flags;
    uint32_t speed_max;
    uint32_t ipg_chk_disable;
    uint32_t pbm[PBM_LPORT_WORDS];
    cdk_pbmp_t clpbmp, xlpbmp;
    EPC_LINK_BMAPm_t epc_link;
    CLMAC_TXFIFO_CELL_CNTr_t clcell_cnt;
    CLMAC_CTRLr_t clmac_ctrl;
    CLMAC_MODEr_t clmac_mode;
    XLMAC_TXFIFO_CELL_CNTr_t xlcell_cnt;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_MODEr_t xlmac_mode;
    PORT_TABm_t port_tab;
    EGR_PORTm_t egr_port;
    EGR_ING_PORTm_t egr_ing_port;
    CLPORT_CONFIGr_t clport_cfg;
    XLPORT_CONFIGr_t xlport_cfg;
    ICONTROL_OPCODE_BITMAPm_t opcode_bmap;
    EGR_VLAN_CONTROL_3r_t vctrl3;
    CLMAC_EEE_CTRLr_t clmac_eee_ctrl;
    XLMAC_EEE_CTRLr_t xlmac_eee_ctrl;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    speed_max = bcm56960_a0_port_speed_max(unit, port);
    lanes = bcm56960_a0_port_lanes_get(unit, port);

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
        case bmdPortMode10000CR:
            speed = 10000;
            pref_intf = BMD_PHY_IF_CR;
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
        case bmdPortMode20000KR:
            speed = 20000;
            pref_intf = BMD_PHY_IF_KR;
            break;
        case bmdPortMode25000fd:
            speed = 25000;
            break;
        case bmdPortMode25000XFI:
            speed = 25000;
            pref_intf = BMD_PHY_IF_XFI;
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
        case bmdPortMode40000SR:
            speed = 40000;
            pref_intf = BMD_PHY_IF_SR;
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

    sp_sel = MAC_SPEED_MODE_10G_PLUS;

    if (speed > speed_max) {
        return CDK_E_PARAM;
    }

    if ((speed < 10000) || ((lanes == 4) && (speed < 40000))) {
        return CDK_E_PARAM;
    }

    bcm56960_a0_clport_pbmp_get(unit, &clpbmp);
    bcm56960_a0_xlport_pbmp_get(unit, &xlpbmp);
    if (CDK_PBMP_MEMBER(clpbmp, port)) {
        is_clport = 1;
    } else if (CDK_PBMP_MEMBER(xlpbmp, port)) {
        is_clport = 0;
    } else {
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

        /* Drain all packets from the Tx pipeline */
        cnt = DRAIN_WAIT_MSEC / 10;
        while (--cnt >= 0) {
            if (is_clport) {
                ioerr += READ_CLMAC_TXFIFO_CELL_CNTr(unit, port, &clcell_cnt);
                if (CLMAC_TXFIFO_CELL_CNTr_CELL_CNTf_GET(clcell_cnt) == 0) {
                    break;
                }
            } else {
                ioerr += READ_XLMAC_TXFIFO_CELL_CNTr(unit, port, &xlcell_cnt);
                if (XLMAC_TXFIFO_CELL_CNTr_CELL_CNTf_GET(xlcell_cnt) == 0) {
                    break;
                }
            }
            BMD_SYS_USLEEP(10000);
        }
        if (cnt < 0) {
            CDK_WARN(("bcm56960_a0_bmd_port_mode_set[%d]: "
                      "drain failed on port %d\n", unit, port));
        }
        /*
         * If HiGig/Ethernet encapsulation changes, we need 
         * to reinitialize the warpcore.
         */
        rv = bcm56960_a0_bmd_port_mode_get(unit, port, 
                                           &cur_mode, &cur_flags);
        if (CDK_SUCCESS(rv) && 
            ((flags ^ cur_flags) & HG_FLAGS)) {
            if (flags & HG_FLAGS) {
                BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_HG;
            } else {
                BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
            }
            if (is_clport) {
                rv = bcm56960_a0_clport_init(unit, port);
            } else {
                rv = bcm56960_a0_xlport_init(unit, port);
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

    if (is_clport) {
        /* Reset the MAC */
        ioerr += READ_CLMAC_CTRLr(unit, port, &clmac_ctrl);
        CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 1);
        ioerr += WRITE_CLMAC_CTRLr(unit, port, clmac_ctrl);
    
        /* Disable MACs (Rx only) */
        ioerr += READ_CLMAC_CTRLr(unit, port, &clmac_ctrl);
        CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, 0);
        ioerr += WRITE_CLMAC_CTRLr(unit, port, clmac_ctrl);
    } else {
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
            if (is_clport) {
                ioerr += READ_CLMAC_MODEr(unit, port, &clmac_mode);
            } else {
                ioerr += READ_XLMAC_MODEr(unit, port, &xlmac_mode);
            }
            ioerr += READ_PORT_TABm(unit, lport, &port_tab);
            ioerr += READ_EGR_PORTm(unit, lport, &egr_port);
            ioerr += READ_EGR_ING_PORTm(unit, lport, &egr_ing_port);
            if (is_clport) {
                ioerr += READ_CLPORT_CONFIGr(unit, port, &clport_cfg);
                /* MAC header mode */
                CLMAC_MODEr_HDR_MODEf_SET(clmac_mode, hg2 ? 2 : hg);
            } else {
                ioerr += READ_XLPORT_CONFIGr(unit, port, &xlport_cfg);
                /* MAC header mode */
                XLMAC_MODEr_HDR_MODEf_SET(xlmac_mode, hg2 ? 2 : hg);
            }
            /* Set IEEE vs HiGig */        
            PORT_TABm_PORT_TYPEf_SET(port_tab, hg);
            EGR_PORTm_PORT_TYPEf_SET(egr_port, hg);
            EGR_ING_PORTm_PORT_TYPEf_SET(egr_ing_port, hg);
            if (is_clport) {
                CLPORT_CONFIGr_HIGIG_MODEf_SET(clport_cfg, hg);
            } else {
                XLPORT_CONFIGr_HIGIG_MODEf_SET(xlport_cfg, hg);
            }
            ICONTROL_OPCODE_BITMAPm_SET(opcode_bmap, 0, hg ? 0x1 : 0x0);
            /* Set HiGig vs. HiGig2 */
            PORT_TABm_HIGIG2f_SET(port_tab, hg2);
            EGR_PORTm_HIGIG2f_SET(egr_port, hg2);
            EGR_ING_PORTm_HIGIG2f_SET(egr_ing_port, hg2);
            if (is_clport) {
                CLPORT_CONFIGr_HIGIG2_MODEf_SET(clport_cfg, hg2);
                ioerr += WRITE_CLMAC_MODEr(unit, port, clmac_mode);
            } else {
                XLPORT_CONFIGr_HIGIG2_MODEf_SET(xlport_cfg, hg2);
                ioerr += WRITE_XLMAC_MODEr(unit, port, xlmac_mode);
            }
            ioerr += WRITE_PORT_TABm(unit, lport, port_tab);
            ioerr += WRITE_EGR_PORTm(unit, lport, egr_port);
            ioerr += WRITE_EGR_ING_PORTm(unit, lport, egr_ing_port);
            if (is_clport) {
                ioerr += WRITE_CLPORT_CONFIGr(unit, port, clport_cfg);
            } else {
                ioerr += WRITE_XLPORT_CONFIGr(unit, port, xlport_cfg);
            }
            ioerr += WRITE_ICONTROL_OPCODE_BITMAPm(unit, lport, opcode_bmap);

            /* HiGig ports require special egress tag action */
            ioerr += READ_EGR_VLAN_CONTROL_3r(unit, lport, &vctrl3);
            EGR_VLAN_CONTROL_3r_TAG_ACTION_PROFILE_PTRf_SET(vctrl3, hg ? 1 : 0);
            ioerr += WRITE_EGR_VLAN_CONTROL_3r(unit, lport, vctrl3);

            if (is_clport) {
                /* Configure MAC mode */
                ioerr += READ_CLMAC_MODEr(unit, port, &clmac_mode);
                CLMAC_MODEr_SPEED_MODEf_SET(clmac_mode, sp_sel);
                ioerr += WRITE_CLMAC_MODEr(unit, port, clmac_mode);
                
                /* Configure EEE */
                ioerr += READ_CLMAC_EEE_CTRLr(unit, port, &clmac_eee_ctrl);            
                CLMAC_EEE_CTRLr_EEE_ENf_SET(clmac_eee_ctrl, 0);
                ioerr += WRITE_CLMAC_EEE_CTRLr(unit, port, clmac_eee_ctrl);                            
                if (flags & BMD_PORT_MODE_F_EEE) {
                    /* Enable IEEE 802.3az EEE */
                    CLMAC_EEE_CTRLr_EEE_ENf_SET(clmac_eee_ctrl, 1);
                    ioerr += WRITE_CLMAC_EEE_CTRLr(unit, port, clmac_eee_ctrl);                            
                }
            } else {
                /* Configure MAC mode */
                ioerr += READ_XLMAC_MODEr(unit, port, &xlmac_mode);
                XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, sp_sel);
                ioerr += WRITE_XLMAC_MODEr(unit, port, xlmac_mode);
                
                /* Configure EEE */
                ioerr += READ_XLMAC_EEE_CTRLr(unit, port, &xlmac_eee_ctrl);            
                XLMAC_EEE_CTRLr_EEE_ENf_SET(xlmac_eee_ctrl, 0);
                ioerr += WRITE_XLMAC_EEE_CTRLr(unit, port, xlmac_eee_ctrl);                            
                if (flags & BMD_PORT_MODE_F_EEE) {
                    /* Enable IEEE 802.3az EEE */
                    XLMAC_EEE_CTRLr_EEE_ENf_SET(xlmac_eee_ctrl, 1);
                    ioerr += WRITE_XLMAC_EEE_CTRLr(unit, port, xlmac_eee_ctrl);                            
                }
            }

            /* Reset EP credit before de-assert SOFT_RESET */
            ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, lport, &egr_port_credit_reset);
            EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
            ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, lport, egr_port_credit_reset);
            BMD_SYS_USLEEP(1000);
            ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, lport, &egr_port_credit_reset);
            EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
            ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, lport, egr_port_credit_reset);

            ipg_chk_disable = (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_HG) ? 1 : 0;
            if (is_clport) {
                /* Bring the MAC out of reset */
                ioerr += READ_CLMAC_CTRLr(unit, port, &clmac_ctrl);
                CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 0);
                CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, 1);
                CLMAC_CTRLr_TX_ENf_SET(clmac_ctrl, 1);
                CLMAC_CTRLr_LOCAL_LPBKf_SET(clmac_ctrl, mac_lb);
                CLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(clmac_ctrl, ipg_chk_disable);
                ioerr += WRITE_CLMAC_CTRLr(unit, port, clmac_ctrl);
            } else {
                /* Bring the MAC out of reset */
                ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
                XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
                XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 1);
                XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
                XLMAC_CTRLr_LOCAL_LPBKf_SET(xlmac_ctrl, mac_lb);
                XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(xlmac_ctrl, ipg_chk_disable);
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
#endif /* CDK_CONFIG_INCLUDE_BCM56960_A0 */

