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
#include <bmd/shell/shcmd_stat.h>

#include "bmd_shell_util.h"

static struct {
    bmd_stat_t  stat;
    char        *name;
} _stat_ctrs[] = {
    { bmdStatTxPackets,         "Tx packets" },
    { bmdStatTxBytes,           "Tx bytes"   },
    { bmdStatTxErrors,          "Tx errors"  },
    { bmdStatRxPackets,         "Rx packets" },
    { bmdStatRxBytes,           "Rx bytes"   },
    { bmdStatRxErrors,          "Rx errors"  },
    { bmdStatRxDrops,           "Rx drops"   }
};

static int
_clear_stat(int unit, int port)
{
    int rv = CDK_E_NONE;
    int idx;

    for (idx = 0; idx < COUNTOF(_stat_ctrs); idx++) {
        rv = bmd_stat_clear(unit, port, _stat_ctrs[idx].stat);
        if (CDK_FAILURE(rv)) {
            break;
        }
    }
    return rv;
}

static int
_show_stat(int unit, int port)
{
    bmd_counter_t counter;
    int rv = CDK_E_NONE;
    int idx;

    CDK_PRINTF("Port %d statistics:\n", CDK_PORT_MAP_P2L(unit, port));
    for (idx = 0; idx < COUNTOF(_stat_ctrs); idx++) {
        rv = bmd_stat_get(unit, port, _stat_ctrs[idx].stat, &counter);
        if (CDK_FAILURE(rv)) {
            break;
        }
        CDK_PRINTF("%-16s %10"PRIu32"\n", _stat_ctrs[idx].name, counter.v[0]);
    }
    return rv;
}

int 
bmd_shcmd_stat(int argc, char *argv[])
{
    int unit;
    cdk_pbmp_t pbmp;
    int lport, port = -1;
    int clear = 0;
    int ax;
    int rv = CDK_E_NONE;

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);

    for (ax = 0; ax < argc; ax++) {
        if (CDK_STRCMP(argv[ax], "clear") == 0) {
            clear = 1;
        } else if (port < 0) {
            port = bmd_shell_parse_port_str(unit, argv[ax], &pbmp);
        } else {
            return CDK_SHELL_CMD_BAD_ARG;
        }
    }

    if (port < 0) {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    if (clear) {
        CDK_LPORT_ITER(unit, pbmp, lport, port) {
            rv = _clear_stat(unit, port);
            if (CDK_FAILURE(rv)) {
                break;
            }
        }
    } else {
        CDK_LPORT_ITER(unit, pbmp, lport, port) {
            rv = _show_stat(unit, port);
            if (CDK_FAILURE(rv)) {
                break;
            }
        }
    }

    return cdk_shell_error(rv);
}
