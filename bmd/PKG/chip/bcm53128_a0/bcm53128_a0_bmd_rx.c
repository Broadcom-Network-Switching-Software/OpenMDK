#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53128_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_dma.h>

#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include <cdk/chip/bcm53128_a0_defs.h>

#include "bcm53128_a0_internal.h"
#include "bcm53128_a0_bmd.h"

#if BMD_CONFIG_INCLUDE_DMA == 1

typedef struct robo_rx_dscr_s {
    bmd_pkt_t *pkt;
} robo_rx_dscr_t;

static robo_rx_dscr_t _rx_dscr[BMD_CONFIG_MAX_UNITS];

static int
_cpu_port_enable_set(int unit, int enable)
{
    int ioerr = 0;
    IMP_CTLr_t mii_pctl;

    ioerr += READ_IMP_CTLr(unit, &mii_pctl);
    IMP_CTLr_RX_BCST_ENf_SET(mii_pctl, enable);
    IMP_CTLr_RX_MCST_ENf_SET(mii_pctl, enable);
    IMP_CTLr_RX_UCST_ENf_SET(mii_pctl, enable);
    ioerr += WRITE_IMP_CTLr(unit, mii_pctl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif

int
bcm53128_a0_bmd_rx_start(int unit, bmd_pkt_t *pkt)
{
#if BMD_CONFIG_INCLUDE_DMA == 1
    int rv;

    BMD_CHECK_UNIT(unit);

    /* Save packet pointer locally */
    _rx_dscr[unit].pkt = pkt;

    rv = _cpu_port_enable_set(unit, 1);

    return rv; 
#else
    return CDK_E_UNAVAIL;
#endif
}

int
bcm53128_a0_bmd_rx_poll(int unit, bmd_pkt_t **ppkt)
{
#if BMD_CONFIG_INCLUDE_DMA == 1
    int rv = CDK_E_NONE;
    bmd_pkt_t *pkt = _rx_dscr[unit].pkt;
    uint8_t *brcm_hdr;

    BMD_CHECK_UNIT(unit);

    if (pkt == NULL) {
        return CDK_E_DISABLED;
    }

    rv = cdk_dev_read(unit, CDK_DEV_ADDR_ETH, pkt->data, pkt->size);
    if (CDK_FAILURE(rv)) {
        return rv;
    }
    pkt->size = rv;

    brcm_hdr = &pkt->data[12];
    if (pkt->size > (12 + ROBO_BRCM_HDR_SIZE)) {

        CDK_VVERB(("Rx BRCM header = %02x%02x %02x%02x\n",
                   brcm_hdr[0], brcm_hdr[1], brcm_hdr[2], 
                   brcm_hdr[3]));

        /* Extract ingress port from BRCM tag */
        pkt->port = brcm_hdr[3] & 0x1f;

        /* Strip Broadcom header */
        pkt->size -= ROBO_BRCM_HDR_SIZE;
        
        
        CDK_MEMCPY(brcm_hdr, &brcm_hdr[ROBO_BRCM_HDR_SIZE], pkt->size - 12);
    }

    /* Pass packet back to application */
    *ppkt = pkt;

    CDK_MEMSET(&_rx_dscr[unit], 0, sizeof(_rx_dscr[unit]));

    return rv; 
#else
    return CDK_E_UNAVAIL;
#endif
}

int
bcm53128_a0_bmd_rx_stop(int unit)
{
#if BMD_CONFIG_INCLUDE_DMA == 1
    int rv;

    BMD_CHECK_UNIT(unit);

    if (_rx_dscr[unit].pkt == NULL) {
        return CDK_E_DISABLED;
    }

    rv = _cpu_port_enable_set(unit, 0);

    CDK_MEMSET(&_rx_dscr[unit], 0, sizeof(_rx_dscr[unit]));

    return rv; 
#else
    return CDK_E_UNAVAIL;
#endif
}
#endif /* CDK_CONFIG_INCLUDE_BCM53128_A0 */
