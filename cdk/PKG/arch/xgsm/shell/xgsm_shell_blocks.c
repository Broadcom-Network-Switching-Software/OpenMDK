/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

/*******************************************************************************
 *
 * Shell Block Utility Functions
 */

#include <cdk/arch/xgsm_shell.h>

static char *_badblk = "*BADBLK*";

const char *
cdk_xgsm_shell_block_type2name(int unit, int blktype)
{
    if (blktype >= 0 && blktype < CDK_XGSM_INFO(unit)->nblktypes) {
        return CDK_XGSM_BLKTYPE_NAMES(unit)[blktype]; 
    }
    return _badblk; 
}

int
cdk_xgsm_shell_block_name2type(int unit, const char* name)
{
    int i; 

    for (i = 0; i < CDK_XGSM_INFO(unit)->nblktypes; i++) {
	if (CDK_STRCMP(CDK_XGSM_BLKTYPE_NAMES(unit)[i], name) == 0) {
	    return i;
	}
    }	
    return -1; 
}

char* 
cdk_xgsm_shell_block_name(int unit, int block, char* dst)
{
    int blktype, n; 

    if (cdk_xgsm_block_type(unit, block, &blktype, &n) >= 0) {
	CDK_SPRINTF(dst, "%s%d", cdk_xgsm_shell_block_type2name(unit, blktype), n); 
	return dst; 
    }
    return _badblk; 
}
