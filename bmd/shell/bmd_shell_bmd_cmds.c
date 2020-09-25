/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * BMD API commands
 */

#include <bmd_config.h>
#include <bmd/shell/shcmd_reset.h>
#include <bmd/shell/shcmd_init.h>
#include <bmd/shell/shcmd_tx.h>
#include <bmd/shell/shcmd_rx.h>
#include <bmd/shell/shcmd_vlan.h>
#include <bmd/shell/shcmd_port_stp.h>
#include <bmd/shell/shcmd_port_mode.h>
#include <bmd/shell/shcmd_port_mac.h>
#include <bmd/shell/shcmd_port_vlan.h>
#include <bmd/shell/shcmd_cpu_mac.h>
#include <bmd/shell/shcmd_stat.h>
#include <bmd/shell/shcmd_switching_init.h>
#include <bmd/shell/shcmd_bmd.h>
#include <bmd/shell/shcmd_phy.h>
#include <bmd/shell/shcmd_pdl.h>
#include <bmd/shell/shcmd_port_info.h>
#include <bmd/shell/bmd_cmds.h>

#include <cdk/cdk_shell.h>

#if BMD_CONFIG_SHELL_INCLUDE_RESET == 1

static cdk_shell_command_t shcmd_reset = {
    "reset",
    bmd_shcmd_reset,
    BMD_SHCMD_RESET_DESC,
    BMD_SHCMD_RESET_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { BMD_SHCMD_RESET_HELP }
#endif
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_RESET */

#if BMD_CONFIG_SHELL_INCLUDE_INIT == 1

static cdk_shell_command_t shcmd_init = {
    "init",
    bmd_shcmd_init,
    BMD_SHCMD_INIT_DESC,
    BMD_SHCMD_INIT_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { BMD_SHCMD_INIT_HELP }
#endif
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_INIT */

#if BMD_CONFIG_SHELL_INCLUDE_TX == 1

static cdk_shell_command_t shcmd_tx = {
    "tx",
    bmd_shcmd_tx,
    BMD_SHCMD_TX_DESC,
    BMD_SHCMD_TX_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { BMD_SHCMD_TX_HELP,
      BMD_SHCMD_TX_HELP_2 }
#endif
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_TX */

#if BMD_CONFIG_SHELL_INCLUDE_RX == 1

static cdk_shell_command_t shcmd_rx = {
    "rx",
    bmd_shcmd_rx,
    BMD_SHCMD_RX_DESC,
    BMD_SHCMD_RX_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { BMD_SHCMD_RX_HELP }
#endif
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_RX */

#if BMD_CONFIG_SHELL_INCLUDE_VLAN == 1

static cdk_shell_command_t shcmd_vlan = {
    "vlan",
    bmd_shcmd_vlan,
    BMD_SHCMD_VLAN_DESC,
    BMD_SHCMD_VLAN_SYNOP,
#if CDK_CONFIG_SHELL_INCLUDE_HELP == 1
    { BMD_SHCMD_VLAN_HELP }
#endif
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_VLAN */

#if BMD_CONFIG_SHELL_INCLUDE_PORT_MODE == 1
static cdk_shell_command_t shcmd_port_mode = {
    "portmode",
    bmd_shcmd_port_mode,
    BMD_SHCMD_PORT_MODE_DESC,
    BMD_SHCMD_PORT_MODE_SYNOP,
    { BMD_SHCMD_PORT_MODE_HELP }
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_PORT_MODE */

#if BMD_CONFIG_SHELL_INCLUDE_PORT_STP == 1

static cdk_shell_command_t shcmd_port_stp = {
    "stp",
    bmd_shcmd_port_stp,
    BMD_SHCMD_PORT_STP_DESC,
    BMD_SHCMD_PORT_STP_SYNOP,
    { BMD_SHCMD_PORT_STP_HELP }
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_PORT_STP */

#if BMD_CONFIG_SHELL_INCLUDE_PORT_VLAN == 1

static cdk_shell_command_t shcmd_port_vlan = {
    "pvlan",
    bmd_shcmd_port_vlan,
    BMD_SHCMD_PORT_VLAN_DESC,
    BMD_SHCMD_PORT_VLAN_SYNOP,
    { BMD_SHCMD_PORT_VLAN_HELP }
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_PORT_VLAN */

#if BMD_CONFIG_SHELL_INCLUDE_PORT_MAC == 1

static cdk_shell_command_t shcmd_port_mac = {
    "pmac",
    bmd_shcmd_port_mac,
    BMD_SHCMD_PORT_MAC_DESC,
    BMD_SHCMD_PORT_MAC_SYNOP,
    { BMD_SHCMD_PORT_MAC_HELP }
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_PORT_MAC */

#if BMD_CONFIG_SHELL_INCLUDE_CPU_MAC == 1

static cdk_shell_command_t shcmd_cpu_mac = {
    "cpumac",
    bmd_shcmd_cpu_mac,
    BMD_SHCMD_CPU_MAC_DESC,
    BMD_SHCMD_CPU_MAC_SYNOP,
    { BMD_SHCMD_CPU_MAC_HELP }
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_CPU_MAC */

#if BMD_CONFIG_SHELL_INCLUDE_STAT == 1

static cdk_shell_command_t shcmd_stat = {
    "stat",
    bmd_shcmd_stat,
    BMD_SHCMD_STAT_DESC,
    BMD_SHCMD_STAT_SYNOP,
    { BMD_SHCMD_STAT_HELP }
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_STAT */

#if BMD_CONFIG_SHELL_INCLUDE_SWITCHING_INIT == 1

static cdk_shell_command_t shcmd_switching_init = {
    "swinit",
    bmd_shcmd_switching_init,
    BMD_SHCMD_SWITCHING_INIT_DESC,
    BMD_SHCMD_SWITCHING_INIT_SYNOP,
    { BMD_SHCMD_SWITCHING_INIT_HELP }
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_SWITCHING_INIT */

#if BMD_CONFIG_SHELL_INCLUDE_PHY == 1

static cdk_shell_command_t shcmd_phy = {
    "phy",
    bmd_shcmd_phy,
    BMD_SHCMD_PHY_DESC,
    BMD_SHCMD_PHY_SYNOP,
    { BMD_SHCMD_PHY_HELP }
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_PHY */

#if BMD_CONFIG_SHELL_INCLUDE_BMD == 1

static cdk_shell_command_t shcmd_bmd = {
    "bmd",
    bmd_shcmd_bmd,
    BMD_SHCMD_BMD_DESC,
    BMD_SHCMD_BMD_SYNOP,
    { BMD_SHCMD_BMD_HELP }
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_BMD */

#if BMD_CONFIG_SHELL_INCLUDE_PDL == 1

static cdk_shell_command_t shcmd_pdl = {
    "pdl",
    bmd_shcmd_pdl,
    BMD_SHCMD_PDL_DESC,
    BMD_SHCMD_PDL_SYNOP,
    { BMD_SHCMD_PDL_HELP }
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_PDL */

#if BMD_CONFIG_SHELL_INCLUDE_PORT_INFO == 1

static cdk_shell_command_t shcmd_port_info = {
    "pinfo",
    bmd_shcmd_port_info,
    BMD_SHCMD_PORT_INFO_DESC,
    BMD_SHCMD_PORT_INFO_SYNOP,
    { BMD_SHCMD_PORT_INFO_HELP }
};

#endif /* BMD_CONFIG_SHELL_INCLUDE_PORT_INFO */

void
bmd_shell_add_bmd_cmds(void)
{
#if BMD_CONFIG_SHELL_INCLUDE_PORT_INFO == 1
    cdk_shell_add_command(&shcmd_port_info, 0); 
#endif
#if BMD_CONFIG_SHELL_INCLUDE_PDL == 1
    cdk_shell_add_command(&shcmd_pdl, 0); 
#endif
#if BMD_CONFIG_SHELL_INCLUDE_PHY == 1
    cdk_shell_add_command(&shcmd_phy, 0); 
#endif
#if BMD_CONFIG_SHELL_INCLUDE_BMD == 1
    cdk_shell_add_command(&shcmd_bmd, 0); 
#endif
#if BMD_CONFIG_SHELL_INCLUDE_SWITCHING_INIT == 1
    cdk_shell_add_command(&shcmd_switching_init, 0);
#endif
#if BMD_CONFIG_SHELL_INCLUDE_STAT == 1
    cdk_shell_add_command(&shcmd_stat, 0);
#endif
#if BMD_CONFIG_SHELL_INCLUDE_CPU_MAC == 1
    cdk_shell_add_command(&shcmd_cpu_mac, 0);
#endif
#if BMD_CONFIG_SHELL_INCLUDE_PORT_MAC == 1
    cdk_shell_add_command(&shcmd_port_mac, 0);
#endif
#if BMD_CONFIG_SHELL_INCLUDE_RX == 1
    cdk_shell_add_command(&shcmd_rx, 0);
#endif
#if BMD_CONFIG_SHELL_INCLUDE_TX == 1
    cdk_shell_add_command(&shcmd_tx, 0);
#endif
#if BMD_CONFIG_SHELL_INCLUDE_PORT_MODE == 1
    cdk_shell_add_command(&shcmd_port_mode, 0);
#endif
#if BMD_CONFIG_SHELL_INCLUDE_PORT_STP == 1
    cdk_shell_add_command(&shcmd_port_stp, 0);
#endif
#if BMD_CONFIG_SHELL_INCLUDE_PORT_VLAN == 1
    cdk_shell_add_command(&shcmd_port_vlan, 0);
#endif
#if BMD_CONFIG_SHELL_INCLUDE_VLAN == 1
    cdk_shell_add_command(&shcmd_vlan, 0);
#endif
#if BMD_CONFIG_SHELL_INCLUDE_INIT == 1
    cdk_shell_add_command(&shcmd_init, 0);
#endif
#if BMD_CONFIG_SHELL_INCLUDE_RESET == 1
    cdk_shell_add_command(&shcmd_reset, 0);
#endif
}
