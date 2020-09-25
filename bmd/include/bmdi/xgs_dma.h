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

#endif /* __XGS_DMA_H__ */
