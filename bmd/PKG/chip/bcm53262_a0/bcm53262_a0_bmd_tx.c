#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53262_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_dma.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_util.h>

#include <cdk/chip/bcm53262_a0_defs.h>

#include "bcm53262_a0_internal.h"
#include "bcm53262_a0_bmd.h"

int
bcm53262_a0_bmd_tx(int unit, const bmd_pkt_t *pkt)
{
#if BMD_CONFIG_INCLUDE_DMA == 1
    int rv;
    int txsize;
    uint8_t *txbuf, *crcbuf, *brcm_hdr;
    dma_addr_t baddr;
    uint32_t crc;

    BMD_CHECK_UNIT(unit);

    if (pkt->port > 28) {
        return CDK_E_PORT;
    }

    /* Allocate Tx buffer */
    txsize = pkt->size + ROBO_BRCM_HDR_SIZE;
    txbuf = bmd_dma_alloc_coherent(unit, txsize + 4, &baddr);
    if (txbuf == NULL) {
        return CDK_E_MEMORY;
    }

    /* Copy MAC addresses to Tx buffer */
    CDK_MEMCPY(txbuf, pkt->data, 12);

    /* Initialize Broadcom header */
    brcm_hdr = &txbuf[12];
    CDK_MEMSET(brcm_hdr, 0, ROBO_BRCM_HDR_SIZE);

    /* Add Broadcom ROBO type ID */
    brcm_hdr[0] = (ROBO_DEFAULT_BRCMID >> 8);
    brcm_hdr[1] = (ROBO_DEFAULT_BRCMID & 0xff);

    /* Create Broadcom tag */
    if (pkt->port >= 0) {
        brcm_hdr[2] = 0x40;
        if (pkt->flags & BMD_PKT_F_UNTAGGED) {
            brcm_hdr[2] |= 0x02;
        }
        brcm_hdr[5] = pkt->port+24;
    }

    /* Copy remainder of packet Tx buffer */
    CDK_MEMCPY(&txbuf[12 + ROBO_BRCM_HDR_SIZE], &pkt->data[12], pkt->size - 12);

    /* Add inner CRC based on original packet */
    crc = ~cdk_util_crc32(~0, pkt->data, pkt->size - 4);
    crcbuf = &txbuf[txsize -4];
    *crcbuf++ = (uint8_t)(crc >> 24);
    *crcbuf++ = (uint8_t)(crc >> 16);
    *crcbuf++ = (uint8_t)(crc >> 8);
    *crcbuf++ = (uint8_t)(crc);

    CDK_VVERB(("Tx inner CRC   = %08"PRIx32"\n", crc));
    CDK_VVERB(("Tx BRCM header = %02x%02x %02x%02x%02x%02x\n",
               brcm_hdr[0], brcm_hdr[1], brcm_hdr[2], 
               brcm_hdr[3], brcm_hdr[4], brcm_hdr[5]));

    /* Pass buffer to Ethernet driver */
    rv = cdk_dev_write(unit, CDK_DEV_ADDR_ETH, txbuf, txsize);

    /* Free Tx buffer */
    bmd_dma_free_coherent(unit, txsize, txbuf, baddr);

    return rv;
#else
    return CDK_E_UNAVAIL;
#endif
}
#endif /* CDK_CONFIG_INCLUDE_BCM53262_A0 */
