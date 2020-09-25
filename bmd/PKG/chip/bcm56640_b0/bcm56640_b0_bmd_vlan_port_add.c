/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include "../bcm56640_a0/bcm56640_a0_bmd.h"
#include "bcm56640_b0_bmd.h"

int 
bcm56640_b0_bmd_vlan_port_add(
    int unit, 
    int vlan, 
    int port, 
    uint32_t flags)
{
    return bcm56640_a0_bmd_vlan_port_add(unit, vlan, port, flags);
}

