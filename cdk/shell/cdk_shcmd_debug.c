/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK debug control command
 */

#include <cdk/cdk_shell.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_printf.h>
#include <cdk/cdk_debug.h>
#include <cdk/shell/shcmd_debug.h>

#if CDK_CONFIG_SHELL_INCLUDE_DEBUG == 1

int 
cdk_shcmd_debug(int argc, char *argv[])
{
#if CDK_CONFIG_INCLUDE_DEBUG == 1
    static char *dbg_names[] = { CDK_DBG_NAMES };
    int i, addopt;
    char *opt;
    uint32_t mask;

    /* Ignore unit arg if present */
    /* coverity[unchecked_value] */
    cdk_shell_unit_arg_extract(&argc, argv, 1);

    /* Add/delete debug options */
    while (argc) {
        opt = *argv;
        addopt = 1;
        if (opt[0] == '-' || opt[0] == '!') {
            addopt = 0;
            opt++;
        } else if (opt[0] == '+') {
            opt++;
        }
        mask = 0;
        for (i = 0; i < COUNTOF(dbg_names); i++) {
            if (*opt == 0 || CDK_STRNCMP(opt, dbg_names[i], CDK_STRLEN(opt)) == 0) {
                mask |= (1 << i);
            }
        }
        if (mask ==  0) {
            return CDK_SHELL_CMD_BAD_ARG;
        }
        if (addopt) {
            cdk_debug_level |= mask;
        } else {
            cdk_debug_level &= ~mask;
        }
        argv++;
        argc--;
    }

    /* Show current/new configuration */
    CDK_PRINTF("Enabled:  ");
    for (i = 0; i < COUNTOF(dbg_names); i++) {
        if (cdk_debug_level & (1 << i)) {
            CDK_PRINTF("%s ", dbg_names[i]);
        }
    }
    CDK_PRINTF("\nDisabled: ");
    for (i = 0; i < COUNTOF(dbg_names); i++) {
        if (!(cdk_debug_level & (1 << i))) {
            CDK_PRINTF("%s ", dbg_names[i]);
        }
    }
    CDK_PRINTF("\n");
#endif /* CDK_CONFIG_INCLUDE_DEBUG */

    return 0;
}

#endif /* CDK_CONFIG_SHELL_INCLUDE_DEBUG */
