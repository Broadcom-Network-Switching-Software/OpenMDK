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
#include <bmdi/arch/xgsd_dma.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_util.h>
#include <cdk/chip/bcm56160_a0_defs.h>

#include "bcm56160_a0_bmd.h"
#include "bcm56160_a0_internal.h"

#define PIPE_RESET_TIMEOUT_MSEC         5

#define JUMBO_MAXSZ                     0x3fe8
#define MMU_NUM_COS                     8

#define FW_ALIGN_BYTES                  16
#define FW_ALIGN_MASK                   (FW_ALIGN_BYTES - 1)

#define CMIC_NUM_PKT_DMA_CHAN           4

static int
_firmware_helper(void *ctx, uint32_t offset, uint32_t size, void *data)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    XLPORT_WC_UCMEM_CTRLr_t xl_ucmem_ctrl;
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;
    GPORT_WC_UCMEM_CTRLr_t g_ucmem_ctrl;
    GPORT_WC_UCMEM_DATAm_t g_ucmem_data;
    int unit, port;
    const char *drv_name;
    uint32_t wbuf[4];
    uint32_t wdata;
    uint32_t *fw_data;
    uint32_t *fw_entry;
    uint32_t fw_size;
    uint32_t idx, wdx;
    uint32_t be_host;

    /* Get unit, port and driver name from context */
    bmd_phy_fw_info_get(ctx, &unit, &port, &drv_name);

    /* Check if PHY driver requests optimized MDC clock */
    if (data == NULL) {
        CMIC_RATE_ADJUSTr_t rate_adjust;
        uint32_t val = 1;

        /* Offset value is MDC clock in kHz (or zero for default) */
        if (offset) {
            val = offset / 1500;
        }
        ioerr += READ_CMIC_RATE_ADJUSTr(unit, &rate_adjust);
        CMIC_RATE_ADJUSTr_DIVIDENDf_SET(rate_adjust, val);
        ioerr += WRITE_CMIC_RATE_ADJUSTr(unit, rate_adjust);

        return ioerr ? CDK_E_IO : CDK_E_NONE;
    }

    if (size == 0) {
        return CDK_E_INTERNAL;
    }
    /* Aligned firmware size */
    fw_size = (size + FW_ALIGN_MASK) & ~FW_ALIGN_MASK;

    /* We need to byte swap on big endian host */
    be_host = 1;
    if (*((uint8_t *)&be_host) == 1) {
        be_host = 0;
    }

    if (CDK_STRSTR(drv_name, "qtce")) {
        int bidx = XPORT_BLKIDX(unit, BLKTYPE_PMQ, port);

        /* Enable parallel bus access */
        ioerr += READ_GPORT_WC_UCMEM_CTRLr(unit, bidx, &g_ucmem_ctrl);
        GPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(g_ucmem_ctrl, 1);
        ioerr += WRITE_GPORT_WC_UCMEM_CTRLr(unit, bidx, g_ucmem_ctrl);

        /* DMA buffer needs 32-bit words in little endian order */
        fw_data = (uint32_t *)data;
        for (idx = 0; idx < fw_size; idx += 16) {
            if (idx + 15 < size) {
                fw_entry = &fw_data[idx >> 2];
            } else {
                /* Use staging buffer for modulo bytes */
                CDK_MEMSET(wbuf, 0, sizeof(wbuf));
                CDK_MEMCPY(wbuf, &fw_data[idx >> 2], 16 - (fw_size - size));
                fw_entry = wbuf;
            }
            for (wdx = 0; wdx < 4; wdx++) {
                wdata = fw_entry[wdx];
                if (be_host) {
                    wdata = cdk_util_swap32(wdata);
                }
                GPORT_WC_UCMEM_DATAm_SET(g_ucmem_data, wdx, wdata);
            }
            WRITE_GPORT_WC_UCMEM_DATAm(unit, bidx, (idx >> 4), g_ucmem_data);
        }

        /* Disable parallel bus access */
        GPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(g_ucmem_ctrl, 0);
        ioerr += WRITE_GPORT_WC_UCMEM_CTRLr(unit, bidx, g_ucmem_ctrl);
    } else if (CDK_STRSTR(drv_name, "tsce")) {
        /* Enable parallel bus access */
        ioerr += READ_XLPORT_WC_UCMEM_CTRLr(unit, &xl_ucmem_ctrl, port);
        XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(xl_ucmem_ctrl, 1);
        ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, xl_ucmem_ctrl, port);

        /* DMA buffer needs 32-bit words in little endian order */
        fw_data = (uint32_t *)data;
        for (idx = 0; idx < fw_size; idx += 16) {
            if (idx + 15 < size) {
                fw_entry = &fw_data[idx >> 2];
            } else {
                /* Use staging buffer for modulo bytes */
                CDK_MEMSET(wbuf, 0, sizeof(wbuf));
                CDK_MEMCPY(wbuf, &fw_data[idx >> 2], 16 - (fw_size - size));
                fw_entry = wbuf;
            }
            for (wdx = 0; wdx < 4; wdx++) {
                wdata = fw_entry[wdx];
                if (be_host) {
                    wdata = cdk_util_swap32(wdata);
                }
                XLPORT_WC_UCMEM_DATAm_SET(xl_ucmem_data, wdx, wdata);
            }
            WRITE_XLPORT_WC_UCMEM_DATAm(unit, (idx >> 4), xl_ucmem_data, port);
        }

        /* Disable parallel bus access */
        XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(xl_ucmem_ctrl, 0);
        ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, xl_ucmem_ctrl, port);
    } else {
        return CDK_E_UNAVAIL;
    }

    return ioerr ? CDK_E_IO : rv;
}

static int
_mmu_tdm_init(int unit)
{
    int ioerr = 0;
    IARB_TDM_CONTROLr_t iarb_tdm_ctrl;
    IARB_TDM_TABLEm_t iarb_tdm;
    MMU_ARB_TDM_TABLEm_t mmu_arb_tdm;
    const int *tdm_seq;
    int tdm_size, tdm_max;
    int idx;
    int port, mport;

    /* Get default TDM sequence for this configuration */
    tdm_size = bcm56160_a0_tdm_default(unit, &tdm_seq);
    if (tdm_size <= 0) {
        return CDK_E_INTERNAL;
    }
    tdm_max = tdm_size - 1;

    ioerr += READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 1);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, 83);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);

    for (idx = 0; idx < tdm_size; idx++) {
        port = tdm_seq[idx];
        mport = port;
        if (mport != 63) {
            mport = P2M(unit, port);
        }
        IARB_TDM_TABLEm_CLR(iarb_tdm);
        IARB_TDM_TABLEm_PORT_NUMf_SET(iarb_tdm, port);
        ioerr += WRITE_IARB_TDM_TABLEm(unit, idx, iarb_tdm);

        MMU_ARB_TDM_TABLEm_CLR(mmu_arb_tdm);
        MMU_ARB_TDM_TABLEm_PORT_NUMf_SET(mmu_arb_tdm, mport);
        if (idx == (tdm_size - 1)) {
            MMU_ARB_TDM_TABLEm_WRAP_ENf_SET(mmu_arb_tdm, 1);
        }
        ioerr += WRITE_MMU_ARB_TDM_TABLEm(unit, idx, mmu_arb_tdm);

        CDK_VERB(("%s", (idx == 0) ? "TDM seq:" : ""));
        CDK_VERB(("%s", (idx & 0xf) == 0 ? "\n" : ""));
        CDK_VERB(("%3d ", tdm_seq[idx]));
    }
    CDK_VERB(("\n"));

    /* Enable arbiter */
    ioerr += READ_IARB_TDM_CONTROLr(unit, &iarb_tdm_ctrl);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_ctrl, 0);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_ctrl, tdm_max);
    ioerr += WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static int
_mmu_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    CFAPCONFIGr_t cfapconfig;
    HOLCOSPKTSETLIMITr_t hol_pkt_lim;
    PKTAGINGTIMERr_t pkt_ag_tim;
    PKTAGINGLIMITr_t pkt_ag_lim;
    MMUPORTENABLEr_t mmu_port_en;
    IP_TO_CMICM_CREDIT_TRANSFERr_t cmic_cred_xfer;
    cdk_pbmp_t pbmp, gpbmp, xlpbmp;
    uint32_t pbm = 0;
    int port, mport, idx;

    /* Setup TDM for MMU */
    rv = _mmu_tdm_init(unit);

    CDK_PBMP_CLEAR(pbmp);
    CDK_PBMP_ADD(pbmp, CMIC_PORT);
    bcm56160_a0_gport_pbmp_get(unit, &gpbmp);
    bcm56160_a0_xlport_pbmp_get(unit, &xlpbmp);
    CDK_PBMP_OR(pbmp, gpbmp);
    CDK_PBMP_OR(pbmp, xlpbmp);

    ioerr += READ_CFAPCONFIGr(unit, &cfapconfig);
    CFAPCONFIGr_CFAPPOOLSIZEf_SET(cfapconfig, MMU_CFAPm_MAX);
    ioerr += WRITE_CFAPCONFIGr(unit, cfapconfig);

    /* Enable IP to CMICM credit transfer */
    IP_TO_CMICM_CREDIT_TRANSFERr_CLR(cmic_cred_xfer);
    IP_TO_CMICM_CREDIT_TRANSFERr_TRANSFER_ENABLEf_SET(cmic_cred_xfer, 1);
    IP_TO_CMICM_CREDIT_TRANSFERr_NUM_OF_CREDITSf_SET(cmic_cred_xfer, 32);
    ioerr += WRITE_IP_TO_CMICM_CREDIT_TRANSFERr(unit, cmic_cred_xfer);
    HOLCOSPKTSETLIMITr_SET(hol_pkt_lim, (MMU_XQ0m_MAX + 1) / MMU_NUM_COS);
    CDK_PBMP_ITER(pbmp, port) {
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

    return ioerr ? CDK_E_IO : rv;
}

static int
_port_init(int unit, int port)
{
    int ioerr = 0;
    EGR_ENABLEm_t egr_enable;
    EGR_PORT_64r_t egr_port;
    IEGR_PORT_64r_t iegr_port;
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
    ioerr += READ_EGR_PORT_64r(unit, lport, &egr_port);
    EGR_PORT_64r_EN_EFILTERf_SET(egr_port, 1);
    ioerr += WRITE_EGR_PORT_64r(unit, lport, egr_port);

    /* Directed Mirroring ON by default */
    ioerr += READ_EGR_PORT_64r(unit, lport, &egr_port);
    EGR_PORT_64r_EM_SRCMOD_CHANGEf_SET(egr_port, 1);
    ioerr += WRITE_EGR_PORT_64r(unit, lport, egr_port);
    ioerr += READ_IEGR_PORT_64r(unit, lport, &iegr_port);
    IEGR_PORT_64r_EM_SRCMOD_CHANGEf_SET(iegr_port, 1);
    ioerr += WRITE_IEGR_PORT_64r(unit, lport, iegr_port);

    /* Configure egress VLAN for backward compatibility */
    EGR_VLAN_CONTROL_1r_CLR(egr_vlan_ctrl1);
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
    int rv = CDK_E_NONE;
    COMMAND_CONFIGr_t command_cfg;
    PAUSE_CONTROLr_t pause_ctrl;
    PAUSE_QUANTr_t pause_quant;
    MAC_PFC_REFRESH_CTRLr_t mac_pfc_refresh;
    TX_IPG_LENGTHr_t tx_ipg_len;
    cdk_pbmp_t pbmp;

    ioerr += _port_init(unit, port);

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    if (CDK_PBMP_MEMBER(pbmp, port)) {
        /* GXMAC init */
        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
        COMMAND_CONFIGr_SW_RESETf_SET(command_cfg, 1);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

        /* Ensure that MAC and loopback mode is disabled */
        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
        COMMAND_CONFIGr_LOOP_ENAf_SET(command_cfg, 0);
        COMMAND_CONFIGr_RX_ENAf_SET(command_cfg, 0);
        COMMAND_CONFIGr_TX_ENAf_SET(command_cfg, 0);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

        ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
        COMMAND_CONFIGr_SW_RESETf_SET(command_cfg, 0);
        ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

        PAUSE_CONTROLr_CLR(pause_ctrl);
        PAUSE_CONTROLr_ENABLEf_SET(pause_ctrl, 1);
        PAUSE_CONTROLr_VALUEf_SET(pause_ctrl, 0x1ffff);
        ioerr += WRITE_PAUSE_CONTROLr(unit, port, pause_ctrl);

        PAUSE_QUANTr_SET(pause_quant, 0xffff);
        ioerr += WRITE_PAUSE_QUANTr(unit, port, pause_quant);

        ioerr += READ_MAC_PFC_REFRESH_CTRLr(unit, port, &mac_pfc_refresh);
        MAC_PFC_REFRESH_CTRLr_PFC_REFRESH_ENf_SET(mac_pfc_refresh, 1);
        MAC_PFC_REFRESH_CTRLr_PFC_REFRESH_TIMERf_SET(mac_pfc_refresh, 0xc000);
        ioerr += WRITE_MAC_PFC_REFRESH_CTRLr(unit, port, mac_pfc_refresh);

        TX_IPG_LENGTHr_SET(tx_ipg_len, 12);
        ioerr += WRITE_TX_IPG_LENGTHr(unit, port, tx_ipg_len);
    }

    return ioerr ? CDK_E_IO : rv;
}

int
bcm56160_a0_xlport_init(int unit, int port)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    XLPORT_ENABLE_REGr_t xlport_enable;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;
    PGW_XL_TXFIFO_CTRLr_t xlport_txfifo_ctrl;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    XLMAC_PAUSE_CTRLr_t xlmac_pause_ctrl;
    XLMAC_RX_MAX_SIZEr_t xlmac_rx_max_size;
    XLMAC_MODEr_t xlmac_mode;
    XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
    XLMAC_PFC_CTRLr_t xlmac_pfc_ctrl;
    cdk_pbmp_t pbmp;
    int lport, speed_max;
    int idx, pidx;

    ioerr += _port_init(unit, port);

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    if (CDK_PBMP_MEMBER(pbmp, port)) {
        speed_max = bcm56160_a0_port_speed_max(unit, port);

        if (XLPORT_SUBPORT(unit, port) == 0) {
            /* Disable XLPORT */
            XLPORT_ENABLE_REGr_CLR(xlport_enable);
            ioerr += WRITE_XLPORT_ENABLE_REGr(unit, xlport_enable, port);

            for (pidx = port; pidx <= (port + 3); pidx++) {
                lport = P2L(unit, pidx);
                if (lport < 0) {
                    continue;
                }

                /* CREDIT reset and MAC count clear */
                ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, pidx, &egr_port_credit_reset);
                EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
                ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, pidx, egr_port_credit_reset);

                ioerr += READ_PGW_XL_TXFIFO_CTRLr(unit, pidx, &xlport_txfifo_ctrl);
                PGW_XL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(xlport_txfifo_ctrl, 1);
                PGW_XL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(xlport_txfifo_ctrl, 1);
                ioerr += WRITE_PGW_XL_TXFIFO_CTRLr(unit, pidx, xlport_txfifo_ctrl);

                BMD_SYS_USLEEP(1000);

                ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, pidx, &egr_port_credit_reset);
                EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
                ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, pidx, egr_port_credit_reset);

                ioerr += READ_PGW_XL_TXFIFO_CTRLr(unit, pidx, &xlport_txfifo_ctrl);
                PGW_XL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(xlport_txfifo_ctrl, 0);
                PGW_XL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(xlport_txfifo_ctrl, 0);
                ioerr += WRITE_PGW_XL_TXFIFO_CTRLr(unit, pidx, xlport_txfifo_ctrl);
            }

            /* Enable XLPORT */
            for (idx = 0; idx <= 3; idx++) {
                lport = P2L(unit, port + idx);
                if (lport < 0) {
                    continue;
                }
                if (idx == 0) {
                    XLPORT_ENABLE_REGr_PORT0f_SET(xlport_enable, 1);
                } else if (idx == 1) {
                    XLPORT_ENABLE_REGr_PORT1f_SET(xlport_enable, 1);
                } else if (idx == 2) {
                    XLPORT_ENABLE_REGr_PORT2f_SET(xlport_enable, 1);
                } else if (idx == 3) {
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
        XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 0);
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
        if (speed_max == 1000) {
            XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, 2);
        } else if (speed_max == 2500) {
            XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, 3);
        }
        ioerr += WRITE_XLMAC_MODEr(unit, port, xlmac_mode);

        ioerr += READ_XLMAC_RX_LSS_CTRLr(unit, port, &xlmac_rx_lss_ctrl);
        XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
        XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
        XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LINK_INTERRUPTf_SET(xlmac_rx_lss_ctrl, 1);
        ioerr += WRITE_XLMAC_RX_LSS_CTRLr(unit, port, xlmac_rx_lss_ctrl);

        /* Disable loopback and bring XLMAC out of reset */
        ioerr += READ_XLMAC_CTRLr(unit, port, &xlmac_ctrl);
        XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 1);
        XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
        ioerr += WRITE_XLMAC_CTRLr(unit, port, xlmac_ctrl);
    }

    return ioerr ? CDK_E_IO : rv;
}

int
bcm56160_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    ING_HW_RESET_CONTROL_1r_t ing_rst_ctl_1;
    ING_HW_RESET_CONTROL_2r_t ing_rst_ctl_2;
    EGR_HW_RESET_CONTROL_0r_t egr_rst_ctl_0;
    EGR_HW_RESET_CONTROL_1r_t egr_rst_ctl_1;
    PGW_GE_MODE_REGr_t pgw_ge_mode;
    XLPORT_MODE_REGr_t xlport_mode;
    XLPORT_SOFT_RESETr_t xlport_sreset;
    XLPORT_MIB_RESETr_t mib_rst;
    GPORT_RSV_MASKr_t gport_rsv_mask;
    GPORT_CONFIGr_t gport_cfg;
    MISCCONFIGr_t misc_cfg;
    CMIC_RATE_ADJUSTr_t rate_adjust;
    CMIC_RATE_ADJUST_INT_MDIOr_t rate_adjust_int_mdio;
    CMIC_MIIM_CONFIGr_t miim_cfg;
    RDBGC0_SELECTr_t rdbgc0_select;
    VLAN_PROFILE_TABm_t vlan_profile;
    ING_VLAN_TAG_ACTION_PROFILEm_t vlan_action;
    EGR_VLAN_TAG_ACTION_PROFILEm_t egr_action;
    ING_PHYS_TO_LOGIC_MAPm_t ing_p2l;
    EGR_LOGIC_TO_PHYS_MAPr_t  egr_l2p;
    MMU_LOGIC_TO_PHYS_MAPr_t mmu_l2p;
    cdk_pbmp_t pbmp, gpbmp, xlpbmp;
    int lport, port, lanes;
    int idx, bidx, gidx;
    int port_ratio;
    int fval;
    int num_lport, num_port;
    int freq, target_freq, divisor, dividend, delay;

    BMD_CHECK_UNIT(unit);

    /* Reset the IPIPE block */
    ING_HW_RESET_CONTROL_1r_CLR(ing_rst_ctl_1);
    ioerr += WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_rst_ctl_1);
    ING_HW_RESET_CONTROL_2r_CLR(ing_rst_ctl_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_rst_ctl_2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_rst_ctl_2, 1);
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_rst_ctl_2, 0x4000);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);

    /* Reset the EPIPE block */
    EGR_HW_RESET_CONTROL_0r_CLR(egr_rst_ctl_0);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_rst_ctl_0);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_rst_ctl_1, 0x2000);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);

    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_ING_HW_RESET_CONTROL_2r(unit, &ing_rst_ctl_2);
        if (ING_HW_RESET_CONTROL_2r_DONEf_GET(ing_rst_ctl_2)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56160_a0_bmd_init[%d]: IPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_EGR_HW_RESET_CONTROL_1r(unit, &egr_rst_ctl_1);
        if (EGR_HW_RESET_CONTROL_1r_DONEf_GET(egr_rst_ctl_1)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm56160_a0_bmd_init[%d]: EPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    /* Clear pipe reset registers */
    ING_HW_RESET_CONTROL_2r_CLR(ing_rst_ctl_2);
    ioerr += WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_rst_ctl_2);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);

    /* Initialize the TDM mode of each PGW_GE blocks */
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_PMQ, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if ((XPORT_SUBPORT(unit, BLKTYPE_PMQ, port) == 0) ||
            (XPORT_SUBPORT(unit, BLKTYPE_PMQ, port) == 8)) {
            port_ratio = bcm56160_a0_qtcport_ratio(unit, port);

            fval = 0;
            if (port_ratio == BLOCK_PORT_RATIO_OCTAL) {
                fval = 8;
            } else if (port_ratio == BLOCK_PORT_RATIO_QUAD) {
                fval = 4;
            } else if (port_ratio == BLOCK_PORT_RATIO_DUAL_1_1) {
                fval = 2;
            } else if (port_ratio == BLOCK_PORT_RATIO_SINGLE) {
                fval = 1;
            }

            bidx = XPORT_BLKIDX(unit, BLKTYPE_PMQ, port);
            gidx = XPORT_SUBPORT(unit, BLKTYPE_PMQ, port) >> 3;
            ioerr += READ_PGW_GE_MODE_REGr(unit, bidx, gidx, &pgw_ge_mode);
            PGW_GE_MODE_REGr_GP0_TDM_MODEf_SET(pgw_ge_mode, fval);
            ioerr += WRITE_PGW_GE_MODE_REGr(unit, bidx, gidx, pgw_ge_mode);
        }
    }

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (XLPORT_SUBPORT(unit, port) == 0) {
            /* Assert XLPORT soft reset */
            XLPORT_SOFT_RESETr_CLR(xlport_sreset);
            for (idx = 0; idx <= 3; idx++) {
                lport = P2L(unit, port + idx);
                if (lport < 0) {
                    continue;
                }
                if (idx == 0) {
                    XLPORT_SOFT_RESETr_PORT0f_SET(xlport_sreset, 1);
                } else if (idx == 1) {
                    XLPORT_SOFT_RESETr_PORT1f_SET(xlport_sreset, 1);
                } else if (idx == 2) {
                    XLPORT_SOFT_RESETr_PORT2f_SET(xlport_sreset, 1);
                } else if (idx == 3) {
                    XLPORT_SOFT_RESETr_PORT3f_SET(xlport_sreset, 1);
                }
            }
            ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlport_sreset, port);

            /* Set XPORT mode */
            port_ratio = bcm56160_a0_xlport_ratio(unit, port);
            if (port_ratio == BLOCK_PORT_RATIO_SINGLE) {
                fval = XLPORT_MODE_SINGLE;
            } else if (port_ratio == BLOCK_PORT_RATIO_DUAL_2_2) {
                fval = XLPORT_MODE_DUAL;
            } else if (port_ratio == BLOCK_PORT_RATIO_TRI_1_1_2) {
                fval = XLPORT_MODE_TRI_012;
            } else {
                fval = XLPORT_MODE_QUAD;
            }

            XLPORT_MODE_REGr_CLR(xlport_mode);
            XLPORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(xlport_mode, fval);
            XLPORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(xlport_mode, fval);
            ioerr += WRITE_XLPORT_MODE_REGr(unit, xlport_mode, port);

            /* De-assert XLPORT soft reset */
            XLPORT_SOFT_RESETr_CLR(xlport_sreset);
            ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlport_sreset, port);
        }
    }

    /* Ingress physical to logical port mapping */
    num_port = ING_PHYS_TO_LOGIC_MAPm_MAX;
    for (port = 0; port <= num_port; port++) {
        lport = P2L(unit, port);
        if (lport == -1) {
            lport = 0x30;
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
        if (port != -1) {
            MMU_LOGIC_TO_PHYS_MAPr_CLR(mmu_l2p);
            MMU_LOGIC_TO_PHYS_MAPr_PHYS_PORTf_SET(mmu_l2p, port);
            ioerr += WRITE_MMU_LOGIC_TO_PHYS_MAPr(unit, lport, mmu_l2p);
        }
    }

    /* Enable Field Processor metering clock */
    ioerr += READ_MISCCONFIGr(unit, &misc_cfg);
    MISCCONFIGr_METERING_CLK_ENf_SET(misc_cfg, 1);
    ioerr += WRITE_MISCCONFIGr(unit, misc_cfg);

    freq = 300;
    if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ111) {
        freq = 111;
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ230) {
        freq = 230;
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ75) {
        freq = 75;
    }

    /*
     * Set external MDIO freq to around 10MHz
     * target_freq = core_clock_freq * DIVIDEND / DIVISOR / 2
     */
    target_freq = 10;
    divisor = (freq + (target_freq * 2 - 1)) / (target_freq * 2);
    dividend = 1;

    CMIC_RATE_ADJUSTr_CLR(rate_adjust);
    CMIC_RATE_ADJUSTr_DIVISORf_SET(rate_adjust, divisor);
    CMIC_RATE_ADJUSTr_DIVIDENDf_SET(rate_adjust, dividend);
    ioerr += WRITE_CMIC_RATE_ADJUSTr(unit, rate_adjust);

    /*
     * Set internal MDIO freq to around 10MHz
     * Valid range is from 2.5MHz to 12.5MHz
     * target_freq = core_clock_freq * DIVIDEND / DIVISOR / 2
     * or DIVISOR = core_clock_freq * DIVIDENT / (target_freq * 2)
     */
    target_freq = 10;
    divisor = (freq + (target_freq * 2 - 1)) / (target_freq * 2);
    dividend = 1;

    CMIC_RATE_ADJUST_INT_MDIOr_CLR(rate_adjust_int_mdio);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVISORf_SET(rate_adjust_int_mdio, divisor);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVIDENDf_SET(rate_adjust_int_mdio, dividend);
    ioerr += WRITE_CMIC_RATE_ADJUST_INT_MDIOr(unit, rate_adjust_int_mdio);

    delay = 0;
    if (delay >= 1 && delay <= 15) {
        ioerr += READ_CMIC_MIIM_CONFIGr(unit, &miim_cfg);
        CMIC_MIIM_CONFIGr_MDIO_OUT_DELAYf_SET(miim_cfg, delay);
        ioerr += WRITE_CMIC_MIIM_CONFIGr(unit, miim_cfg);
    }

    /* Configure discard counter */
    RDBGC0_SELECTr_CLR(rdbgc0_select);
    RDBGC0_SELECTr_BITMAPf_SET(rdbgc0_select, 0x0400ad11);
    ioerr += WRITE_RDBGC0_SELECTr(unit, rdbgc0_select);

    /* Initialize MMU */
    rv = _mmu_init(unit);

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

    if (!(CDK_XGSD_FLAGS(unit) & CHIP_FLAG_NO_TSC)) {
        /* Reset XLPORT MIB counter */
        XLPORT_MIB_RESETr_SET(mib_rst, 0xf);
        ioerr += WRITE_XLPORT_MIB_RESETr(unit, mib_rst, -1);
        XLPORT_MIB_RESETr_SET(mib_rst, 0x0);
        ioerr += WRITE_XLPORT_MIB_RESETr(unit, mib_rst, -1);
    }

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
    bcm56160_a0_gport_pbmp_get(unit, &gpbmp);
    CDK_PBMP_ITER(gpbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv += _gport_init(unit, port);
        }
    }

    /* Configure XLPORTs */
    bcm56160_a0_xlport_pbmp_get(unit, &xlpbmp);
    CDK_PBMP_ITER(xlpbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv += bcm56160_a0_xlport_init(unit, port);
        }
    }

    CDK_PBMP_ITER(gpbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }
        if (CDK_SUCCESS(rv)) {
            lanes = bcm56160_a0_port_num_lanes(unit, port);
            if (lanes == 4) {
                rv = bmd_phy_mode_set(unit, port, "qtce",
                                      BMD_PHY_MODE_SERDES, 0);
            } else { /* lanes = 1 */
                rv = bmd_phy_mode_set(unit, port, "qtce",
                                      BMD_PHY_MODE_SERDES, 1);
                rv = bmd_phy_mode_set(unit, port, "qtce",
                                      BMD_PHY_MODE_2LANE, 0);
            }
        }
        
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_fw_helper_set(unit, port, _firmware_helper);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_init(unit, port);
        }
        /* Set to serdes if port is passthru mode. */
        if (CDK_SUCCESS(rv)) {
            if (CDK_PORT_CONFIG_PORT_FLAGS(unit, port) & CDK_DCFG_PORT_F_PASSTHRU) {
                rv = bmd_phy_mode_set(unit, port, "qtce", BMD_PHY_MODE_PASSTHRU, 1);
            }
        }
    }

    CDK_PBMP_ITER(xlpbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }
        if (CDK_SUCCESS(rv)) {
            lanes = bcm56160_a0_port_num_lanes(unit, port);
            if (lanes == 4) {
                rv = bmd_phy_mode_set(unit, port, "tsce",
                                      BMD_PHY_MODE_SERDES, 0);
            } else if (lanes == 2) {
                rv = bmd_phy_mode_set(unit, port, "tsce",
                                      BMD_PHY_MODE_SERDES, 1);
                rv = bmd_phy_mode_set(unit, port, "tsce",
                                      BMD_PHY_MODE_2LANE, 1);
            } else { /* lanes = 1 */
                rv = bmd_phy_mode_set(unit, port, "tsce",
                                      BMD_PHY_MODE_SERDES, 1);
                rv = bmd_phy_mode_set(unit, port, "tsce",
                                      BMD_PHY_MODE_2LANE, 0);
            }
        }
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_fw_helper_set(unit, port, _firmware_helper);
        }
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_init(unit, port);
        }
        /* Set to serdes if port is passthru mode. */
        if (CDK_SUCCESS(rv)) {
            if (CDK_PORT_CONFIG_PORT_FLAGS(unit, port) & CDK_DCFG_PORT_F_PASSTHRU) {
                rv = bmd_phy_mode_set(unit, port, "tsce", BMD_PHY_MODE_PASSTHRU, 1);
            }
        }
    }

#if BMD_CONFIG_INCLUDE_DMA
    /* Common port initialization for CPU port */
    ioerr += _port_init(unit, CMIC_PORT);
    if (CDK_SUCCESS(rv)) {
        rv = bmd_xgsd_dma_init(unit);
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
#endif /* CDK_CONFIG_INCLUDE_BCM56160_A0 */
