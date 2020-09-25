/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS register access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/arch/xgsm_reg.h>
    
/*******************************************************************************
 *
 * Access 32 bit internal registers
 */

int
cdk_xgsm_reg32_read(int unit, uint32_t adext, uint32_t addr, void *vptr)
{
    return cdk_xgsm_reg_read(unit, adext, addr, (uint32_t*)vptr, 1); 
}
