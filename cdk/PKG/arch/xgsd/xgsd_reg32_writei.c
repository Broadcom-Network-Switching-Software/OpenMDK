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
 * Access 32 bit internal registers
 */

int
cdk_xgsd_reg32_writei(int unit, uint32_t adext,  uint32_t addr, uint32_t data)
{
    return cdk_xgsd_reg_write(unit, adext, addr, &data, 1); 
}

