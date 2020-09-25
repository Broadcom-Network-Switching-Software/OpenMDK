#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53125_A0 == 1

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

#include <cdk/chip/bcm53125_a0_defs.h>

#include "bcm53125_a0_internal.h"
#include "bcm53125_a0_bmd.h"

int
bcm53125_a0_bmd_tx(int unit, const bmd_pkt_t *pkt)
{
#if BMD_CONFIG_INCLUDE_DMA == 1
    int rv;
    int txsize;
    uint8_t *txbuf, *brcm_hdr;
    dma_addr_t baddr;
    IMP_CTLr_t impstl;
    int temp;

    BMD_CHECK_UNIT(unit);

    if (pkt->port > 5 && pkt->port != 8) {
        return CDK_E_PORT;
    }

    /* Before tx, re-check IMP setting for 8051 always change to disable RX */
    bcm53125_a0_int_cpu_lock(unit, 1);

    READ_IMP_CTLr(unit, &impstl);
    temp = IMP_CTLr_RX_UCST_ENf_GET(impstl);
    if (temp == 0) {
        IMP_CTLr_RX_UCST_ENf_SET(impstl, 1);
        IMP_CTLr_RX_MCST_ENf_SET(impstl, 1);
        IMP_CTLr_RX_BCST_ENf_SET(impstl, 1);
        IMP_CTLr_RX_DISf_SET(impstl, 0);
        IMP_CTLr_TX_DISf_SET(impstl, 0);
    }
    WRITE_IMP_CTLr(unit, impstl);

    /* Allocate Tx buffer */
    txsize = pkt->size + ROBO_BRCM_HDR_SIZE;
    txbuf = bmd_dma_alloc_coherent(unit, txsize, &baddr);
    if (txbuf == NULL) {
        return CDK_E_MEMORY;
    }

    /* Copy MAC addresses to Tx buffer */
    CDK_MEMCPY(txbuf, pkt->data, 12);

    /* Initialize Broadcom header */
    brcm_hdr = &txbuf[12];
    CDK_MEMSET(brcm_hdr, 0, ROBO_BRCM_HDR_SIZE);

    /* Create Broadcom tag */
    if (pkt->port >= 0) {
        brcm_hdr[0] = 0x20;
        if (pkt->port == CPIC_PORT) {
            brcm_hdr[2] = 0x01;
        } else {
            if (pkt->flags & BMD_PKT_F_UNTAGGED) {
                brcm_hdr[0] |= 0x01;
            }
            brcm_hdr[3] = 1 << pkt->port;
        }
    }

    /* Copy remainder of packet Tx buffer */
    CDK_MEMCPY(&txbuf[12 + ROBO_BRCM_HDR_SIZE], &pkt->data[12], pkt->size - 12);

    CDK_VVERB(("Tx BRCM header = %02x%02x %02x%02x\n",
               brcm_hdr[0], brcm_hdr[1], brcm_hdr[2], 
               brcm_hdr[3]));

    /* Pass buffer to Ethernet driver */
    /* In ROBO packet size include header size, but while tx packet, the header would be removed */
    rv = cdk_dev_write(unit, CDK_DEV_ADDR_ETH, txbuf, txsize - 4);

    bcm53125_a0_int_cpu_lock(unit, 0);
    /* Free Tx buffer */
    bmd_dma_free_coherent(unit, txsize, txbuf, baddr);
    return rv;
#else
    return CDK_E_UNAVAIL;
#endif
}
#endif /* CDK_CONFIG_INCLUDE_BCM53125_A0 */
