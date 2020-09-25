/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Chip pmac command
 */

#include <bmd/shell/shcmd_port_mac.h>

#include "bmd_shell_util.h"

int 
bmd_shcmd_port_mac(int argc, char* argv[])
{
    int unit;
    int rv;
    int port;
    int vlan;
    bmd_mac_addr_t mac_addr;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);

    if (argc != 4) {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    port = CDK_STRTOUL(argv[1], NULL, 0);
    vlan = CDK_STRTOUL(argv[2], NULL, 0);

    if (bmd_shell_parse_mac_addr(argv[3], &mac_addr) < 0) {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    port = CDK_PORT_MAP_L2P(unit, port);

    if (CDK_STRCMP(argv[0], "add") == 0) {
        rv = bmd_port_mac_addr_add(unit, port, vlan, &mac_addr);
    } else if (CDK_STRCMP(argv[0], "remove") == 0) {
        rv = bmd_port_mac_addr_remove(unit, port, vlan, &mac_addr);
    } else {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    return cdk_shell_error(rv);
}
