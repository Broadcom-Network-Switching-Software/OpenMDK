/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Chip cpumac command
 */

#include <bmd/shell/shcmd_cpu_mac.h>

#include "bmd_shell_util.h"

int 
bmd_shcmd_cpu_mac(int argc, char* argv[])
{
    int unit;
    int rv;
    int vlan;
    bmd_mac_addr_t mac_addr;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);

    if (argc != 3) {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    vlan = CDK_STRTOUL(argv[1], NULL, 0);

    if (bmd_shell_parse_mac_addr(argv[2], &mac_addr) < 0) {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    if (CDK_STRCMP(argv[0], "add") == 0) {
        rv = bmd_cpu_mac_addr_add(unit, vlan, &mac_addr);
    } else if (CDK_STRCMP(argv[0], "remove") == 0) {
        rv = bmd_cpu_mac_addr_remove(unit, vlan, &mac_addr);
    } else {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    return cdk_shell_error(rv);
}
