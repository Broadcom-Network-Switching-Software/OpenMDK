/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS memory access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_mem.h>

/*******************************************************************************
 *
 * Access block-based memories
 */

int
cdk_xgsd_mem_unique_block_clear(int unit, uint32_t blkacc, int block,
                                int blkidx, int baseidx, uint32_t offset, 
                                uint32_t si, uint32_t ei, int size)
{
    int ioerr = 0;
    uint32_t idx; 
    static uint32_t zeros[CDK_MAX_REG_WSIZE]; 

    for (idx = si; idx <= ei; idx++) {
        ioerr += cdk_xgsd_mem_unique_block_write(unit, blkacc, block, blkidx, 
                                                 baseidx, offset, idx, 
                                                 zeros, size);
    }
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
