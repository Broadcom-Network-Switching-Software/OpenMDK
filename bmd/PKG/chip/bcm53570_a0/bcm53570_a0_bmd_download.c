/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53570_A0 == 1

#include <bmd/bmd.h>
#include <bmdi/arch/xgsd_led.h>

#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm53570_a0_defs.h>

#include "bcm53570_a0_bmd.h"
#include "bcm53570_a0_internal.h"

int 
bcm53570_a0_bmd_download(int unit, bmd_download_t type, uint8_t *data, int size)
{
    return CDK_E_UNAVAIL; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM53570_A0 */
