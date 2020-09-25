/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int cdk_shcmd_xgs_seti(int argc, char *argv[]);

#define CDK_SHCMD_XGS_SETI_DESC  "Modify chip register/memory contents (raw)"
#define CDK_SHCMD_XGS_SETI_SYNOP \
"access-type addr value[:value2[:value3 ... ]] [size]"
#define CDK_SHCMD_XGS_SETI_HELP \
"Modify raw chip register or memory contents. The access-type\n" \
"must be one of cmic, reg, mem or miim. The optional size\n" \
"parameter is the number of 32-bit words to write.\n\n" \
"Examples:\n" \
"seti mem 0x01900002 0x4\n" \
"seti mem 0x06700000 0x112233:0x44550000:0x8 3"
