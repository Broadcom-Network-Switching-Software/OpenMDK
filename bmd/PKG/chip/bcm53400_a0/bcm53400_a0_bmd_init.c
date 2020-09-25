/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53400_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_dma.h>
#include <bmd/bmd_device.h>
#include <bmdi/arch/xgsd_dma.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/arch/xgsd_schan.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_util.h>
#include <cdk/chip/bcm53400_a0_defs.h>

#include <bmd/bmd_phy_ctrl.h>

#include "bcm53400_a0_bmd.h"
#include "bcm53400_a0_internal.h"

#define PIPE_RESET_TIMEOUT_MSEC         5
#define SBUSDMA_TIMEOUT_USEC            500

#define JUMBO_MAXSZ                     0x3fe8

#define CMIC_NUM_PKT_DMA_CHAN           4

#define FW_ALIGN_BYTES                  16
#define FW_ALIGN_MASK                   (FW_ALIGN_BYTES - 1)

/* MMU related */
/* Greyhound uses 2MB/1MB of internal Buffers */
#define MMU_BUFFER_SIZE_2MB         (2 * 1024 * 1024) 
#define MMU_BUFFER_SIZE_1MB         (1 * 1024 * 1024) 

/* For SKU with 1MB buffer pool, use the following line instead */
/* MMU_TOTAL_CELLS = MMU_BUFFER_SIZE_1MB / MMU_CELL_SIZE; */
#define MMU_TOTAL_CELLS             (MMU_BUFFER_SIZE_2MB / MMU_CELL_SIZE)
#define MMU_NUM_COS                 8
#define MMU_CELL_SIZE               128
#define MMU_IN_COS_MIN_CELLS        16
#define MMU_IN_COS_MIN_XQS          16
#define MMU_XOFF_CELL_LIMIT         80
#define MMU_RESET_CELL_LIMIT        40
#define MMU_IN_PORT_STATIC_CELLS    (MMU_NUM_COS * MMU_IN_COS_MIN_CELLS) 
#define MMU_IN_PORT_TOTAL_XQS       6144 
#define MMU_IN_PORT_STATIC_XQS      (MMU_NUM_COS * MMU_IN_COS_MIN_XQS) 
#define MMU_IN_PORT_DYNAMIC_XQS     (MMU_IN_PORT_TOTAL_XQS - MMU_IN_PORT_STATIC_XQS - 16) 
#define MMU_IN_COS_XQ_LIMIT         (MMU_IN_PORT_DYNAMIC_XQS + MMU_IN_COS_MIN_XQS)

#if BMD_CONFIG_INCLUDE_DMA
/* DMA buffer for TSCE ucode loading */
static struct phy_dma_buf_s {
    uint8_t *laddr;         /* logical address */
    dma_addr_t paddr;       /* physical address */
    uint32_t size;          /* DMA buffer size */
} phy_dma_buf[BMD_CONFIG_MAX_UNITS];

static int
_sbusdma_mem_write(int unit, uint32_t blkacc, int port, uint32_t offset,
                   int data_beats, int index_begin, int index_end, 
                   dma_addr_t dma_buf_addr)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    const cdk_xgsd_block_t *blkp = CDK_XGSD_INFO(unit)->blocks;
    uint32_t adext = CDK_XGSD_BLKACC2ADEXT(blkacc);
    cdk_pbmp_t pbmp;
    uint32_t acctype, dstblk, datalen;
    int block, blktype, idx, idx1;
    schan_msg_t schan_msg;
    uint32_t start_addr;
    uint32_t spacing;
    int count;
    CMIC_CMC_SBUSDMA_CH1_CONTROLr_t sbusdma_ch1_ctrl;
    CMIC_CMC_SBUSDMA_CH1_OPCODEr_t sbusdma_ch1_opcode;
    CMIC_CMC_SBUSDMA_CH1_HOSTMEM_START_ADDRESSr_t sbusdma_ch1_hostaddr;
    CMIC_CMC_SBUSDMA_CH1_SBUS_START_ADDRESSr_t sbusdma_ch1_addr;
    CMIC_CMC_SBUSDMA_CH1_COUNTr_t sbusdma_ch1_count;
    CMIC_CMC_SBUSDMA_CH1_REQUESTr_t sbusdma_ch1_request;
    CMIC_CMC_SBUSDMA_CH1_STATUSr_t sbusdma_ch1_status;

    CDK_ASSERT(blkp);

    /* Get the block number, block type and address from given blkacc
     * and physical port number.
     */
    block = -1;
    for (idx = 0; idx < CDK_XGSD_INFO(unit)->nblocks; idx++, blkp++) {
        if ((1 << blkp->type) & blkacc) {
            CDK_PBMP_ASSIGN(pbmp, blkp->pbmps);
            if (blkp->ptype == 0) {
                CDK_PBMP_AND(pbmp, CDK_XGSD_INFO(unit)->valid_pbmps);
            }

            /* Skip unused blocks */
            if ((port < 0  && CDK_PBMP_NOT_NULL(pbmp)) ||
                CDK_PBMP_MEMBER(blkp->pbmps, port)) {
                block = blkp->blknum;

                /* Calculate the absolute address */
                start_addr = cdk_xgsd_blockport_addr(unit, block, -1, offset, 0);
                /* Calculate the blocktype */
                cdk_xgsd_block_type(unit, block, &blktype, NULL);
                /* Update address extension with specified block */
                CDK_XGSD_ADEXT_BLOCK_SET(adext, block);
                break;
            }
        }
    }
    if (block < 0) {
        return CDK_E_FAIL;
    }


    acctype = CDK_XGSD_ADEXT2ACCTYPE(adext);
    dstblk = CDK_XGSD_ADEXT2BLOCK(adext);
    datalen = data_beats * sizeof(uint32_t);

    count = index_end - index_begin + 1;
    if (count < 1) {
        return CDK_E_NONE;
    }

    /* Construct S-channel message header */
    SCHAN_MSG_CLEAR(&schan_msg);
    SCMH_OPCODE_SET(schan_msg.header, WRITE_MEMORY_CMD_MSG);
    SCMH_ACCTYPE_SET(schan_msg.header, acctype);
    SCMH_DSTBLK_SET(schan_msg.header, dstblk);
    SCMH_DATALEN_SET(schan_msg.header, datalen);

    /* Set mode, clear abort, start (clears status and errors) */
    ioerr += READ_CMIC_CMC_SBUSDMA_CH1_CONTROLr(unit, &sbusdma_ch1_ctrl);
    CMIC_CMC_SBUSDMA_CH1_CONTROLr_MODEf_SET(sbusdma_ch1_ctrl, 0);
    CMIC_CMC_SBUSDMA_CH1_CONTROLr_ABORTf_SET(sbusdma_ch1_ctrl, 0);
    CMIC_CMC_SBUSDMA_CH1_CONTROLr_STARTf_SET(sbusdma_ch1_ctrl, 0);
    ioerr += WRITE_CMIC_CMC_SBUSDMA_CH1_CONTROLr(unit, sbusdma_ch1_ctrl);

    /* Set first S-channel control word as opcode. */
    CMIC_CMC_SBUSDMA_CH1_OPCODEr_SET(sbusdma_ch1_opcode, schan_msg.dwords[0]);
    ioerr += WRITE_CMIC_CMC_SBUSDMA_CH1_OPCODEr(unit, sbusdma_ch1_opcode);
    CMIC_CMC_SBUSDMA_CH1_HOSTMEM_START_ADDRESSr_SET(sbusdma_ch1_hostaddr, (uint32_t)dma_buf_addr);
    ioerr += WRITE_CMIC_CMC_SBUSDMA_CH1_HOSTMEM_START_ADDRESSr(unit, sbusdma_ch1_hostaddr);
    CMIC_CMC_SBUSDMA_CH1_SBUS_START_ADDRESSr_SET(sbusdma_ch1_addr, start_addr);
    ioerr += WRITE_CMIC_CMC_SBUSDMA_CH1_SBUS_START_ADDRESSr(unit, sbusdma_ch1_addr);
    CMIC_CMC_SBUSDMA_CH1_COUNTr_SET(sbusdma_ch1_count, count);
    ioerr += WRITE_CMIC_CMC_SBUSDMA_CH1_COUNTr(unit, sbusdma_ch1_count);

    /* Program beats, increment/decrement, single etc. */
    ioerr += READ_CMIC_CMC_SBUSDMA_CH1_REQUESTr(unit, &sbusdma_ch1_request);
    CMIC_CMC_SBUSDMA_CH1_REQUESTr_REQ_WORDSf_SET(sbusdma_ch1_request, data_beats);
    CMIC_CMC_SBUSDMA_CH1_REQUESTr_REP_WORDSf_SET(sbusdma_ch1_request, 0);
    CMIC_CMC_SBUSDMA_CH1_REQUESTr_INCR_SHIFTf_SET(sbusdma_ch1_request, 0);
    CMIC_CMC_SBUSDMA_CH1_REQUESTr_REQ_SINGLEf_SET(sbusdma_ch1_request, 0);
    CMIC_CMC_SBUSDMA_CH1_REQUESTr_DECRf_SET(sbusdma_ch1_request, 0);

    spacing = 0;
    if (blktype == BLKTYPE_IPIPE || blktype == BLKTYPE_EPIPE) {
        spacing = data_beats > 7 ? data_beats + 1 : 8;
    }
    CMIC_CMC_SBUSDMA_CH1_REQUESTr_PEND_CLOCKSf_SET(sbusdma_ch1_request, spacing);
    ioerr += WRITE_CMIC_CMC_SBUSDMA_CH1_REQUESTr(unit, sbusdma_ch1_request);

    /* Start DMA */
    ioerr += READ_CMIC_CMC_SBUSDMA_CH1_CONTROLr(unit, &sbusdma_ch1_ctrl);
    CMIC_CMC_SBUSDMA_CH1_CONTROLr_STARTf_SET(sbusdma_ch1_ctrl, 1);
    ioerr += WRITE_CMIC_CMC_SBUSDMA_CH1_CONTROLr(unit, sbusdma_ch1_ctrl);

    /* Poll the DMA status */
    for (idx1 = 0; idx1 < SBUSDMA_TIMEOUT_USEC; idx1++) {
        ioerr += READ_CMIC_CMC_SBUSDMA_CH1_STATUSr(unit, &sbusdma_ch1_status);
        if (CMIC_CMC_SBUSDMA_CH1_STATUSr_DONEf_GET(sbusdma_ch1_status)) {
            if (CMIC_CMC_SBUSDMA_CH1_STATUSr_ERRORf_GET(sbusdma_ch1_status)) {
                rv = CDK_E_FAIL;
            }
            break;
        }
        BMD_SYS_USLEEP(100);
    }
    if (idx1 >= SBUSDMA_TIMEOUT_USEC) {
        CDK_WARN(("bcm53400_a0_bmd_init[%d]: SBUSDMA timeout\n", unit));
        rv = CDK_E_TIMEOUT;

        /* Abort SBUSDMA */
        ioerr += READ_CMIC_CMC_SBUSDMA_CH1_CONTROLr(unit, &sbusdma_ch1_ctrl);
        CMIC_CMC_SBUSDMA_CH1_CONTROLr_ABORTf_SET(sbusdma_ch1_ctrl, 1);
        ioerr += WRITE_CMIC_CMC_SBUSDMA_CH1_CONTROLr(unit, sbusdma_ch1_ctrl);

        for (idx1 = 0; idx1 < SBUSDMA_TIMEOUT_USEC; idx1++) {
            ioerr += READ_CMIC_CMC_SBUSDMA_CH1_STATUSr(unit, &sbusdma_ch1_status);
            if (CMIC_CMC_SBUSDMA_CH1_STATUSr_DONEf_GET(sbusdma_ch1_status)) {
                break;
            }
            BMD_SYS_USLEEP(100);
        }
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* BMD_CONFIG_INCLUDE_DMA */

static int
_firmware_helper(void *ctx, uint32_t offset, uint32_t size, void *data)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    XLPORT_WC_UCMEM_CTRLr_t ucmem_ctrl;
    XLPORT_WC_UCMEM_DATAm_t ucmem_data;
    int unit, port;
    const char *drv_name;
    uint32_t wbuf[4];
    uint32_t wdata;
    uint32_t *fw_data;
    uint32_t *fw_entry;
    uint32_t fw_size;
    uint32_t idx, wdx;
    uint32_t be_host;
#if BMD_CONFIG_INCLUDE_DMA
    struct phy_dma_buf_s *dbuf;
    int s_match;
#endif /* BMD_CONFIG_INCLUDE_DMA */

    /* Get unit, port and driver name from context */
    bmd_phy_fw_info_get(ctx, &unit, &port, &drv_name);

    if (CDK_STRSTR(drv_name, "tsce") == NULL) {
        return CDK_E_UNAVAIL;
    }

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

    /* Enable parallel bus access */
    ioerr += READ_XLPORT_WC_UCMEM_CTRLr(unit, &ucmem_ctrl, port);
    XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(ucmem_ctrl, 1);
    ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, ucmem_ctrl, port);

    /* We need to byte swap on big endian host */
    be_host = 1;
    if (*((uint8_t *)&be_host) == 1) {
        be_host = 0;
    }

#if BMD_CONFIG_INCLUDE_DMA
    dbuf = &phy_dma_buf[unit];

    /* Check if the DMA buffer need allocate */
    if (dbuf->laddr == NULL) {
        dbuf->size = fw_size;
        dbuf->laddr = bmd_dma_alloc_coherent(unit, dbuf->size, &dbuf->paddr);
        if (dbuf->laddr == NULL) {
            CDK_WARN(("bcm53400_a0_bmd_init[%d]: DMA allocation failed\n", unit));
        } else {
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
                    *(((uint32_t *)dbuf->laddr + ((idx >> 2) + wdx))) = wdata;
                }
            }
        }
    }

    s_match = 1;
    if (dbuf->size != fw_size) {
        s_match = 0;
        CDK_WARN(("bcm53400_a0_bmd_init[%d]: DMA buffer size mismatch.\n", unit));
    }

    if (dbuf->laddr && s_match) {
        rv = _sbusdma_mem_write(unit, XLPORT_WC_UCMEM_DATAm_BLKACC,
                                port, XLPORT_WC_UCMEM_DATAm,
                                (XLPORT_WC_UCMEM_DATAm_SIZE >> 2),
                                0, (fw_size >> 4), dbuf->paddr);
    } else
#endif /* BMD_CONFIG_INCLUDE_DMA */
    {
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
                XLPORT_WC_UCMEM_DATAm_SET(ucmem_data, wdx, wdata);
            }
            WRITE_XLPORT_WC_UCMEM_DATAm(unit, idx >> 4, ucmem_data, port);
        }
    }

    /* Disable parallel bus access */
    XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(ucmem_ctrl, 0);
    ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, ucmem_ctrl, port);

    return ioerr ? CDK_E_IO : rv;
}

static int
_mmu_tdm_init(int unit)
{
    int ioerr = 0;
    IARB_TDM_CONTROLr_t iarb_tdm_ctrl;
    IARB_TDM_TABLEm_t iarb_tdm;
    MMU_ARB_TDM_TABLEm_t mmu_arb_tdm;
    const int *default_tdm_seq;
    int tdm_seq[128], tdm_size, tdm_max;
    int idx;
    cdk_pbmp_t pbmp;
    int step, sub_port;
    int port, mport;

    /* Get default TDM sequence for this configuration */
    tdm_size = bcm53400_a0_tdm_default(unit, &default_tdm_seq);
    if (tdm_size <= 0 || tdm_size > COUNTOF(tdm_seq)) {
        return CDK_E_INTERNAL;
    }
    tdm_max = tdm_size - 1;

    /* Make local copy of TDM sequence */
    for (idx = 0; idx < tdm_size; idx++) {
        tdm_seq[idx] = default_tdm_seq[idx];
    }

    /* Update TDM sequence for flex XLPORT port 2. */
    bcm53400_a0_xport_pbmp_get(unit, XPORT_FLAG_XLPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (port == 2) {
            step = 0;
            if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_FLEX) {
                if (BMD_PORT_PROPERTIES(unit, port + 1) != BMD_PORT_FLEX) {
                    /* Sub-port 1 is configured: assume quad mode */
                    step = 1;
                } else if (BMD_PORT_PROPERTIES(unit, port + 2) != BMD_PORT_FLEX) {
                    /* Sub-port 2 is configured: assume dual mode */
                    step = 2;
                }
    
                /* According the enabled sub-ports to replace the TDM table*/
                sub_port = 0;
                for (idx = 0; idx < tdm_size; idx++) {
                    if ((tdm_seq[idx] - XLPORT_SUBPORT(unit, tdm_seq[idx])) == port) {
                        tdm_seq[idx] = (port + sub_port);
                        sub_port += step;
                        if (sub_port > 3) {
                            sub_port = 0;
                        }
                    }
                }
            }
        }
    }
    
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
    cdk_pbmp_t pbmp, mmu_pbmp;
    uint32_t pbm = 0;
    int port, mport, idx;
    int mmu_total_dyn_cells;
    uint32_t cell_resume_delta;
    uint32_t dyn_cell_reset_delta;
    uint32_t max_cell_limit_pri0;
    uint32_t max_cell_limit_pri1_7;
    uint32_t dyn_cell_limit;
    uint32_t xoff_pkt_limit;
    uint32_t reset_pkt_limit;
    MISCCONFIGr_t misc_cfg;
    CFAPCONFIGr_t cfapconfig;
    PKTAGINGTIMERr_t pkt_ag_tim;
    PKTAGINGLIMITr_t pkt_ag_lim;
    MMUPORTENABLEr_t mmu_port_en;
    IP_TO_CMICM_CREDIT_TRANSFERr_t cmic_cred_xfer;
    HOLCOSPKTSETLIMITr_t holcos_set_limit;
    HOLCOSPKTRESETLIMITr_t holcos_reset_limit;
    HOLCOSMINXQCNTr_t holcos_min_cnt;
    LWMCOSCELLSETLIMITr_t lwmcos_set_limit;
    HOLCOSCELLMAXLIMITr_t holcos_max_limit;
    PGCELLLIMITr_t pg_limit;
    PGDISCARDSETLIMITr_t pg_discard_limit;
    DYNXQCNTPORTr_t dyn_xqcnt;
    DYNRESETLIMPORTr_t dyn_reset_limit;
    DYNCELLLIMITr_t dyn_limit;
    TOTALDYNCELLSETLIMITr_t total_set_limit;
    TOTALDYNCELLRESETLIMITr_t total_reset_limit;
    IBPPKTSETLIMITr_t ibp_set_limit;
    IE2E_CONTROLr_t ie2e_ctrl;
    E2EFC_CNT_SET_LIMITr_t e2efc_set_limit;
    E2EFC_CNT_RESET_LIMITr_t e2efc_reset_limit;
    E2EFC_CNT_DISC_LIMITr_t e2efc_disc_limit;
    E2EFC_IBP_ENr_t e2efc_ibp_en;

    /* Setup TDM for MMU */
    rv = _mmu_tdm_init(unit);

    /* Ports to configure */
    CDK_PBMP_CLEAR(mmu_pbmp);
    CDK_PBMP_ADD(mmu_pbmp, CMIC_PORT);
    bcm53400_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &pbmp);
    CDK_PBMP_OR(mmu_pbmp, pbmp);

    ioerr += READ_CFAPCONFIGr(unit, &cfapconfig);
    CFAPCONFIGr_CFAPPOOLSIZEf_SET(cfapconfig, MMU_CFAPm_MAX);
    ioerr += WRITE_CFAPCONFIGr(unit, cfapconfig);

    ioerr += READ_MISCCONFIGr(unit, &misc_cfg);
    MISCCONFIGr_DYN_XQ_ENf_SET(misc_cfg, 1);
    MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_SET(misc_cfg, 1);
    MISCCONFIGr_SKIDMARKERf_SET(misc_cfg, 3);
    ioerr += WRITE_MISCCONFIGr(unit, misc_cfg);

    /* Enable IP to CMICM credit transfer */
    IP_TO_CMICM_CREDIT_TRANSFERr_CLR(cmic_cred_xfer);
    IP_TO_CMICM_CREDIT_TRANSFERr_TRANSFER_ENABLEf_SET(cmic_cred_xfer, 1);
    IP_TO_CMICM_CREDIT_TRANSFERr_NUM_OF_CREDITSf_SET(cmic_cred_xfer, 32);
    ioerr += WRITE_IP_TO_CMICM_CREDIT_TRANSFERr(unit, cmic_cred_xfer);

    max_cell_limit_pri0 = 7776;
    max_cell_limit_pri1_7 = 48;
    xoff_pkt_limit = 12;
    reset_pkt_limit = 0;
    cell_resume_delta = 12;
    dyn_cell_reset_delta = 24;
    mmu_total_dyn_cells = MMU_TOTAL_CELLS - (29 * MMU_IN_PORT_STATIC_CELLS);
    if ((CDK_XGSD_FLAGS(unit) & CHIP_FLAG_MIXED) ||
        ((CDK_XGSD_FLAGS(unit) & CHIP_FLAG_X2P5G) &&
         !(CDK_XGSD_FLAGS(unit) & CHIP_FLAG_GE)) ||
        ((CDK_XGSD_FLAGS(unit) & CHIP_FLAG_X1G) &&
         (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_HPC))) {
        max_cell_limit_pri0 = 3456;
        max_cell_limit_pri1_7 = 48;
        xoff_pkt_limit = 80;
        reset_pkt_limit = 0;
        mmu_total_dyn_cells = MMU_TOTAL_CELLS - (25 * MMU_IN_PORT_STATIC_CELLS);
    } else if ((CDK_XGSD_FLAGS(unit) & CHIP_FLAG_X10G) ||
               ((CDK_XGSD_FLAGS(unit) & CHIP_FLAG_X1G) &&
                (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_LPC))) {
        max_cell_limit_pri0 = 4096;
        max_cell_limit_pri1_7 = 4096;
        xoff_pkt_limit = 80;
        reset_pkt_limit = 1;
        cell_resume_delta = 16;
        dyn_cell_reset_delta = 32;
        mmu_total_dyn_cells = MMU_TOTAL_CELLS - (17 * MMU_IN_PORT_STATIC_CELLS);
    } 

    dyn_cell_limit = max_cell_limit_pri0 + 
                     ((MMU_NUM_COS - 1) * 4 * (1536 / MMU_CELL_SIZE)); 

    CDK_PBMP_ITER(mmu_pbmp, port) {
        mport = P2M(unit, port);
        if (mport < 0) {
            continue;
        }
        pbm |= 1 << mport;
        
        for (idx = 0; idx < MMU_NUM_COS; idx++) {
            /* 
             * The HOLCOSPKTSETLIMITr register controls BOTH the XQ 
             * size per cosq AND the HOL set limit for that cosq.
             */
            ioerr += READ_HOLCOSPKTSETLIMITr(unit, mport, idx, &holcos_set_limit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcos_set_limit, MMU_IN_COS_XQ_LIMIT);
            ioerr += WRITE_HOLCOSPKTSETLIMITr(unit, mport, idx, holcos_set_limit);
             
            /* Reset limit is set limit - 4 */
            ioerr += READ_HOLCOSPKTRESETLIMITr(unit, mport, idx, &holcos_reset_limit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcos_reset_limit, 
                                                        (MMU_IN_COS_XQ_LIMIT - 4));
            ioerr += WRITE_HOLCOSPKTRESETLIMITr(unit, mport, idx, holcos_reset_limit);

            HOLCOSMINXQCNTr_CLR(holcos_min_cnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcos_min_cnt, MMU_IN_COS_MIN_XQS);
            ioerr += WRITE_HOLCOSMINXQCNTr(unit, mport, idx, holcos_min_cnt);

            LWMCOSCELLSETLIMITr_CLR(lwmcos_set_limit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcos_set_limit, MMU_IN_COS_MIN_CELLS);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcos_set_limit, MMU_IN_COS_MIN_CELLS);
            ioerr += WRITE_LWMCOSCELLSETLIMITr(unit, mport, idx, lwmcos_set_limit);

            if(!idx) {
                HOLCOSCELLMAXLIMITr_CLR(holcos_max_limit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcos_max_limit, max_cell_limit_pri0);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcos_max_limit, 
                                            (max_cell_limit_pri0 - cell_resume_delta));
                ioerr += WRITE_HOLCOSCELLMAXLIMITr(unit, mport, idx, holcos_max_limit);
            } else {
                HOLCOSCELLMAXLIMITr_CLR(holcos_max_limit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcos_max_limit,
                                                      max_cell_limit_pri1_7);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcos_max_limit, 
                                            (max_cell_limit_pri1_7 - cell_resume_delta));
                ioerr += WRITE_HOLCOSCELLMAXLIMITr(unit, mport, idx, holcos_max_limit);
            }

            PGCELLLIMITr_CLR(pg_limit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pg_limit, MMU_XOFF_CELL_LIMIT);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pg_limit, MMU_RESET_CELL_LIMIT);
            ioerr += WRITE_PGCELLLIMITr(unit, mport, idx, pg_limit);

            PGDISCARDSETLIMITr_CLR(pg_discard_limit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pg_discard_limit,
                                                    (MMU_TOTAL_CELLS - 1));
            ioerr += WRITE_PGDISCARDSETLIMITr(unit, mport, idx, pg_discard_limit);
        }

        DYNXQCNTPORTr_CLR(dyn_xqcnt);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dyn_xqcnt, MMU_IN_PORT_DYNAMIC_XQS);
        ioerr += WRITE_DYNXQCNTPORTr(unit, mport, dyn_xqcnt);
        
        DYNRESETLIMPORTr_CLR(dyn_reset_limit);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dyn_reset_limit, 
                                              (MMU_IN_PORT_DYNAMIC_XQS - 16));
        ioerr += WRITE_DYNRESETLIMPORTr(unit, mport, dyn_reset_limit);
        
        DYNCELLLIMITr_CLR(dyn_limit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyn_limit, dyn_cell_limit);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyn_limit, dyn_cell_limit - 24);
        ioerr += WRITE_DYNCELLLIMITr(unit, mport, dyn_limit);
        
        TOTALDYNCELLSETLIMITr_CLR(total_set_limit);
        TOTALDYNCELLSETLIMITr_TOTALDYNCELLSETLIMITf_SET(total_set_limit, 
                                                        mmu_total_dyn_cells);
        ioerr += WRITE_TOTALDYNCELLSETLIMITr(unit, total_set_limit);
        
        TOTALDYNCELLRESETLIMITr_CLR(total_reset_limit);
        TOTALDYNCELLRESETLIMITr_TOTALDYNCELLRESETLIMITf_SET(total_reset_limit, 
                                    (mmu_total_dyn_cells - dyn_cell_reset_delta));
        ioerr += WRITE_TOTALDYNCELLRESETLIMITr(unit, total_reset_limit);
        
        IBPPKTSETLIMITr_CLR(ibp_set_limit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibp_set_limit, xoff_pkt_limit);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibp_set_limit, reset_pkt_limit);
        ioerr += WRITE_IBPPKTSETLIMITr(unit, mport, ibp_set_limit);
        
        /* E2EFC configuration */
        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
            ioerr += READ_IE2E_CONTROLr(unit, mport, &ie2e_ctrl);
            IE2E_CONTROLr_IBP_ENABLEf_SET(ie2e_ctrl, 1);
            ioerr += WRITE_IE2E_CONTROLr(unit, mport, ie2e_ctrl);

            E2EFC_CNT_SET_LIMITr_CLR(e2efc_set_limit);
            E2EFC_CNT_SET_LIMITr_PKT_SET_LIMITf_SET(e2efc_set_limit, 1456);
            E2EFC_CNT_SET_LIMITr_CELL_SET_LIMITf_SET(e2efc_set_limit, 9504);
            ioerr += WRITE_E2EFC_CNT_SET_LIMITr(unit, mport, e2efc_set_limit);

            E2EFC_CNT_RESET_LIMITr_CLR(e2efc_reset_limit);
            E2EFC_CNT_RESET_LIMITr_PKT_RESET_LIMITf_SET(e2efc_reset_limit, 1456);
            E2EFC_CNT_RESET_LIMITr_CELL_RESET_LIMITf_SET(e2efc_reset_limit, 9504);
            ioerr += WRITE_E2EFC_CNT_RESET_LIMITr(unit, mport, e2efc_reset_limit);

            E2EFC_CNT_DISC_LIMITr_CLR(e2efc_disc_limit);
            E2EFC_CNT_DISC_LIMITr_PKT_DISC_LIMITf_SET(e2efc_disc_limit, 1456);
            E2EFC_CNT_DISC_LIMITr_CELL_DISC_LIMITf_SET(e2efc_disc_limit, 9504);
            ioerr += WRITE_E2EFC_CNT_DISC_LIMITr(unit, mport, e2efc_disc_limit);
        } else {
            E2EFC_CNT_SET_LIMITr_CLR(e2efc_set_limit);
            E2EFC_CNT_SET_LIMITr_PKT_SET_LIMITf_SET(e2efc_set_limit, 12);
            E2EFC_CNT_SET_LIMITr_CELL_SET_LIMITf_SET(e2efc_set_limit, 80);
            ioerr += WRITE_E2EFC_CNT_SET_LIMITr(unit, mport, e2efc_set_limit);

            E2EFC_CNT_RESET_LIMITr_CLR(e2efc_reset_limit);
            E2EFC_CNT_RESET_LIMITr_PKT_RESET_LIMITf_SET(e2efc_reset_limit, 9);
            E2EFC_CNT_RESET_LIMITr_CELL_RESET_LIMITf_SET(e2efc_reset_limit, 60);
            ioerr += WRITE_E2EFC_CNT_RESET_LIMITr(unit, mport, e2efc_reset_limit);

            E2EFC_CNT_DISC_LIMITr_CLR(e2efc_disc_limit);
            E2EFC_CNT_DISC_LIMITr_PKT_DISC_LIMITf_SET(e2efc_disc_limit, 1456);
            E2EFC_CNT_DISC_LIMITr_CELL_DISC_LIMITf_SET(e2efc_disc_limit, 9504);
            ioerr += WRITE_E2EFC_CNT_DISC_LIMITr(unit, mport, e2efc_disc_limit);
        }
    }
    /* E2EFC configuration */
    ioerr += READ_E2EFC_IBP_ENr(unit, &e2efc_ibp_en);
    E2EFC_IBP_ENr_ENf_SET(e2efc_ibp_en, 1);
    ioerr += WRITE_E2EFC_IBP_ENr(unit, e2efc_ibp_en);

    /* Disable packet aging on all COSQs */
    PKTAGINGTIMERr_CLR(pkt_ag_tim);
    ioerr += WRITE_PKTAGINGTIMERr(unit, pkt_ag_tim);
    PKTAGINGLIMITr_CLR(pkt_ag_lim);
    ioerr += WRITE_PKTAGINGLIMITr(unit, pkt_ag_lim);

    /* Port enable */
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
_gxport_init(int unit, int port)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    COMMAND_CONFIGr_t command_cfg;
    PAUSE_CONTROLr_t pause_ctrl;
    PAUSE_QUANTr_t pause_quant;
    MAC_PFC_REFRESH_CTRLr_t mac_pfc_refresh;
    TX_IPG_LENGTHr_t tx_ipg_len;
    int speed;

    ioerr += _port_init(unit, port);

    speed = bcm53400_a0_port_speed_max(unit, port);
    
    /* GXMAC init */
    ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
    COMMAND_CONFIGr_SW_RESETf_SET(command_cfg, 1);
    ioerr += WRITE_COMMAND_CONFIGr(unit, port, command_cfg);

    ioerr += READ_COMMAND_CONFIGr(unit, port, &command_cfg);
    COMMAND_CONFIGr_RX_ENAf_SET(command_cfg, 0);
    COMMAND_CONFIGr_TX_ENAf_SET(command_cfg, 0);
    COMMAND_CONFIGr_ETH_SPEEDf_SET(command_cfg, speed);
    COMMAND_CONFIGr_PROMIS_ENf_SET(command_cfg, 1);
    COMMAND_CONFIGr_PAD_ENf_SET(command_cfg, 0);
    COMMAND_CONFIGr_CRC_FWDf_SET(command_cfg, 1);
    COMMAND_CONFIGr_PAUSE_FWDf_SET(command_cfg, 0);
    COMMAND_CONFIGr_PAUSE_IGNOREf_SET(command_cfg, 0);
    COMMAND_CONFIGr_TX_ADDR_INSf_SET(command_cfg, 0);
    COMMAND_CONFIGr_HD_ENAf_SET(command_cfg, 0);
    COMMAND_CONFIGr_LOOP_ENAf_SET(command_cfg, 0);
    COMMAND_CONFIGr_CNTL_FRM_ENAf_SET(command_cfg, 0);
    COMMAND_CONFIGr_NO_LGTH_CHECKf_SET(command_cfg, 1);
    COMMAND_CONFIGr_LINE_LOOPBACKf_SET(command_cfg, 0);
    COMMAND_CONFIGr_RX_ERR_DISCf_SET(command_cfg, 0);
    COMMAND_CONFIGr_RX_ERR_DISCf_SET(command_cfg, 0);
    COMMAND_CONFIGr_CNTL_FRM_ENAf_SET(command_cfg, 1);
    COMMAND_CONFIGr_SW_OVERRIDE_RXf_SET(command_cfg, 1);
    COMMAND_CONFIGr_SW_OVERRIDE_TXf_SET(command_cfg, 1);
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

    return ioerr ? CDK_E_IO : rv;
}

int
bcm53400_a0_xlport_init(int unit, int port)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    XLPORT_MODE_REGr_t xlport_mode;
    XLPORT_SOFT_RESETr_t xlport_sreset;
    XLPORT_ENABLE_REGr_t xlport_enable;
    EGR_PORT_CREDIT_RESETm_t egr_port_credit_reset;
    PGW_XL_TXFIFO_CTRLr_t xlport_txfifo_ctrl;
    PGW_GX_TXFIFO_CTRLr_t gxport_txfifo_ctrl;
    XLMAC_CTRLr_t xlmac_ctrl;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
    XLMAC_TX_CTRLr_t xlmac_tx_ctrl;
    XLMAC_PAUSE_CTRLr_t xlmac_pause_ctrl;
    XLMAC_RX_MAX_SIZEr_t xlmac_rx_max_size;
    XLMAC_MODEr_t xlmac_mode;
    XLPORT_MAC_RSV_MASKr_t xlmac_mac_rsv_mask;
    XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
    XLMAC_PFC_CTRLr_t xlmac_pfc_ctrl;
    int mode, speed;
    int idx, pdx, lport, lanes;

    ioerr += _port_init(unit, port);

    speed = bcm53400_a0_port_speed_max(unit, port);
    
    /* We only need to configure XLPORT registers once per block */
    if (XLPORT_SUBPORT(unit, port) == 0) {
        XLPORT_SOFT_RESETr_CLR(xlport_sreset);
        for (idx = 0; idx <= 3; idx++) {
            lport = P2L(unit, (port + idx));
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

        /* Set XPORT mode to 10G by default */
        lanes = bcm53400_a0_port_num_lanes(unit, port);
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

        /* De-assert XLPORT soft reset */
        XLPORT_SOFT_RESETr_CLR(xlport_sreset);
        ioerr += WRITE_XLPORT_SOFT_RESETr(unit, xlport_sreset, port);

        /* Disable XLPORT */
        XLPORT_ENABLE_REGr_CLR(xlport_enable);
        ioerr += WRITE_XLPORT_ENABLE_REGr(unit, xlport_enable, port);

        for (pdx = port; pdx <= (port + 3); pdx++) {
            lport = P2L(unit, pdx);
            if (lport < 0) {
                continue;
            }
            /* CREDIT reset and MAC count clear */
            ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, pdx, &egr_port_credit_reset);
            EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
            ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, pdx, egr_port_credit_reset);

            if (bcm53400_a0_xlblock_number_get(unit, port) == 32) {
                ioerr += READ_PGW_GX_TXFIFO_CTRLr(unit, pdx, &gxport_txfifo_ctrl);
                PGW_GX_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(gxport_txfifo_ctrl, 1);
                PGW_GX_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(gxport_txfifo_ctrl, 1);
                ioerr += WRITE_PGW_GX_TXFIFO_CTRLr(unit, pdx, gxport_txfifo_ctrl);
            } else {
                ioerr += READ_PGW_XL_TXFIFO_CTRLr(unit, pdx, &xlport_txfifo_ctrl);
                PGW_XL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(xlport_txfifo_ctrl, 1);
                PGW_XL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(xlport_txfifo_ctrl, 1);
                ioerr += WRITE_PGW_XL_TXFIFO_CTRLr(unit, pdx, xlport_txfifo_ctrl);
            }                
            BMD_SYS_USLEEP(1000);
            
            ioerr += READ_EGR_PORT_CREDIT_RESETm(unit, pdx, &egr_port_credit_reset);
            EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
            ioerr += WRITE_EGR_PORT_CREDIT_RESETm(unit, pdx, egr_port_credit_reset);
            
            if (bcm53400_a0_xlblock_number_get(unit, port) == 32) {
                ioerr += READ_PGW_GX_TXFIFO_CTRLr(unit, pdx, &gxport_txfifo_ctrl);
                PGW_GX_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(gxport_txfifo_ctrl, 0);
                PGW_GX_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(gxport_txfifo_ctrl, 0);
                ioerr += WRITE_PGW_GX_TXFIFO_CTRLr(unit, pdx, gxport_txfifo_ctrl);
            } else {
                ioerr += READ_PGW_XL_TXFIFO_CTRLr(unit, pdx, &xlport_txfifo_ctrl);
                PGW_XL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(xlport_txfifo_ctrl, 0);
                PGW_XL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(xlport_txfifo_ctrl, 0);
                ioerr += WRITE_PGW_XL_TXFIFO_CTRLr(unit, pdx, xlport_txfifo_ctrl);
            }                
        }
        
        /* Enable XLPORT */
        for (idx = 0; idx <= 3; idx++) {
            lport = P2L(unit, (port + idx));
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
    if (speed >= 10000) {
        XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 1);
    }
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
    if (speed == 1000) {
        XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, 2);
    } else if (speed == 2500) {
        XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, 3);
    }
    ioerr += WRITE_XLMAC_MODEr(unit, port, xlmac_mode);

    ioerr += READ_XLPORT_MAC_RSV_MASKr(unit, port, &xlmac_mac_rsv_mask);
    XLPORT_MAC_RSV_MASKr_SET(xlmac_mac_rsv_mask, 0x58);
    ioerr += WRITE_XLPORT_MAC_RSV_MASKr(unit, port, xlmac_mac_rsv_mask);
    
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

    return ioerr ? CDK_E_IO : rv;
}

int
bcm53400_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    ING_HW_RESET_CONTROL_1r_t ing_rst_ctl_1;
    ING_HW_RESET_CONTROL_2r_t ing_rst_ctl_2;
    EGR_HW_RESET_CONTROL_0r_t egr_rst_ctl_0;
    EGR_HW_RESET_CONTROL_1r_t egr_rst_ctl_1;
    MISCCONFIGr_t misc_cfg;
    CMIC_RATE_ADJUSTr_t rate_adjust;
    CMIC_RATE_ADJUST_INT_MDIOr_t rate_adjust_int_mdio;
    CMIC_MIIM_CONFIGr_t miim_cfg;
    RDBGC0_SELECTr_t rdbgc0_select;
    VLAN_PROFILE_TABm_t vlan_profile;
    ING_VLAN_TAG_ACTION_PROFILEm_t vlan_action;
    EGR_VLAN_TAG_ACTION_PROFILEm_t egr_action;
    GPORT_RSV_MASKr_t gport_rsv_mask;
    GPORT_CONFIGr_t gport_cfg;
    XLPORT_MIB_RESETr_t mib_rst;
    ING_PHYS_TO_LOGIC_MAPm_t ing_p2l;
    EGR_LOGIC_TO_PHYS_MAPr_t egr_l2p;
    MMU_LOGIC_TO_PHYS_MAPr_t mmu_l2p;
    PGW_CTRL_0r_t pgw_ctrl;
    CHIP_CONFIGr_t chip_cfg;
    CHIP_SWRSTr_t chip_swrst;
    cdk_pbmp_t pbmp, gpbmp, xpbmp;
    int lport, port, lanes;
    int idx, pmq_disabled;
    int num_lport, num_port;
    int freq, target_freq, divisor, dividend, delay;
    int speed_max, ability = 0;

#if BMD_CONFIG_INCLUDE_DMA
    struct phy_dma_buf_s *dbuf = &phy_dma_buf[unit];
#endif /* BMD_CONFIG_INCLUDE_DMA */

    BMD_CHECK_UNIT(unit);

    /* Get the port mapping */
    bcm53400_a0_xport_pbmp_get(unit, XPORT_FLAG_GXPORT, &gpbmp);
    bcm53400_a0_xport_pbmp_get(unit, XPORT_FLAG_XLPORT, &xpbmp);
    bcm53400_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &pbmp);

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
    ioerr += WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_rst_ctl_0);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_rst_ctl_1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_rst_ctl_1, 1);
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_rst_ctl_1, 0x1000);
    ioerr += WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_rst_ctl_1);

    for (idx = 0; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_ING_HW_RESET_CONTROL_2r(unit, &ing_rst_ctl_2);
        if (ING_HW_RESET_CONTROL_2r_DONEf_GET(ing_rst_ctl_2)) {
            break;
        }
        BMD_SYS_USLEEP(10000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm53400_a0_bmd_init[%d]: IPIPE reset timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }
        
    for (; idx < PIPE_RESET_TIMEOUT_MSEC; idx++) {
        ioerr += READ_EGR_HW_RESET_CONTROL_1r(unit, &egr_rst_ctl_1);
        if (EGR_HW_RESET_CONTROL_1r_DONEf_GET(egr_rst_ctl_1)) {
            break;
        }
        BMD_SYS_USLEEP(10000);
    }
    if (idx >= PIPE_RESET_TIMEOUT_MSEC) {
        CDK_WARN(("bcm53400_a0_bmd_init[%d]: EPIPE reset timeout\n", unit));
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

    freq = 176;
    if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ300) {
        freq = 300;
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
    delay = 0;

    CMIC_RATE_ADJUST_INT_MDIOr_CLR(rate_adjust_int_mdio);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVISORf_SET(rate_adjust_int_mdio, divisor);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVIDENDf_SET(rate_adjust_int_mdio, dividend);
    ioerr += WRITE_CMIC_RATE_ADJUST_INT_MDIOr(unit, rate_adjust_int_mdio);

    if (delay >= 1  && delay <= 15) {
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
    if (CDK_FAILURE(rv)) {
        return rv;
    }

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

    /* Reset XLPORT MIB counter */
    XLPORT_MIB_RESETr_SET(mib_rst, 0xf);
    ioerr += WRITE_XLPORT_MIB_RESETr(unit, mib_rst, -1);
    XLPORT_MIB_RESETr_SET(mib_rst, 0x0);
    ioerr += WRITE_XLPORT_MIB_RESETr(unit, mib_rst, -1);

#if BMD_CONFIG_INCLUDE_DMA
    /* We need to initialize DMA before downloading PHY firmware. */
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

    CDK_MEMSET(dbuf, 0, sizeof(*dbuf));
#endif

    /* Set QMODE enable for TSC0Q at QSGMII mode before unimac init process */
    ioerr += READ_PGW_CTRL_0r(unit, &pgw_ctrl);
    pmq_disabled = PGW_CTRL_0r_SW_PM4X10Q_DISABLEf_GET(pgw_ctrl);
    /* Ensure the TSC0Q is enabled and QSGMII port is exist */
    if ((pmq_disabled == 0) && CDK_PBMP_NOT_NULL(gpbmp)) {
        /* Fix-up packet purge filtering */
        GPORT_RSV_MASKr_SET(gport_rsv_mask, 0x70);
        ioerr += WRITE_GPORT_RSV_MASKr(unit, gport_rsv_mask, -1);
    
        /* Enable GPORTs and clear counters */
        ioerr += READ_GPORT_CONFIGr(unit, &gport_cfg, -1);
        GPORT_CONFIGr_CLR_CNTf_SET(gport_cfg, 1);
        GPORT_CONFIGr_GPORT_ENf_SET(gport_cfg, 1);
        ioerr += WRITE_GPORT_CONFIGr(unit, gport_cfg, -1);
        GPORT_CONFIGr_CLR_CNTf_SET(gport_cfg, 0);
        ioerr += WRITE_GPORT_CONFIGr(unit, gport_cfg, -1);

        ioerr += READ_CHIP_CONFIGr(unit, &chip_cfg);
        CHIP_CONFIGr_QMODEf_SET(chip_cfg, 1);
        ioerr += WRITE_CHIP_CONFIGr(unit, chip_cfg);
    }
    
    /* Configure GXPORTs */
    CDK_PBMP_ITER(gpbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv += _gxport_init(unit, port);
        }
    }

    /* Configure XLPORTs */
    CDK_PBMP_ITER(xpbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv += bcm53400_a0_xlport_init(unit, port);
        }
    }

    CDK_PBMP_ITER(pbmp, port) {
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_probe(unit, port);
        }
        
        lanes = bcm53400_a0_port_num_lanes(unit, port);
        speed_max = bcm53400_a0_port_speed_max(unit, port);

        if (CDK_SUCCESS(rv)) {
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

        /* Set the phy default ability to be cached locally,
         * it would then be set in bmd_phy_int function.
         */
        if (CDK_PBMP_MEMBER(xpbmp, port)) {
            ability = (BMD_PHY_ABIL_1000MB_FD | BMD_PHY_ABIL_100MB_FD);
            if (speed_max == 1000) {
                ability |= BMD_PHY_ABIL_10MB_FD;
           } else if (speed_max == 2500) {
                ability |= BMD_PHY_ABIL_2500MB;
           } else if (speed_max == 10000) {
                ability |= (BMD_PHY_ABIL_10GB | BMD_PHY_ABIL_2500MB);
           } else if (speed_max == 13000) {
                ability |= BMD_PHY_ABIL_13GB;
           }
           if (CDK_SUCCESS(rv)) {
               rv = bmd_phy_default_ability_set(unit, port, ability);
           }
        }

#if BMD_CONFIG_INCLUDE_PHY == 1
        if (CDK_SUCCESS(rv)) {
            phy_ctrl_t *pc;

            if (speed_max == 2500) {
                pc = BMD_PORT_PHY_CTRL(unit, port);
                /* Run port at 6.25VCO */
                PHY_CTRL_FLAGS(pc) |= PHY_F_6250_VCO;
            }
        }
#endif /* BMD_CONFIG_INCLUDE_PHY == 1 */
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

    /* Release QSGMII reset state after QSGMII-PCS and unimac init */
    if ((pmq_disabled == 0) && CDK_PBMP_NOT_NULL(gpbmp)) {
        ioerr += READ_CHIP_SWRSTr(unit, &chip_swrst);
        CHIP_SWRSTr_ILKN_BYPASS_RSTNf_SET(chip_swrst, 0xf);
        CHIP_SWRSTr_FLUSHf_SET(chip_swrst, 0);
        CHIP_SWRSTr_SOFT_RESET_QSGMII_PCSf_SET(chip_swrst, 0);
        CHIP_SWRSTr_SOFT_RESET_GPORT1f_SET(chip_swrst, 0);
        CHIP_SWRSTr_SOFT_RESET_GPORT0f_SET(chip_swrst, 0);
        ioerr += WRITE_CHIP_SWRSTr(unit, chip_swrst);
    }

    /* Common port initialization for CPU port */
    ioerr += _port_init(unit, CMIC_PORT);

#if BMD_CONFIG_INCLUDE_DMA
    /* Free SBUSDMA buffer used for PHY firmware download */
    if (dbuf->laddr) {
        bmd_dma_free_coherent(unit, dbuf->size, dbuf->laddr, dbuf->paddr);
        dbuf->size = 0;
        dbuf->laddr = NULL;
    }
#endif /* BMD_CONFIG_INCLUDE_DMA */

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53400_A0 */
