/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * ROBO ARL access through debug memory interface.
 */

#include <cdk/cdk_debug.h>

#include <cdk/arch/robo_mem.h>

int
cdk_robo_mem_sarl_write(int unit, uint32_t addr, uint32_t idx, void *vptr, int size)
{
    CDK_ERR(("cdk_robo_mem_sarl_write[%d]: SARL access is read-only\n", unit));

    return CDK_E_FAIL;
}
