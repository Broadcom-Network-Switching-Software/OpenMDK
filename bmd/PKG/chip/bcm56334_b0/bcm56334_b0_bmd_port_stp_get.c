#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56334_B0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include "../bcm56334_a0/bcm56334_a0_bmd.h"
#include "bcm56334_b0_bmd.h"

int
bcm56334_b0_bmd_port_stp_get(int unit, int port, bmd_stp_state_t *state)
{
    return bcm56334_a0_bmd_port_stp_get(unit, port, state);
}

#endif /* CDK_CONFIG_INCLUDE_BCM56334_B0 */
