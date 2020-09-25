/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int cdk_shcmd_xgsd_set(int argc, char *argv[]);

#define CDK_SHCMD_XGSD_SET_DESC  "Modify chip register/memory contents"
#define CDK_SHCMD_XGSD_SET_SYNOP \
"expr value [value [value ... ]] [field=value [field=value ... ]]"
#define CDK_SHCMD_XGSD_SET_HELP \
"Modify chip register or memory contents. Contents may be modified\n" \
"either by entering raw 32-bit word values or by changing each\n" \
"field separately. The special field value 'all' will modify\n" \
"all fields in a register/memory.\n\n" \
"Examples:\n\n" \
"set GMACC1r.0 RXEN0=1 TXEN0=1\n" \
"set GMACC1r.gport0.[1,2] RXEN0=0b1 TXEN0=0b1\n" \
"set L2Xm[0] MAC_ADDR=0x010203040506\n" \
"set L2Xm[0] 0 0 0\n" \
"set LWMCOSCELLSETLIMITr[1].mmu0.1,2 CELLSETLIMIT=0xa"
