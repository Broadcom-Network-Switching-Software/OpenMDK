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
#include <cdk/shell/shcmd_cdk.h>
#include <cdk/cdk_string.h>

#if CDK_CONFIG_SHELL_INCLUDE_CDK == 1

/*
 * Print out the current build configuration
 */
int     
cdk_shcmd_cdk(int argc, char *argv[])
{
    const char *vstr = "";

    COMPILER_REFERENCE(argc);
    COMPILER_REFERENCE(argv);

    CDK_PRINTF("The CDK package was configured as follows:\n"); 
#define CONFIG_OPTION(var) \
    vstr = #var; \
    if (cdk_shell_option_filter(argc, argv, vstr, var) == 0) { \
        CDK_PRINTF("    %s = %"PRIu32"\n", vstr, (uint32_t)var); \
    }
#include <cdk_config.h>
    return 0; 
}

#endif /* CDK_CONFIG_SHELL_INCLUDE_CDK */
