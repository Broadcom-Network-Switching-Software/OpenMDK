/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56670_A0 == 1

#include <bmd/bmd.h>

#include <bmdi/arch/xgsd_led.h>

#include <cdk/chip/bcm56670_a0_defs.h>

#include "bcm56670_a0_bmd.h"
#include "bcm56670_a0_internal.h"

static int
_p2l_value(int unit, int port, int up1_off, int skip)
{
    if (P2L(unit, port) == -1) {
        return 0;
    } else {
        return (port - up1_off - skip);
    }
}

static int
_led_remap(int unit)
{
    int ioerr = 0;
    CMIC_LEDUP0_CTRLr_t ledup0_ctrl;
    uint32_t skip_count = 0;
    int ix;
    int idx;
    int uP1_off;
    uint8_t skip_slot[65];
    uint32_t fval;
    CMIC_LEDUP0_PORT_ORDER_REMAPr_t remap0;
    CMIC_LEDUP0_DATA_RAMr_t ledup0_ram;
    
    /* configure the LED scan delay cycles */
    ioerr += READ_CMIC_LEDUP0_CTRLr(unit, &ledup0_ctrl);
    CMIC_LEDUP0_CTRLr_LEDUP_SCAN_START_DELAYf_SET(ledup0_ctrl, 0xd);
    CMIC_LEDUP0_CTRLr_LEDUP_SCAN_INTRA_PORT_DELAYf_SET(ledup0_ctrl, 4);
    ioerr += WRITE_CMIC_LEDUP0_CTRLr(unit, ledup0_ctrl);
   
    /* Initialize LED Port remap registers */
    CMIC_LEDUP0_PORT_ORDER_REMAPr_CLR(remap0);
    for (idx = 0; idx < 16; idx++) {
        ioerr += WRITE_CMIC_LEDUP0_PORT_ORDER_REMAPr(unit, idx, remap0);
    }

    for (ix = 1; ix <= 64; ix++) {
        if (P2L(unit, ix) == -1) {
            skip_count++;
        }
        skip_slot[ix] = skip_count;
    }
   
    /* Configure LED Port remap registers */
    ix = 1;
    uP1_off = 0;
    CMIC_LEDUP0_PORT_ORDER_REMAPr_CLR(remap0);
    for (idx = 0; idx < 16; idx++) {
        fval = _p2l_value(unit, ix, uP1_off, skip_slot[ix]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_0f_SET(remap0, fval);
        ++ix;
        fval = _p2l_value(unit, ix, uP1_off, skip_slot[ix]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_1f_SET(remap0, fval);
        ++ix;
        fval = _p2l_value(unit, ix, uP1_off, skip_slot[ix]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_2f_SET(remap0, fval);
        ++ix;
        fval = _p2l_value(unit, ix, uP1_off, skip_slot[ix]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_3f_SET(remap0, fval);
        ++ix;
        fval = _p2l_value(unit, ix, uP1_off, skip_slot[ix]);
        ioerr += WRITE_CMIC_LEDUP0_PORT_ORDER_REMAPr(unit, idx, remap0);
    }

    /* initialize the UP0, UP1 data ram */
    for (ix = 0; ix < 256; ix++) {
        CMIC_LEDUP0_DATA_RAMr_CLR(ledup0_ram);
        ioerr += WRITE_CMIC_LEDUP0_DATA_RAMr(unit, ix, ledup0_ram);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

int
bcm56670_a0_bmd_download(int unit, bmd_download_t type, uint8_t *data, int size)
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
#endif /* CDK_CONFIG_INCLUDE_BCM56670_A0 */

