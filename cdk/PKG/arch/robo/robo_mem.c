/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * ROBO memory access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_field.h>

#include <cdk/arch/robo_mem_regs.h>
#include <cdk/arch/robo_mem.h>

int
cdk_robo_mem_read(int unit, uint32_t addr, uint32_t idx, void *vptr, int size)
{
    switch (ROBO_MEM_ACCESS_METHOD(addr)) {
    case ROBO_MEM_ACC_ARL:
        return cdk_robo_mem_arl_read(unit, addr, idx, vptr, size);
    case ROBO_MEM_ACC_VLAN:
        return cdk_robo_mem_vlan_read(unit, addr, idx, vptr, size);
    case ROBO_MEM_ACC_GARL:
        return cdk_robo_mem_garl_read(unit, addr, idx, vptr, size);
    case ROBO_MEM_ACC_SARL:
        return cdk_robo_mem_sarl_read(unit, addr, idx, vptr, size);
    case ROBO_MEM_ACC_GEN:
        return cdk_robo_mem_gen_read(unit, addr, idx, vptr, size);
    default:
        CDK_ERR(("cdk_robo_mem_read[%d]: unknown access method\n", unit));
        break;
    }

    /* Unsupported access method */
    return CDK_E_FAIL;
}

int
cdk_robo_mem_write(int unit, uint32_t addr, uint32_t idx, void *vptr, int size)
{
    switch (ROBO_MEM_ACCESS_METHOD(addr)) {
    case ROBO_MEM_ACC_ARL:
        return cdk_robo_mem_arl_write(unit, addr, idx, vptr, size);
    case ROBO_MEM_ACC_VLAN:
        return cdk_robo_mem_vlan_write(unit, addr, idx, vptr, size);
    case ROBO_MEM_ACC_GARL:
        return cdk_robo_mem_garl_write(unit, addr, idx, vptr, size);
    case ROBO_MEM_ACC_SARL:
        return cdk_robo_mem_sarl_write(unit, addr, idx, vptr, size);
    case ROBO_MEM_ACC_GEN:
        return cdk_robo_mem_gen_write(unit, addr, idx, vptr, size);
    default:
        CDK_ERR(("cdk_robo_mem_write[%d]: unknown access method\n", unit));
        break;
    }

    /* Unsupported access method */
    return CDK_E_FAIL;
}
