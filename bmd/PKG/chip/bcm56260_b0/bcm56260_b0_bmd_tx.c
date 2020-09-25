/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56260_B0 == 1

#include <bmd/bmd.h>

#include <cdk/chip/bcm56260_b0_defs.h>

#include "bcm56260_b0_bmd.h"

int 
bcm56260_b0_bmd_tx(int unit, const bmd_pkt_t *pkt)
{
    return bcm56260_a0_bmd_tx(unit, pkt); 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56260_B0 */
