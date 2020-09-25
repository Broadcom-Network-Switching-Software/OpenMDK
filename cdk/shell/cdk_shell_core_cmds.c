/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK core commands
 *
 * This file also serves as a template for installing additional
 * shell commands.
 *
 * Note that the command structures are linked directly into the
 * command table, and will they will be modified by the shell, 
 * hence they cannot be declared as type const.
 */

#include <cdk/cdk_shell.h>

#include <cdk/shell/shcmd_quit.h>
#include <cdk/shell/shcmd_help.h>
#include <cdk/shell/shcmd_cdk.h>
#include <cdk/shell/shcmd_debug.h>

static cdk_shell_command_t shcmd_quit = {
    "quit",
    cdk_shcmd_quit,
    CDK_SHCMD_QUIT_DESC,
};

static cdk_shell_command_t shcmd_exit = {
    "exit",
    cdk_shcmd_quit,
    CDK_SHCMD_QUIT_DESC,
};

static cdk_shell_command_t shcmd_help = {
    "help",
    cdk_shcmd_help,
    CDK_SHCMD_HELP_DESC,
    CDK_SHCMD_HELP_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { CDK_SHCMD_HELP_HELP }
#endif
};

#if CDK_CONFIG_SHELL_INCLUDE_CDK == 1
static cdk_shell_command_t shcmd_cdk = {
    "cdk",
    cdk_shcmd_cdk,
    CDK_SHCMD_CDK_DESC,
    CDK_SHCMD_CDK_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { CDK_SHCMD_CDK_HELP }
#endif
};
#endif /* CDK_CONFIG_SHELL_INCLUDE_CDK */

#if CDK_CONFIG_SHELL_INCLUDE_DEBUG == 1
static cdk_shell_command_t shcmd_debug = {
    "debug",
    cdk_shcmd_debug,
    CDK_SHCMD_DEBUG_DESC,
    CDK_SHCMD_DEBUG_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { CDK_SHCMD_DEBUG_HELP }
#endif
};
#endif /* CDK_CONFIG_SHELL_INCLUDE_DEBUG */

void
cdk_shell_add_core_cmds(void)
{
    cdk_shell_add_command(&shcmd_help, 0);
    cdk_shell_add_command(&shcmd_quit, 0);
    cdk_shell_add_command(&shcmd_exit, 0);
#if CDK_CONFIG_SHELL_INCLUDE_CDK == 1
    cdk_shell_add_command(&shcmd_cdk, 0);
#endif /* CDK_CONFIG_SHELL_INCLUDE_CDK */
#if CDK_CONFIG_SHELL_INCLUDE_DEBUG == 1
    cdk_shell_add_command(&shcmd_debug, 0);
#endif /* CDK_CONFIG_SHELL_INCLUDE_DEBUG */
}
