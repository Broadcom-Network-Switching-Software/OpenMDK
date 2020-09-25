/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_assert.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_shell.h>

/*
 * Function:
 *	cdk_shell_parse_vect
 * Purpose:
 *	Parse and dispatch sub commands
 * Parameters:
 *	argc - argc of the main command
 *	argv - argv of the main command
 *	context - optional context pointer passed to sub-command
 *	sv - sub-command vector table
 *      rc - (OUT) return value of sub-command handler
 * Returns:
 *      Always -1
 */
int 
cdk_shell_parse_vect(int argc, char **argv, void *context, 
		     cdk_shell_vect_t *sv, int *rc)
{
    /* Find the first argument in the vector table and call its handler */
    CDK_ASSERT(argv); 
    CDK_ASSERT(sv); 
    
    if (*argv) {	
	while (sv->id) {
	    if (CDK_STRCMP(argv[0], sv->id) == 0) {
		int retval = sv->v(argc-1, argv+1, context); 
		if (rc) *rc = retval; 
		return 0; 
	    }
	    sv++; 
	}
    }
    return -1; 
}
