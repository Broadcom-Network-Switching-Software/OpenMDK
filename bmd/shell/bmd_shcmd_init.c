/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Chip init command
 */

#include <cdk/cdk_shell.h>
#include <cdk/cdk_device.h>
#include <bmd/bmd.h>
#include <bmd/shell/shcmd_init.h>

#include "bmd_shell_util.h"

int 
bmd_shcmd_init(int argc, char* argv[])
{
    int unit;
    int rv;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);

    rv = bmd_init(unit);

    return cdk_shell_error(rv);
}
