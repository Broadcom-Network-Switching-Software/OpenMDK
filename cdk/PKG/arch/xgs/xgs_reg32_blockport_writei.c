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
 * Access 32 bit internal registers by both block and port. 
 */

int
cdk_xgs_reg32_blockport_writei(int unit, int block, int port, 
                               uint32_t addr, uint32_t data)
{
    return cdk_xgs_reg32_writei(unit, 
                                cdk_xgs_blockport_addr(unit, block, port, addr), 
                                data); 
}
