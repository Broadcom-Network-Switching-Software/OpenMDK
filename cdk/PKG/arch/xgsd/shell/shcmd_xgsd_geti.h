/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int cdk_shcmd_xgsd_geti(int argc, char *argv[]);

#define CDK_SHCMD_XGSD_GETI_DESC  "Get chip register/memory contents (raw)"
#define CDK_SHCMD_XGSD_GETI_SYNOP "access-type address [size]"
#define CDK_SHCMD_XGSD_GETI_HELP \
"Read raw chip register or memory contents. The access-type\n" \
"must be one of cmic, reg, mem or miim. The optional size\n" \
"parameter is the number of 32-bit words to read."
