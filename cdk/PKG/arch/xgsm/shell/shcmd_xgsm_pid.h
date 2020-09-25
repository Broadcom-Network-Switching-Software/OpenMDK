/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int cdk_shcmd_xgsm_pid(int argc, char *argv[]);

#define CDK_SHCMD_XGSM_PID_DESC  "Show how input identifier is parsed"
#define CDK_SHCMD_XGSM_PID_SYNOP "identifier"
#define CDK_SHCMD_XGSM_PID_HELP \
"This command prints out how an input identifier is parsed.\n" \
"Used mainly for debugging.\n\n" \
"If you aren't getting the right information based on the input\n" \
"identifier, you can use this command to see if it got parsed\n" \
"incorrectly."
