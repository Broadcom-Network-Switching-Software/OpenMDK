/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS DMA definitions.
 */

#ifndef __XGS_DMA_H__
#define __XGS_DMA_H__

#include <bmd/bmd.h>

#include <cdk/cdk_higig_defs.h>

/*
 * Default channel configuration
 */
#define XGS_DMA_TX_CHAN         0
#define XGS_DMA_RX_CHAN         1

/* Initialize DMA */
extern int bmd_xgs_dma_init(int unit); 

/* DMA TX */
extern int bmd_xgs_dma_tx_start(int unit, dma_addr_t dcb); 
extern int bmd_xgs_dma_tx_poll(int unit, int num_polls); 
extern int bmd_xgs_dma_tx_abort(int unit, int num_polls); 

/* DMA RX */
extern int bmd_xgs_dma_rx_start(int unit, dma_addr_t dcb); 
extern int bmd_xgs_dma_rx_poll(int unit, int num_polls); 
extern int bmd_xgs_dma_rx_abort(int unit, int num_polls); 


/*
 * Per-channel dma
 * Should not be called directly under normal circumstances
 */

#define XGS_DMA_CHAN_DIR_TX     1
#define XGS_DMA_CHAN_DIR_RX     0

extern int bmd_xgs_dma_chan_init(int unit, int chan, int dir); 
extern int bmd_xgs_dma_chan_start(int unit, int chan,  dma_addr_t dcb); 
extern int bmd_xgs_dma_chan_poll(int unit, int chan, int num_polls); 
extern int bmd_xgs_dma_chan_abort(int unit, int chan, int num_polls); 

/*
 * Utility functions for parsing Rx DMA descriptors
 */

#if BMD_CONFIG_INCLUDE_HIGIG == 1

extern int bmd_xgs_parse_higig(int unit, bmd_pkt_t *pkt, uint32_t *mh);
extern int bmd_xgs_parse_higig2(int unit, bmd_pkt_t *pkt, uint32_t *mh);

#else

#define bmd_xgs_parse_higig(_u, _pkt, _mh)
#define bmd_xgs_parse_higig2(_u, _pkt, _mh)

#endif

/*
 * Utility functions for debugging Rx DMA descriptors
 */

#if CDK_CONFIG_INCLUDE_DEBUG == 1

extern int bmd_xgs_dump_rx_dcb(int unit, uint32_t *dcb,
                               int dcb_size, int mh_size);
extern int bmd_xgs_dump_tx_dcbs(int unit, uint32_t *dcbs, int dcb_cnt,
                                int dcb_size, int mh_size);

#else

#define bmd_xgs_dump_rx_dcb(_u, _dcb, _dcb_size, _mh_size)
#define bmd_xgs_dump_tx_dcbs(_u, _dcbs, _dcb_cnt, _dcb_size, _mh_size)

#endif


/* Assume 1:1 mapping between HiGig opcode and BMD packet type */
#define BMD_PKT_TYPE_FROM_HIGIG(_x)     (_x)

#endif /* __XGS_DMA_H__ */
