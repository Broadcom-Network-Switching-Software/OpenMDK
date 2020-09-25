/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS register access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_reg.h>

/*******************************************************************************
 *
 * Access 32 bit internal registers with ACCTYPE equivalent to UNIQUE by both block and port. 
 */

int
cdk_xgsd_reg32_unique_blockport_read(int unit, uint32_t blkacc, int block, 
                                     int blkidx, int port, uint32_t offset, 
                                     int idx, void *data)
{
    return cdk_xgsd_reg_unique_blockport_read(unit, blkacc, block, blkidx, 
                                              port, offset, idx, data, 1);
}
