/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS memory access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/arch/xgs_mem.h>

uint32_t
cdk_xgs_mem_maxidx(int unit, uint32_t addr, uint32_t maxidx)
{
    if (CDK_XGS_INFO(unit)->mem_maxidx) {
        maxidx = CDK_XGS_INFO(unit)->mem_maxidx(addr, maxidx);
    }
    return maxidx;
}
