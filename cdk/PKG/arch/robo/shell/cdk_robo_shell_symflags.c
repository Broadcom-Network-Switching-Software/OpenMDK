/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * ROBO shell symbol flag utilities.
 */

#include <cdk/arch/robo_shell.h>

int
cdk_robo_shell_symflag_cst2flags(const cdk_shell_tokens_t *cst, 
                                 uint32_t *present, uint32_t *absent)
{
    int i; 

    *present = *absent = 0; 
    
    for (i = 0; i < cst->argc; i++) {
        uint32_t flag; 
        char *s = cst->argv[i]; 
        int not = 0; 
        
        /* Make sure this flag is 1 or 0 ?*/
        if (s[0] == '!') {
            /* Flag must be zero */
            not = 1; 
            s++; 
        }
                
        if (cdk_shell_parse_uint32(s, &flag) < 0) {
            /* If not integer, look for symbolic name */
            flag = cdk_shell_symflag_name2type(s); 
        }

        /* Add the result to the correct flag */
        if (not) {
            *absent |= flag; 
        } else {
            *present |= flag; 
        }
    }
    return 0;
}    

