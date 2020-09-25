/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int cdk_shcmd_debug(int argc, char *argv[]);

#define CDK_SHCMD_DEBUG_DESC  "Configure debug message output"
#define CDK_SHCMD_DEBUG_SYNOP "[[!]opt] ... "
#define CDK_SHCMD_DEBUG_HELP \
"With no arguments, the current debug settings are displayed.\n" \
"To turn on additional debug messages, add one or more debug\n" \
"options as arguments. To turn off a debug option, prepend it\n" \
"with a '!'."
