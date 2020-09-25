/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default CDK shell unit get/set function.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_shell.h>

static int current_unit = -1;

int
cdk_shell_unit_get(void)
{
    return current_unit;
}

int
cdk_shell_unit_set(int unit)
{
    if (!CDK_DEV_EXISTS(unit)) {
        return -1;
    }
    current_unit = unit;
    return 0;
}

int
cdk_shell_unit_flags_get(int unit, uint32_t *dev_flags)
{
    if (!dev_flags) {
        return -1;
    }
    if (!CDK_DEV_EXISTS(unit)) {
        *dev_flags = 0;
        return -1;
    }
    *dev_flags = CDK_DEV(unit)->flags;
    return 0;
}
