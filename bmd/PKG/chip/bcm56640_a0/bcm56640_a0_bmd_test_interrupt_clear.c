#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56640_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/xgsm_test_interrupt.h>

#include "bcm56640_a0_bmd.h"

int
bcm56640_a0_bmd_test_interrupt_clear(int unit)
{
    return bmd_xgsm_test_interrupt_clear(unit); 
}

#endif /* CDK_CONFIG_INCLUDE_BCM56640_A0 */
