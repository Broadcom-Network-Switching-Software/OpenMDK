/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int cdk_shcmd_help(int argc, char *argv[]);

#define CDK_SHCMD_HELP_DESC     "Obtain help for shell commands"
#define CDK_SHCMD_HELP_SYNOP    "[command]"
#define CDK_SHCMD_HELP_HELP \
"Display detailed help for a command. If no command is specified,\n" \
"a summary of all available commands will be displayed instead."
