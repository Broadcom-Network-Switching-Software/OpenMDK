#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56624_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/xgs_led.h>

#include <cdk/cdk_device.h>

#include "bcm56624_a0_bmd.h"

int
bcm56624_a0_bmd_download(int unit, bmd_download_t type, uint8_t *data, int size)
{
    int rv = CDK_E_UNAVAIL;

    BMD_CHECK_UNIT(unit);

    switch (type) {
    case bmdDownloadPortLedController:
        rv = xgs_led_prog(unit, data, size);
        break;
    default:
        break;
    }

    return rv; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56624_A0 */
