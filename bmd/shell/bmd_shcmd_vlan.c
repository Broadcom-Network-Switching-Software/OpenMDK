/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Chip packet vlan command
 */

#include <cdk/cdk_shell.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_stdlib.h>
#include <cdk/cdk_printf.h>
#include <cdk/cdk_debug.h>

#include <bmd/bmd.h>
#include <bmd/shell/shcmd_vlan.h>

#include "bmd_shell_util.h"

static int 
_vlan_create(int argc, char *argv[])
{
    int unit;
    int vlan;
    int rv = CDK_E_NONE;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);

    if (argc == 1) {
        vlan = CDK_STRTOUL(argv[0], NULL, 0);
        rv = bmd_vlan_create(unit, vlan);
    } else {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    return cdk_shell_error(rv);
}

static int 
_vlan_destroy(int argc, char *argv[])
{
    int unit;
    int vlan;
    int rv = CDK_E_NONE;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);

    if (argc == 1) {
        vlan = CDK_STRTOUL(argv[0], NULL, 0);
        rv = bmd_vlan_destroy(unit, vlan);
    } else {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    return cdk_shell_error(rv);
}

static int 
_vlan_port_add(int argc, char *argv[])
{
    int unit;
    cdk_pbmp_t pbmp;
    int vlan = -1;
    int lport, port = -1;
    uint32_t flags = 0;
    int ax;
    int rv = CDK_E_NONE;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);

    for (ax = 0; ax < argc; ax++) {
        if (CDK_STRCMP(argv[ax], "untag") == 0) {
            flags |= BMD_VLAN_PORT_F_UNTAGGED;
        } else if (vlan < 0) {
            vlan = CDK_STRTOL(argv[ax], NULL, 0);
        } else if (port < 0) {
            port = bmd_shell_parse_port_str(unit, argv[ax], &pbmp);
        } else {
            return CDK_SHELL_CMD_BAD_ARG;
        }
    }

    if (vlan < 0 || port < 0) {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    CDK_LPORT_ITER(unit, pbmp, lport, port) {
        rv = bmd_vlan_port_add(unit, vlan, port, flags);
        if (CDK_FAILURE(rv)) {
            break;
        }
    }

    return cdk_shell_error(rv);
}

static int 
_vlan_port_remove(int argc, char *argv[])
{
    int unit;
    cdk_pbmp_t pbmp;
    int vlan = -1;
    int lport, port = -1;
    int ax;
    int rv = CDK_E_NONE;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);

    for (ax = 0; ax < argc; ax++) {
        if (vlan < 0) {
            vlan = CDK_STRTOL(argv[ax], NULL, 0);
        } else if (port < 0) {
            port = bmd_shell_parse_port_str(unit, argv[ax], &pbmp);
        } else {
            return CDK_SHELL_CMD_BAD_ARG;
        }
    }

    if (vlan < 0 || port < 0) {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    CDK_LPORT_ITER(unit, pbmp, lport, port) {
        rv = bmd_vlan_port_remove(unit, vlan, port);
        if (CDK_FAILURE(rv)) {
            break;
        }
    }

    return cdk_shell_error(rv);
}

static void
_port_list_map_p2l(int unit, int *list)
{
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    int cnt, idx, tmp;

    for (idx = 0; idx < BMD_CONFIG_MAX_PORTS && list[idx] >= 0; idx++) {
        list[idx] = CDK_PORT_MAP_P2L(unit, list[idx]);
    }
    for (cnt  = 0; cnt < BMD_CONFIG_MAX_PORTS; cnt++) {
        for (idx = 1; idx < BMD_CONFIG_MAX_PORTS && list[idx] >= 0; idx++) {
            if (list[idx-1] > list[idx]) {
                tmp = list[idx];
                list[idx] = list[idx-1];
                list[idx-1] = tmp;
            }
        }
    }
#endif
}

static void
_show_port_list(int *list)
{
    int idx;

    if (list[0] < 0) {
        CDK_PRINTF("none ");
        return;
    }
    for (idx  = 0; idx < BMD_CONFIG_MAX_PORTS && list[idx] >= 0; idx++) {
        if (list[idx+1] == list[idx] + 1) {
            CDK_PRINTF("%d-", list[idx]);
            while (list[idx+1] == list[idx] + 1) {
                idx++;
            }
        }
        CDK_PRINTF("%d ", list[idx]);
    }
}

static int 
_vlan_show(int argc, char *argv[])
{
    int unit;
    int vlan, vlan_max;
    int plist[BMD_CONFIG_MAX_PORTS+1];
    int utlist[BMD_CONFIG_MAX_PORTS+1];
    int rv;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);

    if (argc <= 1) {
        if (argc) {
            vlan = CDK_STRTOL(argv[0], NULL, 0);
            vlan_max = vlan;
        }
        else {
            vlan = 0;
            vlan_max = 4095;
        }
        for (; vlan <= vlan_max; vlan++) {
            rv = bmd_vlan_port_get(unit, vlan, plist, utlist);
            if (CDK_SUCCESS(rv)) {
                CDK_PRINTF("VLAN %d ports: ", vlan);
                _port_list_map_p2l(unit, plist);
                _show_port_list(plist);
                CDK_PRINTF("  untagged: ");
                _port_list_map_p2l(unit, utlist);
                _show_port_list(utlist);
                CDK_PRINTF("\n");
            }
        }
    } else {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    return CDK_SHELL_CMD_OK;
}

int 
bmd_shcmd_vlan(int argc, char *argv[])
{
    char *action;

    if (argc == 0) {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    action = argv[0];
    argc--;
    argv++;

    if (CDK_STRCMP(action, "create") == 0) {
        return _vlan_create(argc, argv);
    } else if (CDK_STRCMP(action, "destroy") == 0) {
        return _vlan_destroy(argc, argv);
    } else if (CDK_STRCMP(action, "add") == 0) {
        return _vlan_port_add(argc, argv);
    } else if (CDK_STRCMP(action, "remove") == 0) {
        return _vlan_port_remove(argc, argv);
    } else if (CDK_STRCMP(action, "show") == 0) {
        return _vlan_show(argc, argv);
    }

    return CDK_SHELL_CMD_BAD_ARG;
}
