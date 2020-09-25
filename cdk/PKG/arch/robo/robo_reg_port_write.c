/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * ROBO register access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/arch/robo_reg.h>
    
/*******************************************************************************
 *
 * Access internal port-based registers
 */

int
cdk_robo_reg_port_write(int unit, int port, uint32_t addr, void *vptr, int size)
{
    return cdk_robo_reg_write(unit, cdk_robo_port_addr(unit, port, size, addr), 
                              vptr, size); 
}
