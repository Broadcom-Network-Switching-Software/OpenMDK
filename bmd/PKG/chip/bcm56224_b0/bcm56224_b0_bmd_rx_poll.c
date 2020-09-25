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
bcm56224_b0_bmd_rx_poll(int unit, bmd_pkt_t **ppkt)
{
    return bcm56224_a0_bmd_rx_poll(unit, ppkt);
}

#endif /* CDK_CONFIG_INCLUDE_BCM56224_B0 */
