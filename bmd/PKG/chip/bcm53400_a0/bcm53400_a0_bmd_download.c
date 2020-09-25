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
#include <bmdi/arch/xgsd_led.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm53400_a0_defs.h>

#include "bcm53400_a0_bmd.h"
#include "bcm53400_a0_internal.h"

static int
_led_init(int unit)
{
    int ioerr = 0;
    CMIC_LEDUP0_PORT_ORDER_REMAPr_t led_remap;
    CMIC_LEDUP0_CTRLr_t led_ctrl;
    XLPORT_LED_CHAIN_CONFIGr_t led_chain_cfg;
    cdk_pbmp_t pbmp;
    int idx, pval[4];

    /*
     * Initialize the LED remap register settings. Port 0 entry is
     * skipped to allow easy indexing of physical port starting at 1.
     */ 
    for (idx = 0; idx < 8; idx++) {
        pval[0] = (idx * 4) + 2;
        pval[1] = pval[0] + 1;
        pval[2] = pval[0] + 2;
        pval[3] = pval[0] + 3;
        if ((pval[0] == 26) || (pval[0] == 30)) {
            /* Transpose XLPORT block ports */
            pval[3] = pval[0];
            pval[2] = pval[3] + 1;
            pval[1] = pval[3] + 2;
            pval[0] = pval[3] + 3;
        }
        ioerr += READ_CMIC_LEDUP0_PORT_ORDER_REMAPr(unit, idx, &led_remap);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_0f_SET(led_remap, pval[0]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_1f_SET(led_remap, pval[1]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_2f_SET(led_remap, pval[2]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_3f_SET(led_remap, pval[3]);
        ioerr += WRITE_CMIC_LEDUP0_PORT_ORDER_REMAPr(unit, idx, led_remap);
    }

    bcm53400_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &pbmp);
    XLPORT_LED_CHAIN_CONFIGr_CLR(led_chain_cfg); 
    XLPORT_LED_CHAIN_CONFIGr_INTRA_DELAYf_SET(led_chain_cfg, 6); 
    ioerr += WRITE_XLPORT_LED_CHAIN_CONFIGr(unit, led_chain_cfg, -1);

    ioerr += READ_CMIC_LEDUP0_CTRLr(unit, &led_ctrl);
    CMIC_LEDUP0_CTRLr_LEDUP_SCAN_START_DELAYf_SET(led_ctrl, 5);
    ioerr += WRITE_CMIC_LEDUP0_CTRLr(unit, led_ctrl);

    return ioerr;
}

int 
bcm53400_a0_bmd_download(int unit, bmd_download_t type, uint8_t *data, int size)
{
    int rv = CDK_E_UNAVAIL;
    int ioerr = 0;

    BMD_CHECK_UNIT(unit);

    switch (type) {
    case bmdDownloadPortLedController:
        ioerr += _led_init(unit);
        if (ioerr) {
            return CDK_E_IO;
        }
        rv = xgsd_led_prog(unit, data, size);
        break;
    default:
        break;
    }

    return rv; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM53400_A0 */
