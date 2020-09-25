/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int cdk_shcmd_robo_list(int argc, char *argv[]);

#define CDK_SHCMD_ROBO_LIST_DESC  "List symbols for a chip"
#define CDK_SHCMD_ROBO_LIST_SYNOP "[raw] [flags=symflags] symbol-string"
#define CDK_SHCMD_ROBO_LIST_HELP \
"List all symbols or symbols partially matching optional\n" \
"symbol-string argument. The raw option suppresses the decoding of\n" \
"individual register/memory fields. The flags option will filter\n" \
"output based on one or more of the following flags:\n" \
"register, port, counter, memory."
