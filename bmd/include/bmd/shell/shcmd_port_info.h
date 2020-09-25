/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_port_info(int argc, char *argv[]);

#define BMD_SHCMD_PORT_INFO_DESC "Show port information"
#define BMD_SHCMD_PORT_INFO_SYNOP \
"[phybus] ports"
#define BMD_SHCMD_PORT_INFO_HELP \
"Show detailed port information.\n\n" \
"Examples:\n\n" \
"pinfo 1\n" \
"pinfo phybus 1\n" \
"pinfo 24-28"
