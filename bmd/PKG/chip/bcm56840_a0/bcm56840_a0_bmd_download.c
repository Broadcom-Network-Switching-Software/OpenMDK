#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56840_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/xgs_led.h>

#include <cdk/chip/bcm56840_a0_defs.h>

#include "bcm56840_a0_bmd.h"

int
bcm56840_a0_bmd_download(int unit, bmd_download_t type, uint8_t *data, int size)
{
    int rv = CDK_E_UNAVAIL;
    int ioerr = 0;
    int offset, idx;
    CMIC_LEDUP0_PORT_ORDER_REMAPr_t remap0;
    CMIC_LEDUP1_PORT_ORDER_REMAPr_t remap1;
    CMIC_LEDUP0_CTRLr_t led_ctrl0;
    CMIC_LEDUP1_CTRLr_t led_ctrl1;
    CMIC_LEDUP0_SCANCHAIN_ASSEMBLY_ST_ADDRr_t scan_addr0;
    CMIC_LEDUP1_SCANCHAIN_ASSEMBLY_ST_ADDRr_t scan_addr1;
    CMIC_LEDUP1_PROGRAM_RAMr_t led_prog1;
    CMIC_LEDUP1_DATA_RAMr_t led_data1;
    uint32_t rval;

    BMD_CHECK_UNIT(unit);

    switch (type) {
    case bmdDownloadPortLedController:
        /* Stop and configure LED processor #1 */
        ioerr += READ_CMIC_LEDUP1_CTRLr(unit, &led_ctrl1);
        CMIC_LEDUP1_CTRLr_LEDUP_ENf_SET(led_ctrl1, 0);
        CMIC_LEDUP1_CTRLr_LEDUP_SCAN_START_DELAYf_SET(led_ctrl1, 15);
        ioerr += WRITE_CMIC_LEDUP1_CTRLr(unit, led_ctrl1);
        CMIC_LEDUP1_SCANCHAIN_ASSEMBLY_ST_ADDRr_SET(scan_addr1, 0x4a);
        ioerr += WRITE_CMIC_LEDUP1_SCANCHAIN_ASSEMBLY_ST_ADDRr(unit, scan_addr1);
        /* Initialize the LEDUP1 port mapping to match LEDUP0's default */
        for (idx = 0; idx < 9; idx++) {
            ioerr += READ_CMIC_LEDUP0_PORT_ORDER_REMAPr(unit, idx, &remap0);
            rval = CMIC_LEDUP0_PORT_ORDER_REMAPr_GET(remap0);
            CMIC_LEDUP1_PORT_ORDER_REMAPr_SET(remap1, rval);
            ioerr += WRITE_CMIC_LEDUP1_PORT_ORDER_REMAPr(unit, idx, remap1);
        }
        /* Load program */
        for (offset = 0; offset < CMIC_LED_PROGRAM_RAM_SIZE; offset++) {
            CMIC_LEDUP1_PROGRAM_RAMr_SET(led_prog1, 0);
            if (offset < size) {
                CMIC_LEDUP1_PROGRAM_RAMr_SET(led_prog1, data[offset]);
            }
            ioerr += WRITE_CMIC_LEDUP1_PROGRAM_RAMr(unit, offset, led_prog1);
        }
        /* The LED data area should be clear whenever program starts */
        CMIC_LEDUP1_DATA_RAMr_SET(led_data1, 0);
        for (offset = 0x80; offset < CMIC_LED_DATA_RAM_SIZE; offset++) {
            ioerr += WRITE_CMIC_LEDUP1_DATA_RAMr(unit, offset, led_data1);
        }
        /* Start new LED program */
        CMIC_LEDUP1_CTRLr_LEDUP_ENf_SET(led_ctrl1, 1);
        ioerr += WRITE_CMIC_LEDUP1_CTRLr(unit, led_ctrl1);

        /* Stop and configure LED processor #0 */
        ioerr += READ_CMIC_LEDUP0_CTRLr(unit, &led_ctrl0);
        CMIC_LEDUP0_CTRLr_LEDUP_ENf_SET(led_ctrl0, 0);
        CMIC_LEDUP0_CTRLr_LEDUP_SCAN_START_DELAYf_SET(led_ctrl0, 11);
        ioerr += WRITE_CMIC_LEDUP0_CTRLr(unit, led_ctrl0);
        CMIC_LEDUP0_SCANCHAIN_ASSEMBLY_ST_ADDRr_SET(scan_addr0, 0x4a);
        ioerr += WRITE_CMIC_LEDUP0_SCANCHAIN_ASSEMBLY_ST_ADDRr(unit, scan_addr0);
        if (ioerr) {
            return CDK_E_IO;
        }
        /* Load and start program on LED processor #0 */
        rv = xgs_led_prog(unit, data, size);
        break;
    default:
        break;
    }

    return rv; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56840_A0 */
