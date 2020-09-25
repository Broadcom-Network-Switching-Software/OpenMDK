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

/*******************************************************************************
 *
 * Access block-based memories
 */

int
cdk_xgs_mem_block_write(int unit, int block, uint32_t addr, int idx, 
                        void *entry_data, int size)
{	
    return cdk_xgs_mem_write(unit, cdk_xgs_block_addr(unit, block, addr), 
                             idx, entry_data, size); 
}
