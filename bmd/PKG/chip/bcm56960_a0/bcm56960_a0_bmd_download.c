/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56960_A0 == 1

#include <bmd/bmd.h>

#include <bmdi/arch/xgsd_led.h>

#include <cdk/chip/bcm56960_a0_defs.h>

#include "bcm56960_a0_bmd.h"

static int
_led_remap(int unit)
{
    int ioerr = 0;
    int idx, regidx, pval;
    CMIC_LEDUP0_PORT_ORDER_REMAPr_t remap0;
    CMIC_LEDUP1_PORT_ORDER_REMAPr_t remap1;
    CMIC_LEDUP2_PORT_ORDER_REMAPr_t remap2;

    /* Configure LED-0 remap register settings.
     * LED-0 chain provides port status for tile0 (ports 1-32) 
     * and tile 3 (ports 97-128).
     * Scan chain order: 32, 31, ...., 1, 128, 127, ...., 97
     * LED port status for port 32 arrives first at the CMIC block and 
     * port 97's LED status arrives last.
     */
    CMIC_LEDUP0_PORT_ORDER_REMAPr_CLR(remap0);
    for (idx = 0; idx < 16; idx++) {
        if (idx == 0) {
            regidx = 7;
        } else if (idx == 8) {
            regidx = 15;
        }
        pval = idx * 4;
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_3f_SET(remap0, pval);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_2f_SET(remap0, pval+1);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_1f_SET(remap0, pval+2);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_0f_SET(remap0, pval+3);
        ioerr += WRITE_CMIC_LEDUP0_PORT_ORDER_REMAPr(unit, regidx, remap0);
        regidx --;
    }

    /* Configure LED-1 remap register settings.
     * LED-1 chain provides port status for tile1 (ports 33-64) 
     * and tile 2 (ports 65-96).
     * Scan chain order: 33, 34, 35, ...., 96
     * LED port status for port 33 arrives first at the CMIC block and 
     * port 96's LED status arrives last.
     */
    CMIC_LEDUP1_PORT_ORDER_REMAPr_CLR(remap1);
    for (idx = 0; idx < 16; idx++) {
        pval = idx * 4;
        CMIC_LEDUP1_PORT_ORDER_REMAPr_REMAP_PORT_3f_SET(remap1, pval);
        CMIC_LEDUP1_PORT_ORDER_REMAPr_REMAP_PORT_2f_SET(remap1, pval+1);
        CMIC_LEDUP1_PORT_ORDER_REMAPr_REMAP_PORT_1f_SET(remap1, pval+2);
        CMIC_LEDUP1_PORT_ORDER_REMAPr_REMAP_PORT_0f_SET(remap1, pval+3);
        ioerr += WRITE_CMIC_LEDUP1_PORT_ORDER_REMAPr(unit, idx, remap1);
    }

    /* Configure LED-2 remap register settings. 
     * LED-2 chain provides port status for management ports (129-132).
     * Scan chain port order is - 132 (unused), 131, 130 (unused), 129.
     * Unused remapping port registers are programmed to a larger value.
     */
    CMIC_LEDUP2_PORT_ORDER_REMAPr_CLR(remap2);
    for (idx = 0; idx < 16; idx++) {
        CMIC_LEDUP2_PORT_ORDER_REMAPr_REMAP_PORT_0f_SET(remap2, 0x3f);
        CMIC_LEDUP2_PORT_ORDER_REMAPr_REMAP_PORT_1f_SET(remap2, 0x3f);
        CMIC_LEDUP2_PORT_ORDER_REMAPr_REMAP_PORT_2f_SET(remap2, 0x3f);
        CMIC_LEDUP2_PORT_ORDER_REMAPr_REMAP_PORT_3f_SET(remap2, 0x3f);
        if (idx == 0) {
            CMIC_LEDUP2_PORT_ORDER_REMAPr_REMAP_PORT_1f_SET(remap2, 1);
            CMIC_LEDUP2_PORT_ORDER_REMAPr_REMAP_PORT_3f_SET(remap2, 0);
        }
        ioerr += WRITE_CMIC_LEDUP2_PORT_ORDER_REMAPr(unit, idx, remap2);        
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

int
bcm56960_a0_bmd_download(int unit, bmd_download_t type, uint8_t *data, int size)
{
    int rv = CDK_E_NONE;
    int ioerr = 0;
    int offset;
    CMIC_LEDUP0_CTRLr_t led_ctrl0;
    CMIC_LEDUP1_CTRLr_t led_ctrl1;
    CMIC_LEDUP2_CTRLr_t led_ctrl2;
    CMIC_LEDUP1_PROGRAM_RAMr_t led_prog1;
    CMIC_LEDUP2_PROGRAM_RAMr_t led_prog2;
    CMIC_LEDUP1_DATA_RAMr_t led_data1;
    CMIC_LEDUP2_DATA_RAMr_t led_data2;

    BMD_CHECK_UNIT(unit);

    switch (type) {
    case bmdDownloadPortLedController:
        /* Stop LED processor #0, #1, #2 */
        ioerr += READ_CMIC_LEDUP0_CTRLr(unit, &led_ctrl0);
        CMIC_LEDUP1_CTRLr_LEDUP_ENf_SET(led_ctrl1, 0);
        ioerr += READ_CMIC_LEDUP1_CTRLr(unit, &led_ctrl1);
        CMIC_LEDUP1_CTRLr_LEDUP_ENf_SET(led_ctrl1, 0);
        ioerr += READ_CMIC_LEDUP2_CTRLr(unit, &led_ctrl2);
        CMIC_LEDUP2_CTRLr_LEDUP_ENf_SET(led_ctrl2, 0);

        rv = _led_remap(unit);
        if (CDK_FAILURE(rv)) {
            return rv;
        } 
        /* Load program */
        for (offset = 0; offset < CMIC_LED_PROGRAM_RAM_SIZE; offset++) {
            CMIC_LEDUP1_PROGRAM_RAMr_SET(led_prog1, 0);
            CMIC_LEDUP2_PROGRAM_RAMr_SET(led_prog2, 0);            
            if (offset < size) {
                CMIC_LEDUP1_PROGRAM_RAMr_SET(led_prog1, data[offset]);
                CMIC_LEDUP2_PROGRAM_RAMr_SET(led_prog2, data[offset]);
            }
            ioerr += WRITE_CMIC_LEDUP1_PROGRAM_RAMr(unit, offset, led_prog1);
            ioerr += WRITE_CMIC_LEDUP2_PROGRAM_RAMr(unit, offset, led_prog2);
        }
        /* The LED data area should be clear whenever program starts */
        CMIC_LEDUP1_DATA_RAMr_SET(led_data1, 0);
        CMIC_LEDUP2_DATA_RAMr_SET(led_data2, 0);
        for (offset = 0x80; offset < CMIC_LED_DATA_RAM_SIZE; offset++) {
            ioerr += WRITE_CMIC_LEDUP1_DATA_RAMr(unit, offset, led_data1);
            ioerr += WRITE_CMIC_LEDUP2_DATA_RAMr(unit, offset, led_data2);
        }
        /* Start new LED program */
        CMIC_LEDUP1_CTRLr_LEDUP_ENf_SET(led_ctrl1, 1);
        CMIC_LEDUP2_CTRLr_LEDUP_ENf_SET(led_ctrl2, 1);
        ioerr += WRITE_CMIC_LEDUP1_CTRLr(unit, led_ctrl1);
        ioerr += WRITE_CMIC_LEDUP2_CTRLr(unit, led_ctrl2);

        /* Load and start program on LED processor #0 */
        rv = xgsd_led_prog(unit, data, size);
        break;
    default:
        break;
    }

    return ioerr ? CDK_E_IO : rv; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56960_A0 */

