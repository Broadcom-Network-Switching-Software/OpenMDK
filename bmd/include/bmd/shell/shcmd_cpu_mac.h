/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_cpu_mac(int argc, char *argv[]);

#define BMD_SHCMD_CPU_MAC_DESC "Add or remove CPU MAC address"
#define BMD_SHCMD_CPU_MAC_SYNOP \
"action vlan mac-addr"
#define BMD_SHCMD_CPU_MAC_HELP \
"Add or remove CPU MAC address to/from the L2 address\n" \
"resolution table. Adding a CPU MAC address prevents packets\n" \
"destined for the CPU from being flooded to all switch ports.\n" \
"Valid actions are add and remove. The MAC address should be\n" \
"specified as xx:xx:xx:xx:xx:xx."
