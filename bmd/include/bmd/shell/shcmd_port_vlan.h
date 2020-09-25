/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_port_vlan(int argc, char *argv[]);

#define BMD_SHCMD_PORT_VLAN_DESC "Set or get default VLAN for a port"
#define BMD_SHCMD_PORT_VLAN_SYNOP "ports [vlan]"
#define BMD_SHCMD_PORT_VLAN_HELP \
"If vlan is not specified the current default VLAN for the port(s)\n" \
"will be displayed."
