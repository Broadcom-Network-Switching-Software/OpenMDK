/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56160_A0 == 1

#include <bmd/bmd.h>
#include <bmdi/arch/xgsd_test_interrupt.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_error.h>
#include "bcm56160_a0_bmd.h"

int 
bcm56160_a0_bmd_test_interrupt_assert(int unit)
{
    return bmd_xgsd_test_interrupt_assert(unit); 
}

#endif /* CDK_CONFIG_INCLUDE_BCM56160_A0 */
