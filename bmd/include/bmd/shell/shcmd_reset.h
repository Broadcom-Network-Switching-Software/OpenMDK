/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_reset(int argc, char *argv[]);

#define BMD_SHCMD_RESET_DESC "Reset chip"
#define BMD_SHCMD_RESET_SYNOP ""
#define BMD_SHCMD_RESET_HELP \
"Perform a hard reset of the chip and bring all blocks out\n" \
"of reset again. This will leave the chip in a state where\n" \
"all registers and memories are accessible."
