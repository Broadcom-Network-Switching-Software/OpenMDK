/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Create VLAN command
 */

#include <cdk/cdk_shell.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_stdlib.h>
#include <cdk/cdk_printf.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include <bmd/bmd.h>
#include <bmd/shell/shcmd_port_vlan.h>

#include "bmd_shell_util.h"

int 
bmd_shcmd_port_vlan(int argc, char *argv[])
{
    int unit;
    cdk_pbmp_t pbmp;
    int lport, port;
    int vlan;
    int rv = CDK_E_NONE;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);

    if (argc == 0) {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    port = bmd_shell_parse_port_str(unit, argv[0], &pbmp);

    if (port < 0) {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    if (argc == 1) {
        CDK_LPORT_ITER(unit, pbmp, lport, port) {
            rv = bmd_port_vlan_get(unit, port, &vlan);
            if (CDK_FAILURE(rv)) {
                break;
            }
            CDK_PRINTF("Port %d: Default VLAN: %d\n", lport, vlan);
        }
    } else if (argc == 2) {
        vlan = CDK_STRTOL(argv[1], NULL, 0);
        CDK_LPORT_ITER(unit, pbmp, lport, port) {
            rv = bmd_port_vlan_set(unit, port, vlan);
            if (CDK_FAILURE(rv)) {
                break;
            }
        }
    } else {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    return cdk_shell_error(rv);
}
