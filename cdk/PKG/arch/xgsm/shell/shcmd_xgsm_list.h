/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int cdk_shcmd_xgsm_list(int argc, char *argv[]);

#define CDK_SHCMD_XGSM_LIST_DESC  "List symbols for a chip"
#define CDK_SHCMD_XGSM_LIST_SYNOP "[raw] symbol-string"
#define CDK_SHCMD_XGSM_LIST_HELP \
"List all symbols or symbols partially matching optional\n" \
"symbol-string argument. If raw is specified, only the name" \
"of matching symbols will be displayed."
