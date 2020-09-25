/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * ROBO memory access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/arch/robo_mem.h>

int
cdk_robo_mem_clear(int unit, uint32_t addr, uint32_t si, uint32_t ei, int size)
{
    uint32_t idx; 
    static uint32_t zeros[CDK_MAX_REG_WSIZE]; 
    
    for (idx = si; idx <= ei; idx++) {
	cdk_robo_mem_write(unit, addr, idx, zeros, size); 
    }
    return 0;
}
