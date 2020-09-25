/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS shell symbol flag utilities.
 */

#include <cdk/arch/xgs_shell.h>

static int
_mask2idx(uint32_t mask)
{
    int idx;

    for (idx = 0; idx < 32; idx++) {
        if (mask & (1 << idx)) {
            return idx;
        }
    }
    return -1;
}

const char *
cdk_xgs_shell_symflag_type2name(int unit, uint32_t flag)
{
    const char *rc; 
    
    /* Input can be either a CDK_SYMBOL_FLAG* or a blktype bitmap */
    if ((rc = cdk_shell_symflag_type2name(flag)) != NULL) {
        return rc; 
    }
    if ((rc = cdk_xgs_shell_block_type2name(unit, _mask2idx(flag))) != NULL) {
        return rc; 
    }
    return NULL; 
}

uint32_t
cdk_xgs_shell_symflag_name2type(int unit, const char *name)
{
    uint32_t rc; 
    int blktype;

    if ((rc = cdk_shell_symflag_name2type(name))) {
        return rc; 
    }
    if ((blktype = cdk_xgs_shell_block_name2type(unit, name)) >= 0) {
        return 1 << blktype;
    }
    return 0; 
}


int
cdk_xgs_shell_symflag_cst2flags(int unit, const cdk_shell_tokens_t *cst, 
                                uint32_t *present, uint32_t *absent)
{
    int i; 

    *present = *absent = 0; 
    
    for (i = 0; i < cst->argc; i++) {
        uint32_t flag; 
        char *s = cst->argv[i]; 
        int not_set = 0; 
        
        /* Make sure this flag is 1 or 0 ?*/
        if (s[0] == '!') {
            /* Flag must be zero */
            not_set = 1; 
            s++; 
        }
                
        if (cdk_shell_parse_uint32(s, &flag) < 0) {
            flag = cdk_xgs_shell_symflag_name2type(unit, s); 
        }

        /*
         * Add the result to the correct flag
         */
        if (not_set) {
            *absent |= flag; 
        } else {
            *present |= flag; 
        }
    }
    return 0;
}    
