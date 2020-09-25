/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

extern int cdk_shcmd_xgs_get(int argc, char *argv[]);

#define CDK_SHCMD_XGS_GET_DESC  "Get chip register/memory contents"
#define CDK_SHCMD_XGS_GET_SYNOP \
"expr [nz] [raw] [flags=symflags]"
#define CDK_SHCMD_XGS_GET_HELP \
"Read and decode chip register or memory contents, where expr is a\n" \
"register/memory expression in one of the following forms:\n\n" \
"name\n" \
"name.port\n" \
"name.block\n" \
"name.block.first-port,last-port\n" \
"name[index]\n" \
"name[index].block\n" \
"name[index].block.first-port,last-port\n"
#define CDK_SHCMD_XGS_GET_HELP_2 \
"If the nz option is specified, then only registers/memories with\n" \
"non-zero contents will be displayed. The raw option suppresses\n" \
"the decoding of individual register/memory fields. The flags option\n" \
"will filter output based on one or more of the following flags:\n" \
"register, port, counter, memory, r64, big-endian.\n\n" \
"Examples:\n\n" \
"get GMACC1r.0\n" \
"get GMACC1r.gport0.1,4\n" \
"get LWMCOSCELLSETLIMITr[1].mmu0.1,2\n" \
"get nz flags=counter"
