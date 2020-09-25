/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#ifdef CDK_CONFIG_ARCH_XGSD_INSTALLED

#include <bmdi/arch/xgsd_led.h>

#include <cdk/cdk_device.h>

int
xgsd_led_prog(int unit, uint8_t *program, int size)
{
    uint32_t led_ctrl;
    int offset, port;

    /* Stop LED processor */
    CDK_DEV_READ32(unit, CMIC_LED_CTRL, &led_ctrl);
    led_ctrl &= ~LC_LED_ENABLE;
    CDK_DEV_WRITE32(unit, CMIC_LED_CTRL, led_ctrl);

    /* Load program */
    for (offset = 0; offset < CMIC_LED_PROGRAM_RAM_SIZE; offset++) {
	CDK_DEV_WRITE32(unit, CMIC_LED_PROGRAM_RAM(offset),
                        (offset < size) ? (uint32_t)program[offset] : 0);
    }

    /* The LED data area should be clear whenever program starts */
    for (offset = 0x80; offset < CMIC_LED_DATA_RAM_SIZE; offset++) {
        CDK_DEV_WRITE32(unit, CMIC_LED_DATA_RAM(offset), 0);
    }

    /* Start new LED program */
    led_ctrl |= LC_LED_ENABLE;
    CDK_DEV_WRITE32(unit, CMIC_LED_CTRL, led_ctrl);

    /* Force a link change event on all ports */
    BMD_PORT_ITER(unit, BMD_PORT_ALL, port) {
        BMD_PORT_STATUS_SET(unit, port, BMD_PST_FORCE_UPDATE);
    }

    return CDK_E_NONE;
}

#endif /* CDK_CONFIG_ARCH_XGSD_INSTALLED */
