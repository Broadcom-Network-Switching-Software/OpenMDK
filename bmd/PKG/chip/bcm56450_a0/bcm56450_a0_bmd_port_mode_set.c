#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56450_A0 == 1

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

#include <cdk/chip/bcm56450_a0_defs.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm56450_a0_bmd.h"
#include "bcm56450_a0_internal.h"

#define DRAIN_WAIT_MSEC                 500

/* Supported HiGig encapsulations */
#define HG_FLAGS  (BMD_PORT_MODE_F_HIGIG | BMD_PORT_MODE_F_HIGIG2 | BMD_PORT_MODE_F_HGLITE)

int
bcm56450_a0_bmd_port_mode_set(int unit, int port, 
                              bmd_port_mode_t mode, uint32_t flags)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int mac_lb = (flags & BMD_PORT_MODE_F_MAC_LOOPBACK) ? 1 : 0;
    int phy_lb = (flags & BMD_PORT_MODE_F_PHY_LOOPBACK) ? 1 : 0;
    int duplex = 1;
    int pref_intf = 0;
    int sp_sel = XMAC_MODE_SPEED_10000;
    int ppport, lport, cnt;
    uint32_t pbm;
    uint32_t speed = 1000;
    uint32_t speed_max;
    EPC_LINK_BMAPm_t epc_link;
    XP_TXFIFO_PKT_DROP_CTLr_t drop_ctl;
    XP_TXFIFO_CELL_CNTr_t cell_cnt;
    XMAC_CTRLr_t xmac_ctrl;
    
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    speed_max = bcm56450_a0_port_speed_max(unit, port);

    if (flags & HG_FLAGS) {
        if (speed_max <= 10000) {
            /* HiGig not supported on single lane ports */
            return CDK_E_PARAM;
        }
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
        case bmdPortMode10000KR:
            speed = 10000;
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
            sp_sel = XMAC_MODE_SPEED_10;
            break;
        case bmdPortMode100fd:
        case bmdPortMode100hd:
            speed = 100;
            sp_sel = XMAC_MODE_SPEED_100;
            break;
        case bmdPortMode1000fd:
        case bmdPortMode1000hd:
            speed = 1000;
            sp_sel = XMAC_MODE_SPEED_1000;
            break;
        case bmdPortMode2500fd:
            speed = 2500;
            sp_sel = XMAC_MODE_SPEED_2500;
            break;
        case bmdPortModeAuto:
            break;
        case bmdPortModeDisabled:
            break;
        default:
            return CDK_E_PARAM;
        }
    } else {
        sp_sel = XMAC_MODE_SPEED_10000;
    }

    if (speed > speed_max) {
        return CDK_E_PARAM;
    }

    if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_GE) {
        if (speed > 2500) {
            return CDK_E_PARAM;
        }
    } else {
        if (speed < 100) {
            return CDK_E_PARAM;
        }
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

        /* Drain all packets from the Tx pipeline */
        ioerr += READ_XP_TXFIFO_PKT_DROP_CTLr(unit, port, &drop_ctl);
        XP_TXFIFO_PKT_DROP_CTLr_DROP_ENf_SET(drop_ctl, 1);
        ioerr += WRITE_XP_TXFIFO_PKT_DROP_CTLr(unit, port, drop_ctl);
        cnt = DRAIN_WAIT_MSEC / 10;
        while (--cnt >= 0) {
            ioerr += READ_XP_TXFIFO_CELL_CNTr(unit, port, &cell_cnt);
            if (XP_TXFIFO_CELL_CNTr_GET(cell_cnt) == 0) {
                break;
            }
            BMD_SYS_USLEEP(10000);
        }
        if (cnt < 0) {
            CDK_WARN(("bcm56450_a0_bmd_port_mode_set[%d]: "
                      "drain failed on port %d\n", unit, port));
        }
        XP_TXFIFO_PKT_DROP_CTLr_DROP_ENf_SET(drop_ctl, 0);
        ioerr += WRITE_XP_TXFIFO_PKT_DROP_CTLr(unit, port, drop_ctl);

#if BMD_CONFIG_INCLUDE_HIGIG == 1
        if (BMD_PORT_PROPERTIES(unit, port) & ((BMD_PORT_HG) | (BMD_PORT_XE))) {
            bmd_port_mode_t cur_mode;
            uint32_t cur_flags;
            int blkidx, phy_conn_mode;

            /*
             * If HiGig/Ethernet encapsulation changes, we need 
             * to reinitialize the serdes.
             */
            rv = bcm56450_a0_bmd_port_mode_get(unit, port, 
                                               &cur_mode, &cur_flags);
            if (CDK_SUCCESS(rv) && ((flags ^ cur_flags) & HG_FLAGS)) {
                if (flags & HG_FLAGS) {
                    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_HG;
                } else {
                    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
                }

                blkidx = MXQPORT_BLKIDX(unit, port);
                phy_conn_mode = bcm56450_a0_phy_connection_mode(unit, blkidx);
                if (phy_conn_mode == PHY_CONN_WARPCORE) {
                    bcm56450_a0_warpcore_phy_init(unit, port);
                } else {
                    bcm56450_a0_unicore_phy_init(unit, port);
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

    ioerr += READ_XMAC_CTRLr(unit, port, &xmac_ctrl);
    XMAC_CTRLr_RX_ENf_SET(xmac_ctrl, 0);
    XMAC_CTRLr_SOFT_RESETf_SET(xmac_ctrl, 1);
    ioerr += WRITE_XMAC_CTRLr(unit, port, xmac_ctrl);

    if (mode == bmdPortModeDisabled) {
        BMD_PORT_STATUS_SET(unit, port, BMD_PST_FORCE_LINK);
    } else {
        int hg, hg2;
        int phy_mode, wc_sel;
        XMAC_MODEr_t xmac_mode;
        XPORT_MODE_REGr_t xport_mode;
        EGR_PHYSICAL_PORTm_t egr_port;
        ING_PHYSICAL_PORT_TABLEm_t ing_port;
        XPORT_CONFIGr_t xlport_cfg;
        ICONTROL_OPCODE_BITMAPm_t opcode_bmap;
        EGR_VLAN_CONTROL_3r_t vctrl3;
        XMAC_EEE_CTRLr_t xmac_eee_ctrl;
        XMAC_RX_CTRLr_t xmac_rx_ctrl;
        XMAC_TX_CTRLr_t xmac_tx_ctrl;

        /* Get the phy_mode and wc_sel flag */
        if (CDK_SUCCESS(rv)) {
            rv = bcm56450_a0_phy_mode_get(unit, port, speed, &phy_mode, &wc_sel);
        }

        ioerr += READ_XPORT_MODE_REGr(unit, &xport_mode, port);
        XPORT_MODE_REGr_WC_10G_21G_SELf_SET(xport_mode, wc_sel);
        XPORT_MODE_REGr_PHY_PORT_MODEf_SET(xport_mode, phy_mode);
        XPORT_MODE_REGr_PORT_GMII_MII_ENABLEf_SET(xport_mode, ((speed >= 10000) ? 0 : 1));
        ioerr += WRITE_XPORT_MODE_REGr(unit, xport_mode, port);

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
    
        ioerr += READ_XMAC_MODEr(unit, port, &xmac_mode);
        XMAC_MODEr_SPEED_MODEf_SET(xmac_mode, sp_sel);
        if (speed <= 10000) {
            XMAC_MODEr_HDR_MODEf_SET(xmac_mode, 0);
        } else {
            XMAC_MODEr_HDR_MODEf_SET(xmac_mode, hg2 ? 2 : hg);
        }
        ioerr += WRITE_XMAC_MODEr(unit, port, xmac_mode);
        
        ioerr += READ_EGR_PHYSICAL_PORTm(unit, lport, &egr_port);
        EGR_PHYSICAL_PORTm_PORT_TYPEf_SET(egr_port, hg);
        EGR_PHYSICAL_PORTm_HIGIG2f_SET(egr_port, hg2);
        ioerr += WRITE_EGR_PHYSICAL_PORTm(unit, lport, egr_port);
        
        ioerr += READ_ING_PHYSICAL_PORT_TABLEm(unit, lport, &ing_port);
        ING_PHYSICAL_PORT_TABLEm_PORT_TYPEf_SET(ing_port, hg);
        ING_PHYSICAL_PORT_TABLEm_HIGIG2f_SET(ing_port, hg2);
        ioerr += WRITE_ING_PHYSICAL_PORT_TABLEm(unit, lport, ing_port);
        
        ioerr += READ_XPORT_CONFIGr(unit, port, &xlport_cfg);
        XPORT_CONFIGr_HIGIG_MODEf_SET(xlport_cfg, hg);
        XPORT_CONFIGr_HIGIG2_MODEf_SET(xlport_cfg, hg2);
        ioerr += WRITE_XPORT_CONFIGr(unit, port, xlport_cfg);
        
        ICONTROL_OPCODE_BITMAPm_SET(opcode_bmap, 0, hg ? 0x1 : 0x0);
        ICONTROL_OPCODE_BITMAPm_SET(opcode_bmap, 1, 0x0);
        ioerr += WRITE_ICONTROL_OPCODE_BITMAPm(unit, ppport, opcode_bmap);
        
        /* HiGig ports require special egress tag action */
        ioerr += READ_EGR_VLAN_CONTROL_3r(unit, lport, &vctrl3);
        EGR_VLAN_CONTROL_3r_TAG_ACTION_PROFILE_PTRf_SET(vctrl3, hg ? 1 : 0);
        ioerr += WRITE_EGR_VLAN_CONTROL_3r(unit, lport, vctrl3);
        
        /* Disable Strip CRC */
        ioerr += READ_XMAC_RX_CTRLr(unit, port, &xmac_rx_ctrl);
        XMAC_RX_CTRLr_RUNT_THRESHOLDf_SET(xmac_rx_ctrl, ((speed <= 10000) ? 64 : 76));
        XMAC_RX_CTRLr_STRIP_CRCf_SET(xmac_rx_ctrl, 0);
        ioerr += WRITE_XMAC_RX_CTRLr(unit, port, xmac_rx_ctrl);
        
        ioerr += READ_XMAC_TX_CTRLr(unit, port, &xmac_tx_ctrl);
        XMAC_TX_CTRLr_TX_64BYTE_BUFFER_ENf_SET(xmac_tx_ctrl, ((speed <= 2500) ? 1 : 0));
        XMAC_TX_CTRLr_CRC_MODEf_SET(xmac_tx_ctrl, 3);
        ioerr += WRITE_XMAC_TX_CTRLr(unit, port, xmac_tx_ctrl);
        
        /* Configure 10G MAC */
        ioerr += READ_XMAC_CTRLr(unit, port, &xmac_ctrl);
        XMAC_CTRLr_LINE_LOCAL_LPBKf_SET(xmac_ctrl, mac_lb);
        XMAC_CTRLr_SOFT_RESETf_SET(xmac_ctrl, 0);
        XMAC_CTRLr_RX_ENf_SET(xmac_ctrl, 1);
        XMAC_CTRLr_TX_ENf_SET(xmac_ctrl, 1);
        ioerr += WRITE_XMAC_CTRLr(unit, port, xmac_ctrl);
        
        /* Configure EEE */
        ioerr += READ_XMAC_EEE_CTRLr(unit, port, &xmac_eee_ctrl);            
        XMAC_EEE_CTRLr_EEE_ENf_SET(xmac_eee_ctrl, 0);
        ioerr += WRITE_XMAC_EEE_CTRLr(unit, port, xmac_eee_ctrl);                            
        if (flags & BMD_PORT_MODE_F_EEE) {
            /* Enable IEEE 802.3az EEE */
            XMAC_EEE_CTRLr_EEE_ENf_SET(xmac_eee_ctrl, 1);
            ioerr += WRITE_XMAC_EEE_CTRLr(unit, port, xmac_eee_ctrl);                            
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
#endif /* CDK_CONFIG_INCLUDE_BCM56450_A0 */
