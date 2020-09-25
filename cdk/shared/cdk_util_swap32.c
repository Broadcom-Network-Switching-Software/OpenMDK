/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK utility for byte swapping
 */

#include <cdk/cdk_types.h>
#include <cdk/cdk_util.h>

uint32_t
cdk_util_swap32(uint32_t u32)
{
    u32 = (u32 << 16) | (u32 >> 16);
    return (u32 & 0xff00ffff) >> 8 | (u32 & 0xffff00ff) << 8;
}

