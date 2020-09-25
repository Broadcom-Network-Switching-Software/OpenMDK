/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include "../bcm56450_a0/bcm56450_a0_bmd.h"
#include "bcm56450_b0_bmd.h"

int 
bcm56450_b0_bmd_stat_get(
    int unit, 
    int port, 
    bmd_stat_t stat, 
    bmd_counter_t *counter)
{
    return bcm56450_a0_bmd_stat_get(unit, port, stat, counter);
}

