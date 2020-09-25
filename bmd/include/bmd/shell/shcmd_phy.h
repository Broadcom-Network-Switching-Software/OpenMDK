/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_phy(int argc, char *argv[]);

#define BMD_SHCMD_PHY_DESC "Read and write PHY registers"
#define BMD_SHCMD_PHY_SYNOP "[command] [ports [index[.devad] [reg [data]]]]"
#define BMD_SHCMD_PHY_HELP \
"If only the port parameter is specified, all registers of all\n" \
"PHYs for the port(s) are displayed. Use the index parameter to\n" \
"limit output to a single PHY. Add the reg parameter to display\n" \
"only a single register. If a data value is specified, this\n" \
"value will be written to the specified PHY register.\n\n" \
"Valid commands are probe and info, for which any additional\n" \
"parameters are ignored.\n\n" \
"Examples:\n\n" \
"phy 2\n" \
"phy 2 0 0x01\n" \
"phy 24 0.1 0xca0a 0x8288\n" \
"phy info 1-24\n" \
"phy probe"
