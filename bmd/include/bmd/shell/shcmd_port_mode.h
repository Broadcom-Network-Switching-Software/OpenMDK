/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_port_mode(int argc, char *argv[]);

#define BMD_SHCMD_PORT_MODE_DESC "Set or get port mode"
#define BMD_SHCMD_PORT_MODE_SYNOP \
"[ports [mode [higig|higig2|hglite] [loopback=mac|phy]] [update]]"
#define BMD_SHCMD_PORT_MODE_HELP \
"With no arguments, the command will list possible port modes.\n" \
"If none of the HiGig flags are specified, the port will be\n" \
"configured to use standard IEEE encapsulation. The update option\n" \
"will update the port mode according to the PHY status.\n" \
"Use port 'all' to read, configure or update all ports.\n\n" \
"Examples:\n\n" \
"portmode 1-24 1000fd\n" \
"portmode all update"
