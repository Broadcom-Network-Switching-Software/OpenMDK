/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK configuration command
 */

#include <cdk/cdk_shell.h>
#include <cdk/cdk_printf.h>
#include <bmd_config.h>
#include <bmd/shell/shcmd_bmd.h>
#include <cdk/cdk_string.h>

#if BMD_CONFIG_SHELL_INCLUDE_BMD == 1

/*
 * Print out the current BMD build configuration
 */

int     
bmd_shcmd_bmd(int argc, char *argv[])
{
    CDK_PRINTF("The BMD package was configured with the following options:\n"); 
#define CONFIG_OPTION(var) \
    if (cdk_shell_option_filter(argc, argv, #var, var) == 0) { \
        CDK_PRINTF("    %s = %"PRIu32"\n", #var, (uint32_t)var); \
    }
#include <bmd_config.h>
#undef CONFIG_OPTION

    CDK_PRINTF("The BMD package was configured for the following base drivers:\n"); 
#define BMD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_fl,_cn,_pf,_pd,_r0,_r1) \
    if (cdk_shell_option_filter(argc, argv, #_bd, 1) == 0) { \
        CDK_PRINTF("    %s\n", #_bd); \
    }
#include <bmdi/bmd_devlist.h>

    return CDK_SHELL_CMD_OK; 
}

#endif /* BMD_CONFIG_SHELL_INCLUDE_BMD */
