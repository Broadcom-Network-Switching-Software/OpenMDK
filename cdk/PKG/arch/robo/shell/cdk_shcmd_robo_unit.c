/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * ROBO shell command UNIT
 */

#include <cdk/arch/robo_shell.h>

#include <cdk/arch/shcmd_robo_unit.h>

#if CDK_CONFIG_SHELL_INCLUDE_UNIT == 1

/*
 * Print out the current switch unit information
 */
int
cdk_shcmd_robo_unit(int argc, char *argv[])
{
    int unit;
    int u; 
    char tmp[CDK_PBMP_WORD_MAX * 16]; 

    unit = cdk_shell_unit_arg_extract(&argc, argv, 1);
    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_SHELL_CMD_BAD_ARG;
    }

    if (argc) {
        /* Either a unit number or 'all', '*', or unit number */
        if (CDK_STRCMP(argv[0], "all") == 0 || *argv[0] == '*') {
            /* Specify all units */
            unit = -1; 
        }
    }

    for (u = 0; u < CDK_CONFIG_MAX_UNITS; u++) {
        if (CDK_DEV_EXISTS(u) && (unit == -1 || unit == u)) {
            CDK_PRINTF("unit %d:\n", u); 
            if ((CDK_DEV_FLAGS(u) & CDK_DEV_ARCH_ROBO) == 0) {
                CDK_PRINTF("  Not ROBO architecture\n"); 
                continue;
            }
            CDK_PRINTF("  Device: %s [%04x:%04x:%04x:%02x]\n",
                       CDK_DEV(u)->name, CDK_DEV(u)->id.vendor_id,
                       CDK_DEV(u)->id.device_id, CDK_DEV(u)->id.model,
                       CDK_DEV(u)->id.revision); 
            CDK_PRINTF("  rev. 0x%x\n", CDK_DEV(u)->id.revision); 
            cdk_shell_port_bitmap(tmp, sizeof(tmp),
                                  &CDK_DEV(u)->valid_pbmps,
                                  &CDK_ROBO_INFO(u)->valid_pbmps);
            CDK_PRINTF("  Valid Ports:         %s\n", tmp); 
            CDK_PRINTF("  Flags: 0x%"PRIx32"\n", CDK_ROBO_INFO(u)->flags); 
        }
    }
    return 0; 
}

#endif /* CDK_CONFIG_SHELL_INCLUDE_UNITS */
