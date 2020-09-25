/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56960_A0 == 1

#include <bmd/bmd.h>
#include <bmdi/arch/xgsd_stp_xlate.h>

#include <cdk/chip/bcm56960_a0_defs.h>

#include "bcm56960_a0_bmd.h"
#include "bcm56960_a0_internal.h"

int 
bcm56960_a0_bmd_port_stp_get(int unit, int port, bmd_stp_state_t *state)
{
    int ioerr = 0;
    int lport;
    int hw_state;
    STG_TABm_t stg_tab;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    lport = P2L(unit, port);

    ioerr += READ_STG_TABm(unit, 1, &stg_tab);

    switch (lport) {
    case 1: hw_state = STG_TABm_SP_TREE_PORT1f_GET(stg_tab); break;
    case 2: hw_state = STG_TABm_SP_TREE_PORT2f_GET(stg_tab); break;
    case 3: hw_state = STG_TABm_SP_TREE_PORT3f_GET(stg_tab); break;
    case 4: hw_state = STG_TABm_SP_TREE_PORT4f_GET(stg_tab); break;
    case 5: hw_state = STG_TABm_SP_TREE_PORT5f_GET(stg_tab); break;
    case 6: hw_state = STG_TABm_SP_TREE_PORT6f_GET(stg_tab); break;
    case 7: hw_state = STG_TABm_SP_TREE_PORT7f_GET(stg_tab); break;
    case 8: hw_state = STG_TABm_SP_TREE_PORT8f_GET(stg_tab); break;
    case 9: hw_state = STG_TABm_SP_TREE_PORT9f_GET(stg_tab); break;
    case 10: hw_state = STG_TABm_SP_TREE_PORT10f_GET(stg_tab); break;
    case 11: hw_state = STG_TABm_SP_TREE_PORT11f_GET(stg_tab); break;
    case 12: hw_state = STG_TABm_SP_TREE_PORT12f_GET(stg_tab); break;
    case 13: hw_state = STG_TABm_SP_TREE_PORT13f_GET(stg_tab); break;
    case 14: hw_state = STG_TABm_SP_TREE_PORT14f_GET(stg_tab); break;
    case 15: hw_state = STG_TABm_SP_TREE_PORT15f_GET(stg_tab); break;
    case 16: hw_state = STG_TABm_SP_TREE_PORT16f_GET(stg_tab); break;
    case 17: hw_state = STG_TABm_SP_TREE_PORT17f_GET(stg_tab); break;
    case 18: hw_state = STG_TABm_SP_TREE_PORT18f_GET(stg_tab); break;
    case 19: hw_state = STG_TABm_SP_TREE_PORT19f_GET(stg_tab); break;
    case 20: hw_state = STG_TABm_SP_TREE_PORT20f_GET(stg_tab); break;
    case 21: hw_state = STG_TABm_SP_TREE_PORT21f_GET(stg_tab); break;
    case 22: hw_state = STG_TABm_SP_TREE_PORT22f_GET(stg_tab); break;
    case 23: hw_state = STG_TABm_SP_TREE_PORT23f_GET(stg_tab); break;
    case 24: hw_state = STG_TABm_SP_TREE_PORT24f_GET(stg_tab); break;
    case 25: hw_state = STG_TABm_SP_TREE_PORT25f_GET(stg_tab); break;
    case 26: hw_state = STG_TABm_SP_TREE_PORT26f_GET(stg_tab); break;
    case 27: hw_state = STG_TABm_SP_TREE_PORT27f_GET(stg_tab); break;
    case 28: hw_state = STG_TABm_SP_TREE_PORT28f_GET(stg_tab); break;
    case 29: hw_state = STG_TABm_SP_TREE_PORT29f_GET(stg_tab); break;
    case 30: hw_state = STG_TABm_SP_TREE_PORT30f_GET(stg_tab); break;
    case 31: hw_state = STG_TABm_SP_TREE_PORT31f_GET(stg_tab); break;
    case 32: hw_state = STG_TABm_SP_TREE_PORT32f_GET(stg_tab); break;
    case 33: hw_state = STG_TABm_SP_TREE_PORT33f_GET(stg_tab); break;
    case 34: hw_state = STG_TABm_SP_TREE_PORT34f_GET(stg_tab); break;
    case 35: hw_state = STG_TABm_SP_TREE_PORT35f_GET(stg_tab); break;
    case 36: hw_state = STG_TABm_SP_TREE_PORT36f_GET(stg_tab); break;
    case 37: hw_state = STG_TABm_SP_TREE_PORT37f_GET(stg_tab); break;
    case 38: hw_state = STG_TABm_SP_TREE_PORT38f_GET(stg_tab); break;
    case 39: hw_state = STG_TABm_SP_TREE_PORT39f_GET(stg_tab); break;
    case 40: hw_state = STG_TABm_SP_TREE_PORT40f_GET(stg_tab); break;
    case 41: hw_state = STG_TABm_SP_TREE_PORT41f_GET(stg_tab); break;
    case 42: hw_state = STG_TABm_SP_TREE_PORT42f_GET(stg_tab); break;
    case 43: hw_state = STG_TABm_SP_TREE_PORT43f_GET(stg_tab); break;
    case 44: hw_state = STG_TABm_SP_TREE_PORT44f_GET(stg_tab); break;
    case 45: hw_state = STG_TABm_SP_TREE_PORT45f_GET(stg_tab); break;
    case 46: hw_state = STG_TABm_SP_TREE_PORT46f_GET(stg_tab); break;
    case 47: hw_state = STG_TABm_SP_TREE_PORT47f_GET(stg_tab); break;
    case 48: hw_state = STG_TABm_SP_TREE_PORT48f_GET(stg_tab); break;
    case 49: hw_state = STG_TABm_SP_TREE_PORT49f_GET(stg_tab); break;
    case 50: hw_state = STG_TABm_SP_TREE_PORT50f_GET(stg_tab); break;
    case 51: hw_state = STG_TABm_SP_TREE_PORT51f_GET(stg_tab); break;
    case 52: hw_state = STG_TABm_SP_TREE_PORT52f_GET(stg_tab); break;
    case 53: hw_state = STG_TABm_SP_TREE_PORT53f_GET(stg_tab); break;
    case 54: hw_state = STG_TABm_SP_TREE_PORT54f_GET(stg_tab); break;
    case 55: hw_state = STG_TABm_SP_TREE_PORT55f_GET(stg_tab); break;
    case 56: hw_state = STG_TABm_SP_TREE_PORT56f_GET(stg_tab); break;
    case 57: hw_state = STG_TABm_SP_TREE_PORT57f_GET(stg_tab); break;
    case 58: hw_state = STG_TABm_SP_TREE_PORT58f_GET(stg_tab); break;
    case 59: hw_state = STG_TABm_SP_TREE_PORT59f_GET(stg_tab); break;
    case 60: hw_state = STG_TABm_SP_TREE_PORT60f_GET(stg_tab); break;
    case 61: hw_state = STG_TABm_SP_TREE_PORT61f_GET(stg_tab); break;
    case 62: hw_state = STG_TABm_SP_TREE_PORT62f_GET(stg_tab); break;
    case 63: hw_state = STG_TABm_SP_TREE_PORT63f_GET(stg_tab); break;
    case 64: hw_state = STG_TABm_SP_TREE_PORT64f_GET(stg_tab); break;
    case 65: hw_state = STG_TABm_SP_TREE_PORT65f_GET(stg_tab); break;
    case 66: hw_state = STG_TABm_SP_TREE_PORT66f_GET(stg_tab); break;
    case 67: hw_state = STG_TABm_SP_TREE_PORT67f_GET(stg_tab); break;
    case 68: hw_state = STG_TABm_SP_TREE_PORT68f_GET(stg_tab); break;
    case 69: hw_state = STG_TABm_SP_TREE_PORT69f_GET(stg_tab); break;
    case 70: hw_state = STG_TABm_SP_TREE_PORT70f_GET(stg_tab); break;
    case 71: hw_state = STG_TABm_SP_TREE_PORT71f_GET(stg_tab); break;
    case 72: hw_state = STG_TABm_SP_TREE_PORT72f_GET(stg_tab); break;
    case 73: hw_state = STG_TABm_SP_TREE_PORT73f_GET(stg_tab); break;
    case 74: hw_state = STG_TABm_SP_TREE_PORT74f_GET(stg_tab); break;
    case 75: hw_state = STG_TABm_SP_TREE_PORT75f_GET(stg_tab); break;
    case 76: hw_state = STG_TABm_SP_TREE_PORT76f_GET(stg_tab); break;
    case 77: hw_state = STG_TABm_SP_TREE_PORT77f_GET(stg_tab); break;
    case 78: hw_state = STG_TABm_SP_TREE_PORT78f_GET(stg_tab); break;
    case 79: hw_state = STG_TABm_SP_TREE_PORT79f_GET(stg_tab); break;
    case 80: hw_state = STG_TABm_SP_TREE_PORT80f_GET(stg_tab); break;
    case 81: hw_state = STG_TABm_SP_TREE_PORT81f_GET(stg_tab); break;
    case 82: hw_state = STG_TABm_SP_TREE_PORT82f_GET(stg_tab); break;
    case 83: hw_state = STG_TABm_SP_TREE_PORT83f_GET(stg_tab); break;
    case 84: hw_state = STG_TABm_SP_TREE_PORT84f_GET(stg_tab); break;
    case 85: hw_state = STG_TABm_SP_TREE_PORT85f_GET(stg_tab); break;
    case 86: hw_state = STG_TABm_SP_TREE_PORT86f_GET(stg_tab); break;
    case 87: hw_state = STG_TABm_SP_TREE_PORT87f_GET(stg_tab); break;
    case 88: hw_state = STG_TABm_SP_TREE_PORT88f_GET(stg_tab); break;
    case 89: hw_state = STG_TABm_SP_TREE_PORT89f_GET(stg_tab); break;
    case 90: hw_state = STG_TABm_SP_TREE_PORT90f_GET(stg_tab); break;
    case 91: hw_state = STG_TABm_SP_TREE_PORT91f_GET(stg_tab); break;
    case 92: hw_state = STG_TABm_SP_TREE_PORT92f_GET(stg_tab); break;
    case 93: hw_state = STG_TABm_SP_TREE_PORT93f_GET(stg_tab); break;
    case 94: hw_state = STG_TABm_SP_TREE_PORT94f_GET(stg_tab); break;
    case 95: hw_state = STG_TABm_SP_TREE_PORT95f_GET(stg_tab); break;
    case 96: hw_state = STG_TABm_SP_TREE_PORT96f_GET(stg_tab); break;
    case 97: hw_state = STG_TABm_SP_TREE_PORT97f_GET(stg_tab); break;
    case 98: hw_state = STG_TABm_SP_TREE_PORT98f_GET(stg_tab); break;
    case 99: hw_state = STG_TABm_SP_TREE_PORT99f_GET(stg_tab); break;
    case 100: hw_state = STG_TABm_SP_TREE_PORT100f_GET(stg_tab); break;
    case 101: hw_state = STG_TABm_SP_TREE_PORT101f_GET(stg_tab); break;
    case 102: hw_state = STG_TABm_SP_TREE_PORT102f_GET(stg_tab); break;
    case 103: hw_state = STG_TABm_SP_TREE_PORT103f_GET(stg_tab); break;
    case 104: hw_state = STG_TABm_SP_TREE_PORT104f_GET(stg_tab); break;
    case 105: hw_state = STG_TABm_SP_TREE_PORT105f_GET(stg_tab); break;
    case 106: hw_state = STG_TABm_SP_TREE_PORT106f_GET(stg_tab); break;
    case 107: hw_state = STG_TABm_SP_TREE_PORT107f_GET(stg_tab); break;
    case 108: hw_state = STG_TABm_SP_TREE_PORT108f_GET(stg_tab); break;
    case 109: hw_state = STG_TABm_SP_TREE_PORT109f_GET(stg_tab); break;
    case 110: hw_state = STG_TABm_SP_TREE_PORT110f_GET(stg_tab); break;
    case 111: hw_state = STG_TABm_SP_TREE_PORT111f_GET(stg_tab); break;
    case 112: hw_state = STG_TABm_SP_TREE_PORT112f_GET(stg_tab); break;
    case 113: hw_state = STG_TABm_SP_TREE_PORT113f_GET(stg_tab); break;
    case 114: hw_state = STG_TABm_SP_TREE_PORT114f_GET(stg_tab); break;
    case 115: hw_state = STG_TABm_SP_TREE_PORT115f_GET(stg_tab); break;
    case 116: hw_state = STG_TABm_SP_TREE_PORT116f_GET(stg_tab); break;
    case 117: hw_state = STG_TABm_SP_TREE_PORT117f_GET(stg_tab); break;
    case 118: hw_state = STG_TABm_SP_TREE_PORT118f_GET(stg_tab); break;
    case 119: hw_state = STG_TABm_SP_TREE_PORT119f_GET(stg_tab); break;
    case 120: hw_state = STG_TABm_SP_TREE_PORT120f_GET(stg_tab); break;
    case 121: hw_state = STG_TABm_SP_TREE_PORT121f_GET(stg_tab); break;
    case 122: hw_state = STG_TABm_SP_TREE_PORT122f_GET(stg_tab); break;
    case 123: hw_state = STG_TABm_SP_TREE_PORT123f_GET(stg_tab); break;
    case 124: hw_state = STG_TABm_SP_TREE_PORT124f_GET(stg_tab); break;
    case 125: hw_state = STG_TABm_SP_TREE_PORT125f_GET(stg_tab); break;
    case 126: hw_state = STG_TABm_SP_TREE_PORT126f_GET(stg_tab); break;
    case 127: hw_state = STG_TABm_SP_TREE_PORT127f_GET(stg_tab); break;
    case 128: hw_state = STG_TABm_SP_TREE_PORT128f_GET(stg_tab); break;
    case 129: hw_state = STG_TABm_SP_TREE_PORT129f_GET(stg_tab); break;
    case 130: hw_state = STG_TABm_SP_TREE_PORT130f_GET(stg_tab); break;
    case 131: hw_state = STG_TABm_SP_TREE_PORT131f_GET(stg_tab); break;
    case 132: hw_state = STG_TABm_SP_TREE_PORT132f_GET(stg_tab); break;
    case 133: hw_state = STG_TABm_SP_TREE_PORT133f_GET(stg_tab); break;
    case 134: hw_state = STG_TABm_SP_TREE_PORT134f_GET(stg_tab); break;
    case 135: hw_state = STG_TABm_SP_TREE_PORT135f_GET(stg_tab); break;
    default:
        return ioerr ? CDK_E_IO : CDK_E_PORT;
    }

    if (CDK_FAILURE(bmd_xgsd_stp_state_from_hw(hw_state, state))) {
        /* should never fail */
        CDK_ASSERT(0);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;

}
#endif /* CDK_CONFIG_INCLUDE_BCM56960_A0 */

