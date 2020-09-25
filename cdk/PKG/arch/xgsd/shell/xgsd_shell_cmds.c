/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGSD core commands
 */

#include <cdk/cdk_shell.h>
#include <cdk/cdk_device.h>

#include <cdk/arch/shcmd_xgsd_get.h>
#include <cdk/arch/shcmd_xgsd_set.h>
#include <cdk/arch/shcmd_xgsd_geti.h>
#include <cdk/arch/shcmd_xgsd_seti.h>
#include <cdk/arch/shcmd_xgsd_list.h>
#include <cdk/arch/shcmd_xgsd_unit.h>
#include <cdk/arch/shcmd_xgsd_pid.h>
#include <cdk/arch/xgsd_cmds.h>

#if CDK_CONFIG_SHELL_INCLUDE_GET == 1
static cdk_shell_command_t shcmd_get = {
    "get",
    cdk_shcmd_xgsd_get,
    CDK_SHCMD_XGSD_GET_DESC,
    CDK_SHCMD_XGSD_GET_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { CDK_SHCMD_XGSD_GET_HELP, CDK_SHCMD_XGSD_GET_HELP_2 }
#endif
};
#endif /* CDK_CONFIG_SHELL_INCLUDE_GET */

#if CDK_CONFIG_SHELL_INCLUDE_SET == 1
static cdk_shell_command_t shcmd_set = {
    "set",
    cdk_shcmd_xgsd_set,
    CDK_SHCMD_XGSD_SET_DESC,
    CDK_SHCMD_XGSD_SET_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { CDK_SHCMD_XGSD_SET_HELP }
#endif
};
#endif /* CDK_CONFIG_SHELL_INCLUDE_SET */

#if CDK_CONFIG_SHELL_INCLUDE_GETI == 1
static cdk_shell_command_t shcmd_geti = {
    "geti",
    cdk_shcmd_xgsd_geti,
    CDK_SHCMD_XGSD_GETI_DESC,
    CDK_SHCMD_XGSD_GETI_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { CDK_SHCMD_XGSD_GETI_HELP }
#endif
};
#endif /* CDK_CONFIG_SHELL_INCLUDE_GETI */

#if CDK_CONFIG_SHELL_INCLUDE_SETI == 1
static cdk_shell_command_t shcmd_seti = {
    "seti",
    cdk_shcmd_xgsd_seti,
    CDK_SHCMD_XGSD_SETI_DESC,
    CDK_SHCMD_XGSD_SETI_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { CDK_SHCMD_XGSD_SETI_HELP }
#endif
};
#endif /* CDK_CONFIG_SHELL_INCLUDE_SETI */

#if CDK_CONFIG_SHELL_INCLUDE_LIST == 1
static cdk_shell_command_t shcmd_list = {
    "list",
    cdk_shcmd_xgsd_list,
    CDK_SHCMD_XGSD_LIST_DESC,
    CDK_SHCMD_XGSD_LIST_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { CDK_SHCMD_XGSD_LIST_HELP }
#endif
};
#endif /* CDK_CONFIG_SHELL_INCLUDE_LIST */

#if CDK_CONFIG_SHELL_INCLUDE_UNIT == 1
static cdk_shell_command_t shcmd_unit = {
    "unit",
    cdk_shcmd_xgsd_unit,
    CDK_SHCMD_XGSD_UNIT_DESC,
    CDK_SHCMD_XGSD_UNIT_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { CDK_SHCMD_XGSD_UNIT_HELP }
#endif
};
#endif /* CDK_CONFIG_SHELL_INCLUDE_UNIT */

#if CDK_CONFIG_SHELL_INCLUDE_PID == 1
static cdk_shell_command_t shcmd_pid = {
    "pid",
    cdk_shcmd_xgsd_pid,
    CDK_SHCMD_XGSD_PID_DESC,
    CDK_SHCMD_XGSD_PID_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { CDK_SHCMD_XGSD_PID_HELP }
#endif
};
#endif /* CDK_CONFIG_SHELL_INCLUDE_PID */

void
cdk_shell_add_xgsd_core_cmds(void)
{
#if CDK_CONFIG_SHELL_INCLUDE_GET == 1
    cdk_shell_add_command(&shcmd_get, CDK_DEV_ARCH_XGSD);
#endif
#if CDK_CONFIG_SHELL_INCLUDE_SET == 1
    cdk_shell_add_command(&shcmd_set, CDK_DEV_ARCH_XGSD);
#endif
#if CDK_CONFIG_SHELL_INCLUDE_GETI == 1
    cdk_shell_add_command(&shcmd_geti, CDK_DEV_ARCH_XGSD);
#endif
#if CDK_CONFIG_SHELL_INCLUDE_SETI == 1
    cdk_shell_add_command(&shcmd_seti, CDK_DEV_ARCH_XGSD);
#endif
#if CDK_CONFIG_SHELL_INCLUDE_LIST == 1
    cdk_shell_add_command(&shcmd_list, CDK_DEV_ARCH_XGSD);
#endif
#if CDK_CONFIG_SHELL_INCLUDE_UNIT == 1
    cdk_shell_add_command(&shcmd_unit, CDK_DEV_ARCH_XGSD);
#endif
#if CDK_CONFIG_SHELL_INCLUDE_PID == 1
    cdk_shell_add_command(&shcmd_pid, CDK_DEV_ARCH_XGSD);
#endif
}
