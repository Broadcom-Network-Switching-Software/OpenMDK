/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS register access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/arch/xgs_reg.h>

/*******************************************************************************
 *
 * Access 64 bit internal registers by both block and port. 
 */

int
cdk_xgs_reg64_blockport_read(int unit, int block, int port, 
                             uint32_t addr, void *vptr)
{
    return cdk_xgs_reg64_read(unit, 
                              cdk_xgs_blockport_addr(unit, block, port, addr), 
                              vptr); 
}
