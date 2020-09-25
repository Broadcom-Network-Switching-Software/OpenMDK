/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int cdk_shcmd_robo_seti(int argc, char *argv[]);

#define CDK_SHCMD_ROBO_SETI_DESC  "Modify chip register/memory contents (raw)"
#define CDK_SHCMD_ROBO_SETI_SYNOP \
"access-type addr value[:value2[:value3 ... ]] [size]"
#define CDK_SHCMD_ROBO_SETI_HELP \
"Modify raw chip register or memory contents. The access-type\n" \
"must be one of reg, mem or miim. The optional size parameter\n" \
"is the number of bytes to write. Note that data values are\n" \
"specified as 32-bit values. The default size is 4 times the\n" \
"number of 32-bit words specified.\n\n" \
"Examples:\n" \
"seti reg 0x0200 0x4 1\n" \
"seti mem 0x00003000 0x112233:0x44550000:0x8 12"
