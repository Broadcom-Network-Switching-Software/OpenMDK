/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * PHY Driver Libarary Configuration.
 */

#include <cdk/cdk_printf.h>
#include <cdk/cdk_shell.h>

#include <bmd/bmd.h>
#include <bmd/shell/shcmd_pdl.h>

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy.h>

#endif /* BMD_CONFIG_INCLUDE_PHY */

int 
bmd_shcmd_pdl(int argc, char* argv[])
{
#if BMD_CONFIG_INCLUDE_PHY == 1

    CDK_PRINTF("The PHY package was configured with the following options:\n"); 
#define CONFIG_OPTION(var) \
    if (cdk_shell_option_filter(argc, argv, #var, var) == 0) { \
        CDK_PRINTF("    %s = %"PRIu32"\n", #var, (uint32_t)var); \
    }
#include <phy_config.h>
#undef CONFIG_OPTION

#else
    CDK_PRINTF("No PHY support.\n"); 
#endif /* BMD_CONFIG_INCLUDE_PHY */
    
    return CDK_SHELL_CMD_OK; 
}
