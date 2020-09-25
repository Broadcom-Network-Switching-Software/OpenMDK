#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56224_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/xgs_stp_xlate.h>

#include <cdk/chip/bcm56224_a0_defs.h>
#include <cdk/arch/xgs_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm56224_a0_bmd.h"

int
bcm56224_a0_bmd_port_stp_set(int unit, int port, bmd_stp_state_t state)
{
    int ioerr = 0;
    int hw_state;
    STG_TABm_t stg_tab;
    EGR_VLAN_STGm_t egr_vlan_stg;

    BMD_CHECK_UNIT(unit);

    if (CDK_FAILURE(bmd_xgs_stp_state_to_hw(state, &hw_state))) {
        return CDK_E_PARAM;
    }

    ioerr += READ_STG_TABm(unit, 1, &stg_tab);

    switch (port) {
    case 0: STG_TABm_SP_TREE_PORT0f_SET(stg_tab, hw_state); break;
    case 1: STG_TABm_SP_TREE_PORT1f_SET(stg_tab, hw_state); break;
    case 2: STG_TABm_SP_TREE_PORT2f_SET(stg_tab, hw_state); break;
    case 3: STG_TABm_SP_TREE_PORT3f_SET(stg_tab, hw_state); break;
    case 4: STG_TABm_SP_TREE_PORT4f_SET(stg_tab, hw_state); break;
    case 5: STG_TABm_SP_TREE_PORT5f_SET(stg_tab, hw_state); break;
    case 6: STG_TABm_SP_TREE_PORT6f_SET(stg_tab, hw_state); break;
    case 7: STG_TABm_SP_TREE_PORT7f_SET(stg_tab, hw_state); break;
    case 8: STG_TABm_SP_TREE_PORT8f_SET(stg_tab, hw_state); break;
    case 9: STG_TABm_SP_TREE_PORT9f_SET(stg_tab, hw_state); break;
    case 10: STG_TABm_SP_TREE_PORT10f_SET(stg_tab, hw_state); break;
    case 11: STG_TABm_SP_TREE_PORT11f_SET(stg_tab, hw_state); break;
    case 12: STG_TABm_SP_TREE_PORT12f_SET(stg_tab, hw_state); break;
    case 13: STG_TABm_SP_TREE_PORT13f_SET(stg_tab, hw_state); break;
    case 14: STG_TABm_SP_TREE_PORT14f_SET(stg_tab, hw_state); break;
    case 15: STG_TABm_SP_TREE_PORT15f_SET(stg_tab, hw_state); break;
    case 16: STG_TABm_SP_TREE_PORT16f_SET(stg_tab, hw_state); break;
    case 17: STG_TABm_SP_TREE_PORT17f_SET(stg_tab, hw_state); break;
    case 18: STG_TABm_SP_TREE_PORT18f_SET(stg_tab, hw_state); break;
    case 19: STG_TABm_SP_TREE_PORT19f_SET(stg_tab, hw_state); break;
    case 20: STG_TABm_SP_TREE_PORT20f_SET(stg_tab, hw_state); break;
    case 21: STG_TABm_SP_TREE_PORT21f_SET(stg_tab, hw_state); break;
    case 22: STG_TABm_SP_TREE_PORT22f_SET(stg_tab, hw_state); break;
    case 23: STG_TABm_SP_TREE_PORT23f_SET(stg_tab, hw_state); break;
    case 24: STG_TABm_SP_TREE_PORT24f_SET(stg_tab, hw_state); break;
    case 25: STG_TABm_SP_TREE_PORT25f_SET(stg_tab, hw_state); break;
    case 26: STG_TABm_SP_TREE_PORT26f_SET(stg_tab, hw_state); break;
    case 27: STG_TABm_SP_TREE_PORT27f_SET(stg_tab, hw_state); break;
    case 28: STG_TABm_SP_TREE_PORT28f_SET(stg_tab, hw_state); break;
    case 29: STG_TABm_SP_TREE_PORT29f_SET(stg_tab, hw_state); break;
    default:
        return ioerr ? CDK_E_IO : CDK_E_PORT;
    }

    ioerr += WRITE_STG_TABm(unit, 1, stg_tab);

    /* EGR_VLAN_STG has same field layout as STG_TAB */
    CDK_MEMCPY(&egr_vlan_stg, &stg_tab, sizeof(egr_vlan_stg));
    ioerr += WRITE_EGR_VLAN_STGm(unit, 1, egr_vlan_stg);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56224_A0 */
