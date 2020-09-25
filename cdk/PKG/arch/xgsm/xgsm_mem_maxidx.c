/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS memory access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/arch/xgsm_mem.h>

uint32_t
cdk_xgsm_mem_maxidx(int unit, int enum_val, uint32_t maxidx)
{
    if (CDK_XGSM_INFO(unit)->mem_maxidx) {
        maxidx = CDK_XGSM_INFO(unit)->mem_maxidx(enum_val, maxidx);
    }
    return maxidx;
}
