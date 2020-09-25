#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53125_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/robo_stp_xlate.h>

#include <cdk/chip/bcm53125_a0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm53125_a0_bmd.h"

int
bcm53125_a0_bmd_port_stp_set(int unit, int port, bmd_stp_state_t state)
{
    int ioerr = 0;
    int hw_state;
    MST_TABr_t stg_tab;

    BMD_CHECK_UNIT(unit);

    if (CDK_FAILURE(bmd_robo_stp_state_to_hw(state, &hw_state))) {
        return CDK_E_PARAM;
    }
    /* always use stp=1 entry to show the vlan existed */
    ioerr += READ_MST_TABr(unit, 1, &stg_tab);

    switch (port) {
    case 0: MST_TABr_SPT_STA0f_SET(stg_tab, hw_state); break;
    case 1: MST_TABr_SPT_STA1f_SET(stg_tab, hw_state); break;
    case 2: MST_TABr_SPT_STA2f_SET(stg_tab, hw_state); break;
    case 3: MST_TABr_SPT_STA3f_SET(stg_tab, hw_state); break;
    case 4: MST_TABr_SPT_STA4f_SET(stg_tab, hw_state); break;
    case 5: MST_TABr_SPT_STA5f_SET(stg_tab, hw_state); break;
    default:
        return ioerr ? CDK_E_IO : CDK_E_PORT;
    }

    ioerr += WRITE_MST_TABr(unit, 1, stg_tab);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53125_A0 */
