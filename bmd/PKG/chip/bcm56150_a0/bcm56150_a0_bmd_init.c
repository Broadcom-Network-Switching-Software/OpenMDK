/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56150_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <bmdi/arch/xgsm_dma.h>

#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm56150_a0_defs.h>

#include <bmd/bmd_phy_ctrl.h>

#include "bcm56150_a0_bmd.h"
#include "bcm56150_a0_internal.h"

#define PIPE_RESET_TIMEOUT_MSEC         5

#define JUMBO_MAXSZ                     0x3fe8
#define MMU_NUM_COS                     8

#define CMIC_NUM_PKT_DMA_CHAN           4

/* Embedded Mode TDM */
static uint8_t tdm_embedded[] = {
    26, 27, 28, 29, 30, 31,
    26, 27, 28, 29, 63, 63,
    26, 27, 28, 29,  2, 63,
    26, 27, 28, 29, 10, 63,
    26, 27, 28, 29, 18,  0,
    26, 27, 28, 29, 32, 33,
    26, 27, 28, 29, 63, 63,
    26, 27, 28, 29,  6, 63,
    26, 27, 28, 29, 14, 63,
    26, 27, 28, 29, 22, 63
};

/* Embedded Plus Mode TDM */
static uint8_t tdm_embedded_plus[] = {
    26, 30, 31, 32, 63, 63,
    26, 30, 31, 32, 63, 63,
    26, 30, 31, 32,  2, 63,
    26, 30, 31, 32, 10, 63,
    26, 30, 31, 32, 18,  0,
    26, 30, 31, 32, 63, 63,
    26, 30, 31, 32, 63, 63,
    26, 30, 31, 32,  6, 63,
    26, 30, 31, 32, 14, 63,
    26, 30, 31, 32, 22, 63
};

/* Powersave Mode TDM */
static uint8_t tdm_powersave[] = {
     2, 10, 18, 63, 63,  0,
     3, 11, 19, 63, 63, 63,
     4, 12, 20, 63, 63, 63,
     5, 13, 21, 63, 63, 63,
     6, 14, 22, 63, 63, 63,
     7, 15, 23, 63, 63, 63,
     8, 16, 24, 63, 63, 63,
     9, 17, 25, 63, 63, 63
};

/* Powersave Plus Mode TDM */
static uint8_t tdm_powersave_plus[] = {
     2, 10, 18, 26, 63,  0,
     3, 11, 19, 63, 63, 63,
     4, 12, 20, 27, 63, 63,
     5, 13, 21, 63, 63, 63,
     6, 14, 22, 28, 63, 63,
     7, 15, 23, 63, 63, 63,
     8, 16, 24, 29, 63, 63,
     9, 17, 25, 63, 63, 63
};

/* Powersave Plus Mode TDM (option 1b) */
static uint8_t tdm_powersave_plus_tsc_1[] = {
     2, 10, 18, 30, 63,  0,
     3, 11, 19, 63, 63, 63,
     4, 12, 20, 31, 63, 63,
     5, 13, 21, 63, 63, 63,
     6, 14, 22, 32, 63, 63,
     7, 15, 23, 63, 63, 63,
     8, 16, 24, 33, 63, 63,
     9, 17, 25, 63, 63, 63
};

/* Cascade Mode TDM */
static uint8_t tdm_cascade[] = {
     2, 14, 26, 28, 30, 32,
     3, 15, 26, 28, 30, 32,
     4, 16, 26, 28, 30, 32,
     5, 17, 26, 28, 30, 32,
     0,  
     6, 18, 26, 28, 30, 32,
     7, 19, 26, 28, 30, 32,
     8, 20, 26, 28, 30, 32,
     9, 21, 26, 28, 30, 32,
    63,
    10, 22, 26, 28, 30, 32,
    11, 23, 26, 28, 30, 32,
    12, 24, 26, 28, 30, 32,
    13, 25, 26, 28, 30, 32,
    63
};

/* XAUI Mode TDM */
static uint8_t tdm_xaui[] = {
     2, 14, 26, 27, 28, 30,
     3, 15, 26, 27, 28, 30,
     4, 16, 26, 27, 28, 30,
     5, 17, 26, 27, 28, 30,
     0,  
     6, 18, 26, 27, 28, 30,
     7, 19, 26, 27, 28, 30,
     8, 20, 26, 27, 28, 30,
     9, 21, 26, 27, 28, 30,
    63,
    10, 22, 26, 27, 28, 30,
    11, 23, 26, 27, 28, 30,
    12, 24, 26, 27, 28, 30,
    13, 25, 26, 27, 28, 30,
    63
};

/* Non-Cascade Mode TDM */
static uint8_t tdm_non_cascade[] = {
     2, 14, 26, 27, 28, 29,
     3, 15, 26, 27, 28, 29,
     4, 16, 26, 27, 28, 29,
     5, 17, 26, 27, 28, 29,
     0,
     6, 18, 26, 27, 28, 29,
     7, 19, 26, 27, 28, 29,
     8, 20, 26, 27, 28, 29,
     9, 21, 26, 27, 28, 29,
    63,
    10, 22, 26, 27, 28, 29,
    11, 23, 26, 27, 28, 29,
    12, 24, 26, 27, 28, 29,
    13, 25, 26, 27, 28, 29,
    63
};


/* Non-Cascade Mode TDM on tsc 1 */
static uint8_t tdm_non_cascade_tsc_1[] = {
     2, 14, 30, 31, 32, 33, 
     3, 15, 30, 31, 32, 33, 
     4, 16, 30, 31, 32, 33, 
     5, 17, 30, 31, 32, 33, 
     0, 
     6, 18, 30, 31, 32, 33, 
     7, 19, 30, 31, 32, 33, 
     8, 20, 30, 31, 32, 33, 
     9, 21, 30, 31, 32, 33, 
    63, 
    10, 22, 30, 31, 32, 33, 
    11, 23, 30, 31, 32, 33, 
    12, 24, 30, 31, 32, 33, 
    13, 25, 30, 31, 32, 33, 
    63
};
/* XAUI Mode TDM on tsc 0 */
static uint8_t tdm_xaui_0[] = {
     2, 14, 26, 30, 31, 32, 
     3, 15, 26, 30, 31, 32, 
     4, 16, 26, 30, 31, 32, 
     5, 17, 26, 30, 31, 32, 
     0, 
     6, 18, 26, 30, 31, 32, 
     7, 19, 26, 30, 31, 32, 
     8, 20, 26, 30, 31, 32,  
     9, 21, 26, 30, 31, 32, 
    63, 
    10, 22, 26, 30, 31, 32,  
    11, 23, 26, 30, 31, 32, 
    12, 24, 26, 30, 31, 32,  
    13, 25, 26, 30, 31, 32, 
    63
};

static int
_mmu_init(int unit)
{
    int ioerr = 0;
    CFAPCONFIGr_t cfapconfig;
    HOLCOSPKTSETLIMITr_t hol_pkt_lim;
    PKTAGINGTIMERr_t pkt_ag_tim;
    PKTAGINGLIMITr_t pkt_ag_lim;
    MMUPORTENABLEr_t mmu_port_en;
    IP_TO_CMICM_CREDIT_TRANSFERr_t cmic_cred_xfer;
    cdk_pbmp_t pbmp, mmu_pbmp;
    uint32_t pbm = 0;
    int port, mport, idx;

    /* Ports to configure */
    CDK_PBMP_CLEAR(mmu_pbmp);
    CDK_PBMP_ADD(mmu_pbmp, CMIC_PORT);
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_OR(mmu_pbmp, pbmp);
    bcm56150_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_OR(mmu_pbmp, pbmp);

    ioerr += READ_CFAPCONFIGr(unit, &cfapconfig);
    CFAPCONFIGr_CFAPPOOLSIZEf_SET(cfapconfig, MMU_CFAPm_MAX);
    ioerr += WRITE_CFAPCONFIGr(unit, cfapconfig);

    /* Enable IP to CMICM credit transfer */
    IP_TO_CMICM_CREDIT_TRANSFERr_CLR(cmic_cred_xfer);
    IP_TO_CMICM_CREDIT_TRANSFERr_TRANSFER_ENABLEf_SET(cmic_cred_xfer, 1);
    IP_TO_CMICM_CREDIT_TRANSFERr_NUM_OF_CREDITSf_SET(cmic_cred_xfer, 32);
    ioerr += WRITE_IP_TO_CMICM_CREDIT_TRANSFERr(unit, cmic_cred_xfer);
    HOLCOSPKTSETLIMITr_SET(hol_pkt_lim, (MMU_XQ0m_MAX+1) / MMU_NUM_COS);
    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        if (mport < 0) {
            continue;
        }
        for (idx = 0; idx < MMU_NUM_COS; idx++) {
            ioerr += WRITE_HOLCOSPKTSETLIMITr(unit, mport, idx, hol_pkt_lim);
        }
        pbm |= 1 << mport;
    }

    /* Disable packet aging on all COSQs */
    PKTAGINGTIMERr_CLR(pkt_ag_tim);
    ioerr += WRITE_PKTAGINGTIMERr(unit, pkt_ag_tim);
    PKTAGINGLIMITr_CLR(pkt_ag_lim);
    ioerr += WRITE_PKTAGINGLIMITr(unit, pkt_ag_lim);

    MMUPORTENABLEr_CLR(mmu_port_en);
    MMUPORTENABLEr_MMUPORTENABLEf_SET(mmu_port_en, pbm);
    ioerr += WRITE_MMUPORTENABLEr(unit, mmu_port_en);

    return ioerr;
}

static int
_port_init(int unit, int port)
{
    int ioerr = 0;
    EGR_ENABLEm_t egr_enable;
    EGR_PORTr_t egr_port;
    EGR_VLAN_CONTROL_1r_t egr_vlan_ctrl1;
    PORT_TABm_t port_tab;
    int lport;

    lport = P2L(unit, port);
    if (lport < 0) {
        return 0;
    }


    /* Default port VLAN and tag action, enable L2 HW learning */
    ioerr += READ_PORT_TABm(unit,lport,&port_tab);
    PORT_TABm_PORT_VIDf_SET(port_tab, 1);
    PORT_TABm_FILTER_ENABLEf_SET(port_tab, 1);
    PORT_TABm_OUTER_TPID_ENABLEf_SET(port_tab, 1);
    PORT_TABm_CML_FLAGS_NEWf_SET(port_tab, 8);
    PORT_TABm_CML_FLAGS_MOVEf_SET(port_tab, 8);
    ioerr += WRITE_PORT_TABm(unit, lport, port_tab);


    /* Filter VLAN on egress */
    ioerr += READ_EGR_PORTr(unit, lport, &egr_port);
    EGR_PORTr_EN_EFILTERf_SET(egr_port, 1);
    ioerr += WRITE_EGR_PORTr(unit, lport, egr_port);

    /* Configure egress VLAN for backward compatibility */
    ioerr += READ_EGR_VLAN_CONTROL_1r(unit, lport, &egr_vlan_ctrl1);
    EGR_VLAN_CONTROL_1r_VT_MISS_UNTAGf_SET(egr_vlan_ctrl1, 0);
    EGR_VLAN_CONTROL_1r_REMARK_OUTER_DOT1Pf_SET(egr_vlan_ctrl1, 1);
    ioerr += WRITE_EGR_VLAN_CONTROL_1r(unit, lport, egr_vlan_ctrl1);

    /* Egress enable */
    EGR_ENABLEm_CLR(egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    ioerr += WRITE_EGR_ENABLEm(unit, port, egr_enable); 

    return ioerr;
}

static int
_gport_init(int unit, int port)
{
    int ioerr = 0;
    COMMAND_CONFIGr_t command_cfg;
    TX_IPG_LENGTHr_t tx_ipg;
    cdk_pbmp_t pbmp;
    ioerr += _port_init(unit, port);  
   
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    if (CDK_PBMP_MEMBER(pbmp, port)) {
       ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
       COMMAND_CONFIGr_SW_RESETf_SET(command_cfg, 1);
       ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

       /* Ensure that MAC (Rx) and loopback mode is disabled */
       ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
       COMMAND_CONFIGr_LOOP_ENAf_SET(command_cfg, 0);
       COMMAND_CONFIGr_RX_ENAf_SET(command_cfg, 0);
       COMMAND_CONFIGr_TX_ENAf_SET(command_cfg, 0);
       ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

       ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
       COMMAND_CONFIGr_SW_RESETf_SET(command_cfg, 0);
       ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

       TX_IPG_LENGTHr_SET(tx_ipg, 12);
       WRITE_TX_IPG_LENGTHr(unit, port, tx_ipg);
    }
    return ioerr;
}

int
bcm56150_a0_xlport_init(int unit, int port)
{
    int ioerr = 0;
    XLPORT_MODE_REGr_t xlport_mode;
    XLPORT_SOFT_RESETr_t xlport_sreset;
    XLPORT_ENABLE_REGr_t xlport_enable;
    XLPORT_CNTMAXSIZEr_t xlport_cntmaxsize;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;
    XLPORT_TXFIFO_CTRLr_t xlport_txfifo_ctrl;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    XLMAC_PAUSE_CTRLr_t xlmac_pause_ctrl;
    XLMAC_RX_MAX_SIZEr_t xlmac_rx_max_size;
    XLMAC_MODEr_t xlmac_mode;
    XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
    XLMAC_PFC_CTRLr_t xlmac_pfc_ctrl;
    int mode, sub_port;
    int lport, lanes;

    ioerr += _port_init(unit, port);


    /* We only need to configure XLPORT registers once per block */
    if (XLPORT_SUBPORT(port) == 0) {
        XLPORT_SOFT_RESETr_CLR(xlport_sreset);
        for (sub_port = 0; sub_port <= 3; sub_port++) {
            lport = P2L(unit, port + sub_port);
            if (lport < 0) {
                continue;
            }
            if (sub_port == 0) {
                XLPORT_SOFT_RESETr_PORT0f_SET(xlport_sreset, 1);
            } else if (sub_port == 1) {
                XLPORT_SOFT_RESETr_PORT1f_SET(xlport_sreset, 1);
            } else if (sub_port == 2) {
                XLPORT_SOFT_RESETr_PORT2f_SET(xlport_sreset, 1);
            } else if (sub_port == 3) {
                XLPORT_SOFT_RESETr_PORT3f_SET(xlport_sreset, 1);
            }
        }
        ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlport_sreset, port);

        /* Set XPORT mode to 10G by default */
        lanes = bcm56150_a0_port_num_lanes(unit, port);
        mode = XPORT_MODE_QUAD;
        if (lanes == 4) {
            mode = XPORT_MODE_SINGLE;
        } else if (lanes == 2) {
            mode = XPORT_MODE_DUAL;
        } 
        
        XLPORT_MODE_REGr_CLR(xlport_mode);
        XLPORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(xlport_mode, mode);
        XLPORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(xlport_mode, mode);
        ioerr += WRITE_XLPORT_MODE_REGr(unit, xlport_mode, port);

        for (sub_port = 0; sub_port <= 3; sub_port++) {
            lport = P2L(unit, port + sub_port);
            if (lport < 0) {
                continue;
            }
            ioerr += READ_XLPORT_CNTMAXSIZEr(unit, port+sub_port, &xlport_cntmaxsize);
            XLPORT_CNTMAXSIZEr_CNTMAXSIZEf_SET(xlport_cntmaxsize, 0xde8);
            ioerr += WRITE_XLPORT_CNTMAXSIZEr(unit, port+sub_port, xlport_cntmaxsize);
        }
        
        /* De-assert XLPORT soft reset */
        XLPORT_SOFT_RESETr_CLR(xlport_sreset);
        ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlport_sreset, port);

        /* Disable XLPORT */
        XLPORT_ENABLE_REGr_CLR(xlport_enable);
        ioerr += WRITE_XLPORT_ENABLE_REGr(unit, xlport_enable, port);

        for (sub_port = 0; sub_port <= 3; sub_port++) {
            lport = P2L(unit, port + sub_port);
            if (lport < 0) {
                continue;
            }
            /* CREDIT reset and MAC count clear */
            ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, port+sub_port, &egr_port_credit_reset);
            EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
            ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port+sub_port, egr_port_credit_reset);
            
            ioerr += READ_XLPORT_TXFIFO_CTRLr(unit, port+sub_port, &xlport_txfifo_ctrl);
            XLPORT_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(xlport_txfifo_ctrl, 1);
            XLPORT_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(xlport_txfifo_ctrl, 1);
            ioerr += WRITE_XLPORT_TXFIFO_CTRLr(unit, port+sub_port, xlport_txfifo_ctrl);
            
            BMD_SYS_USLEEP(1000);
            
            ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, port+sub_port, &egr_port_credit_reset);
            EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
            ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, port+sub_port, egr_port_credit_reset);
            
            ioerr += READ_XLPORT_TXFIFO_CTRLr(unit, port+sub_port, &xlport_txfifo_ctrl);
            XLPORT_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(xlport_txfifo_ctrl, 0);
            XLPORT_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(xlport_txfifo_ctrl, 0);
            ioerr += WRITE_XLPORT_TXFIFO_CTRLr(unit, port+sub_port, xlport_txfifo_ctrl);
        }
        
        /* Enable XLPORT */
        for (sub_port = 0; sub_port <= 3; sub_port++) {
            lport = P2L(unit, port + sub_port);
            if (lport < 0) {
                continue;
            }
            if (sub_port == 0) {
                XLPORT_ENABLE_REGr_PORT0f_SET(xlport_enable, 1);
            } else if (sub_port == 1) {
                XLPORT_ENABLE_REGr_PORT1f_SET(xlport_enable, 1);
            } else if (sub_port == 2) {
                XLPORT_ENABLE_REGr_PORT2f_SET(xlport_enable, 1);
            } else if (sub_port == 3) {
                XLPORT_ENABLE_REGr_PORT3f_SET(xlport_enable, 1);
            }
        }
        ioerr += WRITE_XLPORT_ENABLE_REGr(unit, xlport_enable, port);
    }

    /* XLMAC init */
    XLMAC_CTRLr_CLR(xlmac_ctrl);
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 1);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

    ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_SW_LINK_STATUSf_SET(xlmac_ctrl, 1);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

    ioerr += READ_XLMAC_RX_CTRLr(unit, port, &xlmac_rx_ctrl);
    XLMAC_RX_CTRLr_STRIP_CRCf_SET(xlmac_rx_ctrl, 0);
    XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 1);
    ioerr += WRITE_XLMAC_RX_CTRLr(unit, port, xlmac_rx_ctrl);

    ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
    XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, 0xc);
    XLMAC_TX_CTRLr_CRC_MODEf_SET(xlmac_tx_ctrl, 2);
    ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);

    ioerr += READ_XLMAC_PAUSE_CTRLr(unit, port, &xlmac_pause_ctrl);
    XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(xlmac_pause_ctrl, 1);
    XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(xlmac_pause_ctrl, 1);
    ioerr += WRITE_XLMAC_PAUSE_CTRLr(unit, port, xlmac_pause_ctrl);

    ioerr += READ_XLMAC_PFC_CTRLr(unit, port, &xlmac_pfc_ctrl);
    XLMAC_PFC_CTRLr_PFC_REFRESH_ENf_SET(xlmac_pfc_ctrl, 1);
    ioerr += WRITE_XLMAC_PFC_CTRLr(unit, port, xlmac_pfc_ctrl);

    ioerr += READ_XLMAC_TX_CTRLr(unit, port, &xlmac_tx_ctrl);
    XLMAC_TX_CTRLr_THROT_NUMf_SET(xlmac_tx_ctrl, 0);
    ioerr += WRITE_XLMAC_TX_CTRLr(unit, port, xlmac_tx_ctrl);

    ioerr += READ_XLMAC_RX_MAX_SIZEr(unit, port, &xlmac_rx_max_size);
    XLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(xlmac_rx_max_size, JUMBO_MAXSZ);
    ioerr += WRITE_XLMAC_RX_MAX_SIZEr(unit, port, xlmac_rx_max_size);

    ioerr += READ_XLMAC_MODEr(unit, port, &xlmac_mode);
    XLMAC_MODEr_HDR_MODEf_SET(xlmac_mode, 0);
    XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, 4);
    ioerr += WRITE_XLMAC_MODEr(unit, port, xlmac_mode);

    ioerr += READ_XLMAC_RX_LSS_CTRLr(unit, port, &xlmac_rx_lss_ctrl);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
    ioerr += WRITE_XLMAC_RX_LSS_CTRLr(unit, port, xlmac_rx_lss_ctrl);

    ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 1);
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
    ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

int
bcm56150_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int tdm_size;
    IARB_TDM_TABLEm_t iarb_tdm;
    MMU_ARB_TDM_TABLEm_t mmu_arb_tdm;
    IARB_TDM_CONTROLr_t iarb_tdm_ctrl;
    ING_HW_RESET_CONTROL_1r_t ing_rst_ctl_1;
    ING_HW_RESET_CONTROL_2r_t ing_rst_ctl_2;
    EGR_HW_RESET_CONTROL_0r_t egr_rst_ctl_0;
    EGR_HW_RESET_CONTROL_1r_t egr_rst_ctl_1;
    MISCCONFIGr_t misc_cfg;
    CMIC_RATE_ADJUSTr_t rate_adjust;
    CMIC_RATE_ADJUST_INT_MDIOr_t rate_adjust_int_mdio;
    RDBGC0_SELECTr_t rdbgc0_select;
    VLAN_PROFILE_TABm_t vlan_profile;
    ING_VLAN_TAG_ACTION_PROFILEm_t vlan_action;
    EGR_VLAN_TAG_ACTION_PROFILEm_t egr_action;
    GPORT_RSV_MASKr_t gport_rsv_mask;
    GPORT_CONFIGr_t gport_cfg;
    XLPORT_MIB_RESETr_t mib_rst;
    ING_PHYS_TO_LOGIC_MAPm_t ing_p2l;
    EGR_LOGIC_TO_PHYS_MAPr_t  egr_l2p;
    MMU_LOGIC_TO_PHYS_MAPr_t mmu_l2p;
    TOP_MISC_CONTROL_1r_t top_misc_ctrl_1;
    TOP_CORE_PLL_CTRL4r_t top_core_pll_ctrl4;
    TOP_CORE_PLL_CTRL2r_t top_core_pll_ctrl2;
    uint32_t dcfg;
    cdk_pbmp_t pbmp;
    int lport, port, lanes;
    int idx;
    int num_lport, num_port;
    uint8_t *tdm_array;

    BMD_CHECK_UNIT(unit);

    /* Reset the IPIPE block */
    ING_HW_RESET_CONTROL_1r_CLR(ing_rst_ctl_1);
    ioerr += WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_rst_ctl_1);
    ING_HW_RESET_CONTROL_2r_CLR(ing_rst_ctl_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_rst_ctl_2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_rst_ctl_2, 1);
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_rst_ctl_2, 0x8000);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);

    /* Reset the EPIPE block */
    EGR_HW_RESET_CONTROL_0r_CLR(egr_rst_ctl_0);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_rst_ctl_0);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_rst_ctl_1, 0x4000);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);

    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_ING_HW_RESET_CONTROL_2r(unit, &ing_rst_ctl_2);
        if (ING_HW_RESET_CONTROL_2r_DONEf_GET(ing_rst_ctl_2)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56150_a0_bmd_init[%d]: IPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }
        
    for (; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_EGR_HW_RESET_CONTROL_1r(unit, &egr_rst_ctl_1);
        if (EGR_HW_RESET_CONTROL_1r_DONEf_GET(egr_rst_ctl_1)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56150_a0_bmd_init[%d]: EPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    /* Clear pipe reset registers */
    ING_HW_RESET_CONTROL_2r_CLR(ing_rst_ctl_2);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);
    
    /* Ingress physical to logical port mapping */
    num_port = ING_PHYS_TO_LOGIC_MAPm_MAX;
    for (port = 0; port <= num_port; port++) {
        lport = P2L(unit, port);
        if (lport == -1) {
            lport = 0x1f;
        }
        ING_PHYS_TO_LOGIC_MAPm_CLR(ing_p2l); 
        ING_PHYS_TO_LOGIC_MAPm_LOGICAL_PORT_NUMBERf_SET(ing_p2l, lport);
        ioerr += WRITE_ING_PHYS_TO_LOGIC_MAPm(unit, port, ing_p2l);
    }

    /* Egress logical to physical port mapping */
    num_lport = PORT_TABm_MAX;
    for (lport = 0; lport <= num_lport; lport++) {
        port = L2P(unit, lport);
        if (port == -1) {
            port = 0x3f;
        }
        EGR_LOGIC_TO_PHYS_MAPr_CLR(egr_l2p);
        EGR_LOGIC_TO_PHYS_MAPr_PHYSICAL_PORT_NUMBERf_SET(egr_l2p, port);
        ioerr += WRITE_EGR_LOGIC_TO_PHYS_MAPr(unit, lport, egr_l2p);
    }

    /* MMU logical to physical port mapping */
    for (lport = 0; lport <= num_lport; lport++) {
        port = M2P(unit, lport);
        if (port == -1) {
            port = 0x3f;
        }
        MMU_LOGIC_TO_PHYS_MAPr_CLR(mmu_l2p);
        MMU_LOGIC_TO_PHYS_MAPr_PHYS_PORTf_SET(mmu_l2p, port);
        ioerr += WRITE_MMU_LOGIC_TO_PHYS_MAPr(unit, lport, mmu_l2p);
    }

    /* Bump up core clock to 135 MHz if any TSC port is > 1G */
    ioerr += READ_TOP_MISC_CONTROL_1r(unit, &top_misc_ctrl_1);
    TOP_MISC_CONTROL_1r_CMIC_TO_CORE_PLL_LOADf_SET(top_misc_ctrl_1, 1);
    ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_ctrl_1);

    ioerr += READ_TOP_CORE_PLL_CTRL4r(unit, &top_core_pll_ctrl4);
    TOP_CORE_PLL_CTRL4r_MSTR_CH0_MDIVf_SET(top_core_pll_ctrl4, 0x19);
    ioerr += WRITE_TOP_CORE_PLL_CTRL4r(unit, top_core_pll_ctrl4);

    ioerr += READ_TOP_CORE_PLL_CTRL2r(unit, &top_core_pll_ctrl2);
    TOP_CORE_PLL_CTRL2r_LOAD_EN_CH0f_SET(top_core_pll_ctrl2, 1);
    TOP_CORE_PLL_CTRL2r_LOAD_EN_CH1f_SET(top_core_pll_ctrl2, 1);
    ioerr += WRITE_TOP_CORE_PLL_CTRL2r(unit, top_core_pll_ctrl2);

    /* XQLPORT and GPORT configuration determines which TDM table to use */
    ioerr += READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl); 
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 1);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, 83);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);  

    /* Get the TDM table from chip configuration and dynamic configuration flags */
    dcfg = CDK_CHIP_CONFIG(unit);
    if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_TSC_10G) {
        if (dcfg & (DCFG_TSC_0_1 | DCFG_HGD_0 | DCFG_HGD_1)) {
            tdm_array = tdm_cascade;
            tdm_size  = COUNTOF(tdm_cascade);
        } else if (dcfg & (DCFG_XAUI_1 | DCFG_XAUI_0_1)) {
            tdm_array = tdm_xaui;
            tdm_size  = COUNTOF(tdm_xaui);
        } else if (dcfg & DCFG_TSC_1) {
            tdm_array = tdm_non_cascade_tsc_1;
            tdm_size  = COUNTOF(tdm_non_cascade_tsc_1);
        } else if (dcfg & DCFG_XAUI_0) {
            tdm_array = tdm_xaui_0;
            tdm_size  = COUNTOF(tdm_xaui_0);
        } else {
            tdm_array = tdm_non_cascade;
            tdm_size  = COUNTOF(tdm_non_cascade);
        }   
    } else if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_TSC_1G) {
        if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_NO_PHY) {
            tdm_array = tdm_embedded;
            tdm_size  = COUNTOF(tdm_embedded);
        } else {   
            if (dcfg & (DCFG_HGD_0 | DCFG_HGD_1)) {
                tdm_array = tdm_cascade;
                tdm_size  = COUNTOF(tdm_cascade);
            } else if (dcfg & DCFG_TSC_1) {
                tdm_array = tdm_powersave_plus_tsc_1;
                tdm_size  = COUNTOF(tdm_powersave_plus_tsc_1);
            } else {
                tdm_array = tdm_powersave_plus;
                tdm_size  = COUNTOF(tdm_powersave_plus);
            }
        }
    } else if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_NO_TSC) {
        tdm_array = tdm_powersave;
        tdm_size  = COUNTOF(tdm_powersave);
    } else { 
        if (dcfg & DCFG_XAUI_0) {
            tdm_array = tdm_embedded_plus;
            tdm_size  = COUNTOF(tdm_embedded_plus);
        } else {
            tdm_array = tdm_embedded;
            tdm_size  = COUNTOF(tdm_embedded);
        }
    }

    for (idx = 0; idx < tdm_size; idx++) {
        port = tdm_array[idx];
        lport = port;
        if (lport != 63) {
            lport = P2L(unit, port);
        }
        IARB_TDM_TABLEm_CLR(iarb_tdm);
        IARB_TDM_TABLEm_PORT_NUMf_SET(iarb_tdm, port);
        ioerr += WRITE_IARB_TDM_TABLEm(unit, idx, iarb_tdm);

        MMU_ARB_TDM_TABLEm_CLR(mmu_arb_tdm);
        MMU_ARB_TDM_TABLEm_PORT_NUMf_SET(mmu_arb_tdm, lport);
        if (idx == (tdm_size - 1)) {
            MMU_ARB_TDM_TABLEm_WRAP_ENf_SET(mmu_arb_tdm, 1);
        }
        ioerr += WRITE_MMU_ARB_TDM_TABLEm(unit, idx, mmu_arb_tdm);
    }

    /* Enable arbiter */
    IARB_TDM_CONTROLr_CLR(iarb_tdm_ctrl);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 0);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, tdm_size - 1);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);

    /* Enable Field Processor metering clock */
    ioerr += READ_MISCCONFIGr(unit, &misc_cfg);
    MISCCONFIGr_METERING_CLK_ENf_SET(misc_cfg, 1);
    ioerr += WRITE_MISCCONFIGr(unit, misc_cfg);

    /*
     * Set reference clock (based on 200MHz core clock)
     * to be 200MHz * (1/40) = 5MHz
     */
    CMIC_RATE_ADJUSTr_CLR(rate_adjust);
    CMIC_RATE_ADJUSTr_DIVISORf_SET(rate_adjust, 40);
    CMIC_RATE_ADJUSTr_DIVIDENDf_SET(rate_adjust, 1);
    ioerr += WRITE_CMIC_RATE_ADJUSTr(unit, rate_adjust);

    /* Match the Internal MDC freq with above for External MDC */
    CMIC_RATE_ADJUST_INT_MDIOr_CLR(rate_adjust_int_mdio);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVISORf_SET(rate_adjust_int_mdio, 40);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVIDENDf_SET(rate_adjust_int_mdio, 1);
    ioerr += WRITE_CMIC_RATE_ADJUST_INT_MDIOr(unit, rate_adjust_int_mdio);

    /* Configure discard counter */
    RDBGC0_SELECTr_CLR(rdbgc0_select);
    RDBGC0_SELECTr_BITMAPf_SET(rdbgc0_select, 0x0400ad11);
    ioerr += WRITE_RDBGC0_SELECTr(unit, rdbgc0_select);

    /* Initialize MMU */
    ioerr += _mmu_init(unit);

    /* Default VLAN profile */
    VLAN_PROFILE_TABm_CLR(vlan_profile);
    VLAN_PROFILE_TABm_L2_PFMf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_L3_IPV4_PFMf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_L3_IPV6_PFMf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPMCV6_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPMCV4_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPMCV6_L2_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPMCV4_L2_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPV6L3_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILE_TABm_IPV4L3_ENABLEf_SET(vlan_profile, 1);
    ioerr += WRITE_VLAN_PROFILE_TABm(unit, VLAN_PROFILE_TABm_MAX, vlan_profile);

    /* Ensure that all incoming packets get tagged appropriately */
    ING_VLAN_TAG_ACTION_PROFILEm_CLR(vlan_action);
    ING_VLAN_TAG_ACTION_PROFILEm_UT_OTAG_ACTIONf_SET(vlan_action, 1);
    ING_VLAN_TAG_ACTION_PROFILEm_SIT_PITAG_ACTIONf_SET(vlan_action, 3);
    ING_VLAN_TAG_ACTION_PROFILEm_SIT_OTAG_ACTIONf_SET(vlan_action, 1);
    ING_VLAN_TAG_ACTION_PROFILEm_SOT_POTAG_ACTIONf_SET(vlan_action, 2);
    ING_VLAN_TAG_ACTION_PROFILEm_DT_POTAG_ACTIONf_SET(vlan_action, 2);
    ioerr += WRITE_ING_VLAN_TAG_ACTION_PROFILEm(unit, 0, vlan_action);

    /* Create special egress action profile for HiGig ports */
    EGR_VLAN_TAG_ACTION_PROFILEm_CLR(egr_action);
    EGR_VLAN_TAG_ACTION_PROFILEm_SOT_OTAG_ACTIONf_SET(egr_action, 3);
    EGR_VLAN_TAG_ACTION_PROFILEm_DT_OTAG_ACTIONf_SET(egr_action, 3);
    ioerr += WRITE_EGR_VLAN_TAG_ACTION_PROFILEm(unit, 1, egr_action);

    /* Fix-up packet purge filtering */
    GPORT_RSV_MASKr_SET(gport_rsv_mask, 0x70);
    ioerr += WRITE_GPORT_RSV_MASKr(unit, gport_rsv_mask, -1);

    /* Enable GPORTs and clear counters */
    ioerr += READ_GPORT_CONFIGr(unit, &gport_cfg, -1);
    GPORT_CONFIGr_CLR_CNTf_SET(gport_cfg, 1);
    ioerr += WRITE_GPORT_CONFIGr(unit, gport_cfg, -1);
    GPORT_CONFIGr_GPORT_ENf_SET(gport_cfg, 1);
    ioerr += WRITE_GPORT_CONFIGr(unit, gport_cfg, -1);
    GPORT_CONFIGr_CLR_CNTf_SET(gport_cfg, 0);
    ioerr += WRITE_GPORT_CONFIGr(unit, gport_cfg, -1);

    /* Configure GPORTs */
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        ioerr += _gport_init(unit, port);
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_attach(unit, port);
        }
    }

    /* Clear XLPORT counters */ 
    if (!(CDK_XGSM_FLAGS(unit) & CHIP_FLAG_NO_TSC)) {
        XLPORT_MIB_RESETr_SET(mib_rst, 0xf);
        ioerr += WRITE_XLPORT_MIB_RESETr(unit, mib_rst, -1);
        XLPORT_MIB_RESETr_SET(mib_rst, 0x0);
        ioerr += WRITE_XLPORT_MIB_RESETr(unit, mib_rst, -1);
    }
    
    /* Configure XLPORTs */
    bcm56150_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        ioerr += bcm56150_a0_xlport_init(unit, port);
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }
        
        if (port >= 26 && port < 34) {
            if (CDK_SUCCESS(rv)) {
                lanes = bcm56150_a0_port_num_lanes(unit, port);
                if (lanes == 4) {
                    rv = bmd_phy_mode_set(unit, port, "tsc",
                                            BMD_PHY_MODE_SERDES, 0);
                } else if (lanes == 2) {
                    rv = bmd_phy_mode_set(unit, port, "tsc",
                                            BMD_PHY_MODE_SERDES, 1);
                    rv = bmd_phy_mode_set(unit, port, "tsc", 
                                            BMD_PHY_MODE_2LANE, 1);
                } else { /* lanes = 1 */
                    rv = bmd_phy_mode_set(unit, port, "tsc",
                                            BMD_PHY_MODE_SERDES, 1);
                    rv = bmd_phy_mode_set(unit, port, "tsc", 
                                            BMD_PHY_MODE_2LANE, 0);
                }
            }
            if (CDK_SUCCESS(rv) && (XLPORT_SUBPORT(port) == 0)) {
                rv = bmd_phy_fw_base_set(unit, port, "tsc", 
                                         0x0f00);
            }
        }
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_init(unit, port);
        }
    }

#if BMD_CONFIG_INCLUDE_DMA
    /* Common port initialization for CPU port */
    ioerr += _port_init(unit, CMIC_PORT);
    if (CDK_SUCCESS(rv)) {
        rv = bmd_xgsm_dma_init(unit);
    }

    /* Additional configuration required when on PCI bus */
    if (CDK_DEV_FLAGS(unit) & CDK_DEV_MBUS_PCI) {
        CMIC_CMC_HOSTMEM_ADDR_REMAPr_t hostmem_remap;
        uint32_t remap_val[] = { 0x144D2450, 0x19617595, 0x1E75C6DA, 0x1f };

        /* Send DMA data to external host memory when on PCI bus */
        for (idx = 0; idx < COUNTOF(remap_val); idx++) {
            CMIC_CMC_HOSTMEM_ADDR_REMAPr_SET(hostmem_remap, remap_val[idx]);
            ioerr += WRITE_CMIC_CMC_HOSTMEM_ADDR_REMAPr(unit, idx, hostmem_remap);
        }
    }
#endif


    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56150_A0 */
