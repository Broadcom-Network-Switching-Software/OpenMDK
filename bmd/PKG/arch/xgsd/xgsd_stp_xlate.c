/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#ifdef CDK_CONFIG_ARCH_XGSD_INSTALLED

#include <bmdi/arch/xgsd_stp_xlate.h>

#include <cdk/cdk_error.h>

int 
bmd_xgsd_stp_state_to_hw(bmd_stp_state_t bmd_state, int *hw_state)
{
    switch (bmd_state) {
    case bmdSpanningTreeDisabled:
        *hw_state = 0;
        break;
    case bmdSpanningTreeBlocking:
    case bmdSpanningTreeListening:
        *hw_state = 1;
        break;
    case bmdSpanningTreeLearning:
        *hw_state = 2;
        break;
    case bmdSpanningTreeForwarding:
        *hw_state = 3;
        break;
    default:
        return CDK_E_PARAM;
    }
    return CDK_E_NONE;
}

int 
bmd_xgsd_stp_state_from_hw(int hw_state, bmd_stp_state_t *bmd_state)
{
    switch (hw_state) {
    case 0:
        *bmd_state = bmdSpanningTreeDisabled;
        break;
    case 1:
        *bmd_state = bmdSpanningTreeBlocking;
        break;
    case 2:
        *bmd_state = bmdSpanningTreeLearning;
        break;
    case 3:
        *bmd_state = bmdSpanningTreeForwarding;
        break;
    default:
        return CDK_E_PARAM;
    }
    return CDK_E_NONE;
}

#endif /* CDK_CONFIG_ARCH_XGSD_INSTALLED */
