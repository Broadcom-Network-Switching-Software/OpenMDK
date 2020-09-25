#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53262_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/robo_stp_xlate.h>

#include <cdk/chip/bcm53262_a0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include "bcm53262_a0_bmd.h"

int
bcm53262_a0_bmd_port_stp_get(int unit, int port, bmd_stp_state_t *state)
{

    int ioerr = 0;
    int hw_state;
    MSPT_TABm_t stg_tab;

    BMD_CHECK_UNIT(unit);

    ioerr += READ_MSPT_TABm(unit, 1, &stg_tab);

    switch (port) {
    case 0: hw_state = MSPT_TABm_MSP_TREE_PORT0f_GET(stg_tab); break;
    case 1: hw_state = MSPT_TABm_MSP_TREE_PORT1f_GET(stg_tab); break;
    case 2: hw_state = MSPT_TABm_MSP_TREE_PORT2f_GET(stg_tab); break;
    case 3: hw_state = MSPT_TABm_MSP_TREE_PORT3f_GET(stg_tab); break;
    case 4: hw_state = MSPT_TABm_MSP_TREE_PORT4f_GET(stg_tab); break;
    case 5: hw_state = MSPT_TABm_MSP_TREE_PORT5f_GET(stg_tab); break;
    case 6: hw_state = MSPT_TABm_MSP_TREE_PORT6f_GET(stg_tab); break;
    case 7: hw_state = MSPT_TABm_MSP_TREE_PORT7f_GET(stg_tab); break;
    case 8: hw_state = MSPT_TABm_MSP_TREE_PORT8f_GET(stg_tab); break;
    case 9: hw_state = MSPT_TABm_MSP_TREE_PORT9f_GET(stg_tab); break;
    case 10: hw_state = MSPT_TABm_MSP_TREE_PORT10f_GET(stg_tab); break;
    case 11: hw_state = MSPT_TABm_MSP_TREE_PORT11f_GET(stg_tab); break;
    case 12: hw_state = MSPT_TABm_MSP_TREE_PORT12f_GET(stg_tab); break;
    case 13: hw_state = MSPT_TABm_MSP_TREE_PORT13f_GET(stg_tab); break;
    case 14: hw_state = MSPT_TABm_MSP_TREE_PORT14f_GET(stg_tab); break;
    case 15: hw_state = MSPT_TABm_MSP_TREE_PORT15f_GET(stg_tab); break;
    case 16: hw_state = MSPT_TABm_MSP_TREE_PORT16f_GET(stg_tab); break;
    case 17: hw_state = MSPT_TABm_MSP_TREE_PORT17f_GET(stg_tab); break;
    case 18: hw_state = MSPT_TABm_MSP_TREE_PORT18f_GET(stg_tab); break;
    case 19: hw_state = MSPT_TABm_MSP_TREE_PORT19f_GET(stg_tab); break;
    case 20: hw_state = MSPT_TABm_MSP_TREE_PORT20f_GET(stg_tab); break;
    case 21: hw_state = MSPT_TABm_MSP_TREE_PORT21f_GET(stg_tab); break;
    case 22: hw_state = MSPT_TABm_MSP_TREE_PORT22f_GET(stg_tab); break;
    case 23: hw_state = MSPT_TABm_MSP_TREE_PORT23f_GET(stg_tab); break;
    case 24: hw_state = MSPT_TABm_MSP_TREE_PORT24f_GET(stg_tab); break;
    case 25: hw_state = MSPT_TABm_MSP_TREE_PORT25f_GET(stg_tab); break;
    case 26: hw_state = MSPT_TABm_MSP_TREE_PORT26f_GET(stg_tab); break;
    case 27: hw_state = MSPT_TABm_MSP_TREE_PORT27f_GET(stg_tab); break;
    case 28: hw_state = MSPT_TABm_MSP_TREE_PORT28f_GET(stg_tab); break;
    default:
        return ioerr ? CDK_E_IO : CDK_E_PORT;
    }

    if (CDK_FAILURE(bmd_robo_stp_state_from_hw(hw_state, state))) {
        /* should never fail */
        CDK_ASSERT(0);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53262_A0 */
