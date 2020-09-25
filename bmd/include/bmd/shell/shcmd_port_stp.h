/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_port_stp(int argc, char *argv[]);

#define BMD_SHCMD_PORT_STP_DESC "Set or get spanning tree state"
#define BMD_SHCMD_PORT_STP_SYNOP "ports [state]"
#define BMD_SHCMD_PORT_STP_HELP \
"If no state is specified then the current spanning tree state\n" \
"will be displayed. Valid states are disable, block, listen,\n" \
"learn and forward."
