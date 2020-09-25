#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56680_B0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_dma.h>

#include <bmdi/arch/xgs_dma.h>

#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include <cdk/chip/bcm56680_b0_defs.h>

#include "bcm56680_b0_bmd.h"

#if BMD_CONFIG_INCLUDE_DMA == 1

typedef struct xgs_rx_dscr_s {
    RX_DCB_t *dcb;      /* DMA Control Block */
    dma_addr_t bdcb;    /* RX_DCB bus address */
    bmd_pkt_t *pkt;     /* Packet associated with RX_DCB */
} xgs_rx_dscr_t;

static xgs_rx_dscr_t _rx_dscr[BMD_CONFIG_MAX_UNITS];

static int
_cpu_port_enable_set(int unit, int enable)
{
    int ioerr = 0;
    EPC_LINK_BMAP_64r_t epc_link;
    uint32_t epc_pbm;

    ioerr += READ_EPC_LINK_BMAP_64r(unit, &epc_link);
    CDK_ASSERT(CMIC_PORT < 32);
    epc_pbm = EPC_LINK_BMAP_64r_PORT_BITMAP_LOf_GET(epc_link);
    if (enable) {
        epc_pbm |= LSHIFT32(1, CMIC_PORT);
    } else {
        epc_pbm &= ~LSHIFT32(1, CMIC_PORT);
    }
    EPC_LINK_BMAP_64r_PORT_BITMAP_LOf_SET(epc_link, epc_pbm);
    ioerr += WRITE_EPC_LINK_BMAP_64r(unit, epc_link);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif

int
bcm56680_b0_bmd_rx_start(int unit, bmd_pkt_t *pkt)
{
#if BMD_CONFIG_INCLUDE_DMA == 1
    RX_DCB_t *dcb;
    dma_addr_t bdcb;
    int rv = CDK_E_NONE;

    BMD_CHECK_UNIT(unit);

    /* Check for valid physical bus address */
    CDK_ASSERT(pkt->baddr);

    if (_rx_dscr[unit].dcb != NULL) {
        return CDK_E_RESOURCE;
    }

    /* Allocate DMA descriptor from DMA memory pool */
    dcb = bmd_dma_alloc_coherent(unit, sizeof(*dcb), &bdcb);
    if (dcb == NULL) {
        return CDK_E_MEMORY;
    }

    _rx_dscr[unit].dcb = dcb;
    _rx_dscr[unit].bdcb = bdcb;
    _rx_dscr[unit].pkt = pkt;

    /* Set up DMA descriptor */
    RX_DCB_CLR(*dcb); 
    RX_DCB_ADDRf_SET(*dcb, pkt->baddr); 
    RX_DCB_BYTE_COUNTf_SET(*dcb, pkt->size); 

    /* Start DMA */
    BMD_DMA_CACHE_FLUSH(dcb, sizeof(*dcb));
    bmd_xgs_dma_rx_start(unit, bdcb); 

    rv = _cpu_port_enable_set(unit, 1);

    return rv; 
#else
    return CDK_E_UNAVAIL;
#endif
}

int
bcm56680_b0_bmd_rx_poll(int unit, bmd_pkt_t **ppkt)
{
#if BMD_CONFIG_INCLUDE_DMA == 1
    RX_DCB_t *dcb;
    bmd_pkt_t *pkt;
    int rv = CDK_E_NONE;

    BMD_CHECK_UNIT(unit);

    dcb = _rx_dscr[unit].dcb;
    if (dcb == NULL) {
        return CDK_E_DISABLED;
    }

    /* Poll for DMA completion */
    if (bmd_xgs_dma_rx_poll(unit, 1) < 0) {
        return CDK_E_TIMEOUT;
    }
    BMD_DMA_CACHE_INVAL(dcb, sizeof(*dcb));
    CDK_VVERB(("Rx DMA ctrl = %08"PRIx32"\n", RX_DCB_GET(*dcb, 1)));
    CDK_VVERB(("Rx DMA stat = %08"PRIx32"\n", RX_DCB_GET(*dcb, 10)));
    CDK_VVERB(("Rx DMA mhdr = %08"PRIx32" %08"PRIx32" %08"PRIx32" %08"PRIx32"\n", 
               RX_DCB_GET(*dcb, 2), RX_DCB_GET(*dcb, 3), 
               RX_DCB_GET(*dcb, 4), RX_DCB_GET(*dcb, 5)));
    CDK_VVERB(("Rx pkt stat = %08"PRIx32" %08"PRIx32" %08"PRIx32" %08"PRIx32" " 
               "%08"PRIx32" %08"PRIx32" %08"PRIx32" %08"PRIx32" %08"PRIx32"\n", 
               RX_DCB_GET(*dcb, 6), RX_DCB_GET(*dcb, 7), RX_DCB_GET(*dcb, 8), 
               RX_DCB_GET(*dcb, 9), RX_DCB_GET(*dcb, 10), RX_DCB_GET(*dcb, 11), 
               RX_DCB_GET(*dcb, 12), RX_DCB_GET(*dcb, 13), RX_DCB_GET(*dcb, 14)));

    if (RX_DCB_DONEf_GET(*dcb) == 0) {
        return CDK_E_TIMEOUT;
    }

    /* Fill out packet structure */
    pkt = _rx_dscr[unit].pkt;
    pkt->size = RX_DCB_BYTES_TRANSFERREDf_GET(*dcb);
    pkt->port = RX_DCB_SRC_PORTf_GET(*dcb);

    /* Pass packet back to application */
    *ppkt = pkt;

    /* Free DMA descriptor */
    bmd_dma_free_coherent(unit, sizeof(*dcb), dcb, _rx_dscr[unit].bdcb);

    CDK_MEMSET(&_rx_dscr[unit], 0, sizeof(_rx_dscr[unit]));

    return rv; 
#else
    return CDK_E_UNAVAIL;
#endif
}

int
bcm56680_b0_bmd_rx_stop(int unit)
{
#if BMD_CONFIG_INCLUDE_DMA == 1
    RX_DCB_t *dcb;
    int rv = CDK_E_NONE;

    BMD_CHECK_UNIT(unit);

    dcb = _rx_dscr[unit].dcb;
    if (dcb == NULL) {
        return CDK_E_DISABLED;
    }

    rv = _cpu_port_enable_set(unit, 0);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    rv = bmd_xgs_dma_rx_abort(unit, BMD_CONFIG_DMA_MAX_POLLS);
    if (CDK_FAILURE(rv)) {
        return rv;
    }

    /* Free DMA descriptor */
    bmd_dma_free_coherent(unit, sizeof(*dcb), dcb, _rx_dscr[unit].bdcb);

    CDK_MEMSET(&_rx_dscr[unit], 0, sizeof(_rx_dscr[unit]));

    return rv; 
#else
    return CDK_E_UNAVAIL;
#endif
}
#endif /* CDK_CONFIG_INCLUDE_BCM56680_B0 */
