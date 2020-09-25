#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56224_B0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include "../bcm56224_a0/bcm56224_a0_bmd.h"
#include "bcm56224_b0_bmd.h"

int
bcm56224_b0_bmd_stat_clear(int unit, int port, bmd_stat_t stat)
{
    return bcm56224_a0_bmd_stat_clear(unit, port, stat);
}

#endif /* CDK_CONFIG_INCLUDE_BCM56224_B0 */
