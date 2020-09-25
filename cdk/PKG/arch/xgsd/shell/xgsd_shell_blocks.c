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

#include <cdk/arch/xgsd_shell.h>

static char *_badblk = "*BADBLK*";

const char *
cdk_xgsd_shell_block_type2name(int unit, int blktype)
{
    if (blktype >= 0 && blktype < CDK_XGSD_INFO(unit)->nblktypes) {
        return CDK_XGSD_BLKTYPE_NAMES(unit)[blktype]; 
    }
    return _badblk; 
}

int
cdk_xgsd_shell_block_name2type(int unit, const char* name)
{
    int i; 

    for (i = 0; i < CDK_XGSD_INFO(unit)->nblktypes; i++) {
	if (CDK_STRCMP(CDK_XGSD_BLKTYPE_NAMES(unit)[i], name) == 0) {
	    return i;
	}
    }	
    return -1; 
}

char *
cdk_xgsd_shell_block_name(int unit, int block, int acctype, char* dst)
{
    int blktype, n; 

    if (cdk_xgsd_block_type(unit, block, &blktype, &n) >= 0) {
        if (acctype >= 0) {
            n = acctype;
        }
	CDK_SPRINTF(dst, "%s%d", cdk_xgsd_shell_block_type2name(unit, blktype), n); 
	return dst; 
    }
    return _badblk; 
}
