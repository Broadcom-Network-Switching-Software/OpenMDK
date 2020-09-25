#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53600_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/robo_stp_xlate.h>

#include <cdk/chip/bcm53600_a0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm53600_a0_bmd.h"

int
bcm53600_a0_bmd_port_stp_set(int unit, int port, bmd_stp_state_t state)
{
    int ioerr = 0;
    int hw_state;
    MSPT_TABm_t stg_tab;
    
    BMD_CHECK_UNIT(unit);

    /* TB use 2 bits (4 states) differ from Robo other chips */
    switch (state) {
    case bmdSpanningTreeDisabled:
        hw_state = 0;
        break;
    case bmdSpanningTreeBlocking:
        hw_state = 1;
        break;
    case bmdSpanningTreeListening:
        hw_state = 1;
        break;
    case bmdSpanningTreeLearning:
        hw_state = 2;
        break;
    case bmdSpanningTreeForwarding:
        hw_state = 3;
        break;
    default:
        return CDK_E_PARAM;
        
    }

    ioerr += READ_MSPT_TABm(unit, 1, &stg_tab);

    switch (port) {
    case 0: MSPT_TABm_MSP_TREE_PORT0f_SET(stg_tab, hw_state); break;
    case 1: MSPT_TABm_MSP_TREE_PORT1f_SET(stg_tab, hw_state); break;
    case 2: MSPT_TABm_MSP_TREE_PORT2f_SET(stg_tab, hw_state); break;
    case 3: MSPT_TABm_MSP_TREE_PORT3f_SET(stg_tab, hw_state); break;
    case 4: MSPT_TABm_MSP_TREE_PORT4f_SET(stg_tab, hw_state); break;
    case 5: MSPT_TABm_MSP_TREE_PORT5f_SET(stg_tab, hw_state); break;
    case 6: MSPT_TABm_MSP_TREE_PORT6f_SET(stg_tab, hw_state); break;
    case 7: MSPT_TABm_MSP_TREE_PORT7f_SET(stg_tab, hw_state); break;
    case 8: MSPT_TABm_MSP_TREE_PORT8f_SET(stg_tab, hw_state); break;
    case 9: MSPT_TABm_MSP_TREE_PORT9f_SET(stg_tab, hw_state); break;
    case 10: MSPT_TABm_MSP_TREE_PORT10f_SET(stg_tab, hw_state); break;
    case 11: MSPT_TABm_MSP_TREE_PORT11f_SET(stg_tab, hw_state); break;
    case 12: MSPT_TABm_MSP_TREE_PORT12f_SET(stg_tab, hw_state); break;
    case 13: MSPT_TABm_MSP_TREE_PORT13f_SET(stg_tab, hw_state); break;
    case 14: MSPT_TABm_MSP_TREE_PORT14f_SET(stg_tab, hw_state); break;
    case 15: MSPT_TABm_MSP_TREE_PORT15f_SET(stg_tab, hw_state); break;
    case 16: MSPT_TABm_MSP_TREE_PORT16f_SET(stg_tab, hw_state); break;
    case 17: MSPT_TABm_MSP_TREE_PORT17f_SET(stg_tab, hw_state); break;
    case 18: MSPT_TABm_MSP_TREE_PORT18f_SET(stg_tab, hw_state); break;
    case 19: MSPT_TABm_MSP_TREE_PORT19f_SET(stg_tab, hw_state); break;
    case 20: MSPT_TABm_MSP_TREE_PORT20f_SET(stg_tab, hw_state); break;
    case 21: MSPT_TABm_MSP_TREE_PORT21f_SET(stg_tab, hw_state); break;
    case 22: MSPT_TABm_MSP_TREE_PORT22f_SET(stg_tab, hw_state); break;
    case 23: MSPT_TABm_MSP_TREE_PORT23f_SET(stg_tab, hw_state); break;
    case 24: MSPT_TABm_MSP_TREE_PORT24f_SET(stg_tab, hw_state); break;
    case 25: MSPT_TABm_MSP_TREE_PORT25f_SET(stg_tab, hw_state); break;
    case 26: MSPT_TABm_MSP_TREE_PORT26f_SET(stg_tab, hw_state); break;
    case 27: MSPT_TABm_MSP_TREE_PORT27f_SET(stg_tab, hw_state); break;
    case 28: MSPT_TABm_MSP_TREE_PORT28f_SET(stg_tab, hw_state); break;
    default:
        return ioerr ? CDK_E_IO : CDK_E_PORT;
    }

    ioerr += WRITE_MSPT_TABm(unit, 1, stg_tab);
    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53600_A0 */
