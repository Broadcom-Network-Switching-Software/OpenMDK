#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53280_A0 == 1

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

#include <cdk/chip/bcm53280_a0_defs.h>

#include "bcm53280_a0_internal.h"
#include "bcm53280_a0_bmd.h"

int
bcm53280_a0_bmd_tx(int unit, const bmd_pkt_t *pkt)
{
#if BMD_CONFIG_INCLUDE_DMA == 1
    int rv;
    int txsize;
    uint8_t *txbuf, *brcm_hdr;
    uint8_t temp_vlan[2];
    
    dma_addr_t baddr;

    BMD_CHECK_UNIT(unit);

    if (pkt->port > 28) {
        return CDK_E_PORT;
    }

    /* Allocate Tx buffer */
    /* txsize - 4 due to TB will remove vlan tag manually */
    /* There is no untag opcode */
    txsize = pkt->size + ROBO_BRCM_HDR_SIZE;
    txbuf = bmd_dma_alloc_coherent(unit, txsize - 4, &baddr);
    if (txbuf == NULL) {
        return CDK_E_MEMORY;
    }

    /* Initialize Broadcom header */
    brcm_hdr = &txbuf[0];
    CDK_MEMSET(brcm_hdr, 0, ROBO_BRCM_HDR_SIZE);

    /* Create Broadcom tag */
    if (pkt->port >= 0) {
        /* opcode = 0001, TC = 0000 */
        brcm_hdr[0] = 0x10;
        /* DP = 00, Filter_ByPass[7:2] = 111111 */
        brcm_hdr[1] = 0x3f;
        /* Filter_ByPass[1:0] = 11, R = 0, DST_ID[12:11] = 00, DST_ID[10:8] = 000 */
        brcm_hdr[2] = 0xc0;
        /* DST_ID[7:5] = 000, DST_ID[4:0] = port_id */
        brcm_hdr[3] = pkt->port;
        /* R = 00000000 */
        brcm_hdr[4] = 0;
        /* VLAN_ID[11:0], Flow_ID[11:8] = 0 */
        if (pkt->flags & BMD_PKT_F_UNTAGGED) {
            /* set vid to default vlan 1 */
            brcm_hdr[5] = 0;
            brcm_hdr[6] = 0x10;
        } else {
            temp_vlan[0] = pkt->data[14];
            temp_vlan[1] = pkt->data[15];
            brcm_hdr[5] = ((temp_vlan[0] << 4) & 0xf0) | ((temp_vlan[1] >> 4) & 0x0f);
            brcm_hdr[6] = (temp_vlan[1] << 4) & 0xf0;
        }
        /* Flow_ID[7:0] = 0 */
        brcm_hdr[7] = 0;
    }
    CDK_VVERB(("Tx BRCM header = %02x%02x %02x%02x %02x%02x %02x%02x\n",
               brcm_hdr[0], brcm_hdr[1], brcm_hdr[2], brcm_hdr[3],  
               brcm_hdr[4], brcm_hdr[5], brcm_hdr[6], brcm_hdr[7]));

    /* Copy packet to Tx buffer, starting address is following brcm header */
    CDK_MEMCPY(&txbuf[ROBO_BRCM_HDR_SIZE], pkt->data, 12);

    /* Copy remainder of packet Tx buffer */
    /* TB need to manual remove vlan-tag from packet */
    CDK_MEMCPY(&txbuf[12 + ROBO_BRCM_HDR_SIZE], &pkt->data[16], pkt->size - 16);

    /* Pass buffer to Ethernet driver */
    rv = cdk_dev_write(unit, CDK_DEV_ADDR_ETH, txbuf, txsize - 8);
    /* Free Tx buffer */
    bmd_dma_free_coherent(unit, txsize, txbuf, baddr);
    return rv;
#else
    return CDK_E_UNAVAIL;
#endif
}
#endif /* CDK_CONFIG_INCLUDE_BCM53280_A0 */

