/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_stat(int argc, char *argv[]);

#define BMD_SHCMD_STAT_DESC "Get or clear statistics counters"
#define BMD_SHCMD_STAT_SYNOP \
"ports [clear]"
#define BMD_SHCMD_STAT_HELP \
"Read or clear statistics counter for one or more ports. Use port\n" \
"'all' to read or clear statistics counters for all ports. "
