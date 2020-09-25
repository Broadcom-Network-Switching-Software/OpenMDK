/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS shell command UNIT
 */

#include <cdk/arch/xgs_shell.h>

#include <cdk/arch/shcmd_xgs_unit.h>

#if CDK_CONFIG_SHELL_INCLUDE_UNIT == 1

/*
 * Print out the current switch unit information
 */
int
cdk_shcmd_xgs_unit(int argc, char *argv[])
{
    int unit;
    int u, b; 
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
            if ((CDK_DEV_FLAGS(u) & CDK_DEV_ARCH_XGS) == 0) {
                CDK_PRINTF("  Not XGS architecture\n"); 
                continue;
            }
            CDK_PRINTF("  Device: %s [%04x:%04x:%02x]\n",
                       CDK_DEV(u)->name, CDK_DEV(u)->id.vendor_id,
                       CDK_DEV(u)->id.device_id, CDK_DEV(u)->id.revision); 
            CDK_PRINTF("  Base Address: %p\n", (void*)CDK_DEV_BASE_ADDR(u)); 
            cdk_shell_port_bitmap(tmp, sizeof(tmp),
                                  &CDK_DEV(u)->valid_pbmps,
                                  &CDK_XGS_INFO(u)->valid_pbmps);
            CDK_PRINTF("  Valid Ports:         %s\n", tmp); 
            CDK_PRINTF("  Flags: 0x%"PRIx32"\n", CDK_XGS_INFO(u)->flags); 
            if (unit == -1) {
                continue;
            }
            CDK_PRINTF("  Block  Name          Ports\n"); 
            for (b = 0; b < CDK_XGS_INFO(u)->nblocks; b++) {  
                const cdk_xgs_block_t *blkp = CDK_XGS_INFO(u)->blocks+b; 
                if (cdk_xgs_shell_block_name(u, blkp->blknum, tmp)) {
                    CDK_PRINTF("  %-2d     %-12s  ", blkp->blknum, tmp); 
                    cdk_shell_port_bitmap(tmp, sizeof(tmp),
                                          &blkp->pbmps,
                                          &CDK_XGS_INFO(u)->valid_pbmps);
                    CDK_PRINTF("%s\n", tmp);
                }
            }
        }
    }
    return 0; 
}

#endif /* CDK_CONFIG_SHELL_INCLUDE_UNIT */
