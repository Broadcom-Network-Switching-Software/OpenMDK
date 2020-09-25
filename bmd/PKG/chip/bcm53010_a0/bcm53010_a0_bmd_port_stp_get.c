#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53010_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/robo_stp_xlate.h>

#include <cdk/chip/bcm53010_a0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include "bcm53010_a0_bmd.h"

int
bcm53010_a0_bmd_port_stp_get(int unit, int port, bmd_stp_state_t *state)
{
    int ioerr = 0;
    int hw_state;
    MST_TABr_t stg_tab;

    BMD_CHECK_UNIT(unit);

    ioerr += READ_MST_TABr(unit, 1, &stg_tab);

    switch (port) {
    case 0: hw_state = MST_TABr_SPT_STA0f_GET(stg_tab); break;
    case 1: hw_state = MST_TABr_SPT_STA1f_GET(stg_tab); break;
    case 2: hw_state = MST_TABr_SPT_STA2f_GET(stg_tab); break;
    case 3: hw_state = MST_TABr_SPT_STA3f_GET(stg_tab); break;
    case 4: hw_state = MST_TABr_SPT_STA4f_GET(stg_tab); break;
    case 5: hw_state = MST_TABr_SPT_STA5f_GET(stg_tab); break;
    case 7: hw_state = MST_TABr_SPT_STA7f_GET(stg_tab); break;
    default:
        return ioerr ? CDK_E_IO : CDK_E_PORT;
    }

    if (CDK_FAILURE(bmd_robo_stp_state_from_hw(hw_state, state))) {
        /* should never fail */
        CDK_ASSERT(0);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53010_A0 */
