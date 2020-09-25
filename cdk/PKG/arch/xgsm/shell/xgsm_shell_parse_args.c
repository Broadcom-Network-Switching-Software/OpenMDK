/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

/*******************************************************************************
 *
 * Main parsing function for XGSM shell commands
 */

#include <cdk/arch/xgsm_shell.h>

int
cdk_xgsm_shell_parse_args(int argc, char *argv[], cdk_shell_tokens_t *csts, int max)
    
{		
    int idx; 
    cdk_shell_tokens_t *cst = csts;
    char tmpstr[CDK_CONFIG_SHELL_IO_MAX_LINE];

    CDK_MEMSET(csts, 0, max*sizeof(*csts)); 

    /* For all arguments */
    for (idx = 0; idx < argc && idx < max; idx++, cst++) {
	
	/* Parse each individual argument with '=' into cst */
	if (cdk_shell_tokenize(argv[idx], cst, "=") < 0) {
	    return idx;
	}
	if (cst->argc == 2) {
            /*
             * If two tokens are found, we parse a second time in 
             * order parse e.g. "flags=f1,f2,f3" into a single
             * token structure.
             */
	    CDK_SPRINTF(tmpstr, "%s,%s", cst->argv[0], cst->argv[1]); 

            /* Parse second argument with ',' into cst */
	    if (cdk_shell_tokenize(tmpstr, cst, ",") < 0) {
                return idx;
            }
	}		
	else if (cst->argc != 1) {
            /* Number of tokens must be 1 or 2 */
	    return idx;
	}	

    }
    return -1; 
}
