/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int bmd_shcmd_bmd(int argc, char *argv[]);

#define BMD_SHCMD_BMD_DESC     "Show BMD configuration"
#define BMD_SHCMD_BMD_SYNOP \
"[0|1] [[!]substr]"
#define BMD_SHCMD_BMD_HELP \
"Display the values of the BMD configuration variables for the current\n" \
"build. Optional arguments may be used to filter the output.\n" \
"Specifying 0 or 1 will show only variables that are zero or non-zero\n" \
"respectively. Any substr argument will filter based on a substring match\n" \
"with the variable name. Prepending substr with a ! inverts the filter.\n\n" \
"\nExamples:\n\n" \
"bmd 0\n" \
"bmd SHELL_INCLUDE"
