/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include "../bcm56840_a0/bcm56840_a0_bmd.h"
#include "bcm56840_b0_bmd.h"

int 
bcm56840_b0_bmd_attach(
    int unit)
{
    return bcm56840_a0_bmd_attach(unit);
}

