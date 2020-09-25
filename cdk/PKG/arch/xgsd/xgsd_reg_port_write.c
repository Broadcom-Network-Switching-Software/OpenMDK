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
 * Access internal port-based registers
 */

int
cdk_xgsd_reg_port_write(int unit, uint32_t blkacc, int port,
                        uint32_t offset, int idx, void *data, int size)
{
    uint32_t addr, adext;

    addr = cdk_xgsd_port_addr(unit, blkacc, port, offset, idx, &adext);

    return cdk_xgsd_reg_write(unit, adext, addr, data, size);
}
