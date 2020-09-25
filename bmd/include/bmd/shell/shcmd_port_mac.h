/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_port_mac(int argc, char *argv[]);

#define BMD_SHCMD_PORT_MAC_DESC "Add or remove port MAC address"
#define BMD_SHCMD_PORT_MAC_SYNOP \
"action port vlan mac-addr"
#define BMD_SHCMD_PORT_MAC_HELP \
"Add or remove MAC address to/from the L2 address resolution\n" \
"table. Adding a MAC address prevents packets from being flooded\n" \
"to all switch ports. Valid actions are add and remove. The MAC\n" \
"address should be specified as xx:xx:xx:xx:xx:xx."
