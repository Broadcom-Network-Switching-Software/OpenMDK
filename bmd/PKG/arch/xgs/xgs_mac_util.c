/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#ifdef CDK_CONFIG_ARCH_XGS_INSTALLED

#include <bmdi/arch/xgs_mac_util.h>

void
xgs_mac_to_field_val(const uint8_t *mac, uint32_t *fval)
{
    fval[0] = LSHIFT32(mac[2], 24) | LSHIFT32(mac[3],16) | LSHIFT32(mac[4], 8) | mac[5];
    fval[1] = LSHIFT32(mac[0], 8) | mac[1];
}

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
