#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56224_A0 == 1

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

#include <cdk/chip/bcm56224_a0_defs.h>

#include "bcm56224_a0_bmd.h"

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
    EPC_LINK_BMAPr_t epc_link;
    uint32_t pbmp;

    ioerr += READ_EPC_LINK_BMAPr(unit, &epc_link);
    pbmp = EPC_LINK_BMAPr_PORT_BITMAPf_GET(epc_link);
    if (enable) {
        pbmp |= LSHIFT32(1, CMIC_PORT);
    } else {
        pbmp &= ~LSHIFT32(1, CMIC_PORT);
    }
    EPC_LINK_BMAPr_PORT_BITMAPf_SET(epc_link, pbmp);
    ioerr += WRITE_EPC_LINK_BMAPr(unit, epc_link);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif

int
bcm56224_a0_bmd_rx_start(int unit, bmd_pkt_t *pkt)
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
bcm56224_a0_bmd_rx_poll(int unit, bmd_pkt_t **ppkt)
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
    bmd_xgs_dump_rx_dcb(unit, (uint32_t *)dcb,
                        CDK_BYTES2WORDS(RX_DCB_SIZE), CDK_HIGIG2_WSIZE);

    if (RX_DCB_DONEf_GET(*dcb) == 0) {
        return CDK_E_TIMEOUT;
    }

    /* Fill out packet structure */
    pkt = _rx_dscr[unit].pkt;
    pkt->size = RX_DCB_BYTES_TRANSFERREDf_GET(*dcb);
    pkt->port = RX_DCB_SRC_PORTf_GET(*dcb);

    bmd_xgs_parse_higig2(unit, pkt, RX_DCB_MODULE_HEADERf_PTR(*dcb));

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
bcm56224_a0_bmd_rx_stop(int unit)
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
#endif /* CDK_CONFIG_INCLUDE_BCM56224_A0 */
