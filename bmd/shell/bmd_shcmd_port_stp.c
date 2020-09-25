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
#include <bmd/shell/shcmd_port_stp.h>

#include "bmd_shell_util.h"

static struct {
    bmd_stp_state_t stp_state;
    char *state_str;
} _stp_tbl[] = {
    { bmdSpanningTreeDisabled,   "disable" },
    { bmdSpanningTreeBlocking,   "block"   },
    { bmdSpanningTreeListening,  "listen"  },
    { bmdSpanningTreeLearning,   "learn"   },
    { bmdSpanningTreeForwarding, "forward" }
};

int 
bmd_shcmd_port_stp(int argc, char *argv[])
{
    int unit;
    cdk_pbmp_t pbmp;
    int lport, port;
    int idx;
    int found;
    bmd_stp_state_t stp_state;
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
            rv = bmd_port_stp_get(unit, port, &stp_state);
            if (CDK_FAILURE(rv)) {
                break;
            }
            found = 0;
            for (idx = 0; idx < COUNTOF(_stp_tbl); idx++) {
                if (stp_state == _stp_tbl[idx].stp_state) {
                    found = 1;
                    break;
                }
            }
            if (found ) {
                CDK_PRINTF("Port %d, STP state: %s\n",
                           lport,  _stp_tbl[idx].state_str);
            } else {
                return CDK_SHELL_CMD_ERROR;
            }
        }
    } else if (argc == 2) {
        CDK_LPORT_ITER(unit, pbmp, lport, port) {
            found = 0;
            for (idx = 0; idx < COUNTOF(_stp_tbl); idx++) {
                if (CDK_STRCMP(argv[1], _stp_tbl[idx].state_str) == 0) {
                    found = 1;
                    break;
                }
            }
            if (found ) {
                rv = bmd_port_stp_set(unit, port, _stp_tbl[idx].stp_state);
                if (CDK_FAILURE(rv)) {
                    break;
                }
            } else {
                return CDK_SHELL_CMD_BAD_ARG;
            }
        }
    } else {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    return cdk_shell_error(rv);
}
