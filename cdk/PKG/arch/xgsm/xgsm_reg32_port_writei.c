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
 * Access 32 bit internal port-based registers
 */

int
cdk_xgsm_reg32_port_writei(int unit, uint32_t blkacc, int port,
                           uint32_t offset, int idx, uint32_t data)
{
    return cdk_xgsm_reg_port_write(unit, blkacc, port, offset, idx,
                                   &data, 1); 
}
