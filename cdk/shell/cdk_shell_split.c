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
 *	cdk_shell_split
 * Purpose:
 *	Split string into tokens.
 * Parameters:
 *	str - string to split (will be modified)
 *	argv - (OUT) token pointer array
 *	max - size of token pointer array
 *	delim - string of allowed token delimiters
 * Returns:
 *      Number of tokens in input string.
 * Notes:
 *      If delim is NULL, default delimiters will be used.
 */
int
cdk_shell_split(char *str, char *argv[], int max, const char *delim)
{	
    int i, argc = 0; 
    char* ptr; 

    CDK_ASSERT(str); 
    CDK_ASSERT(argv); 
    
    /* Default delimiter is space */
    if (delim == NULL) {
	delim = " ";
    }

    /* 
     * Break the input string up into chunks delimited by 
     * characters in the delimiter string.
     */
    ptr = str; 
    for(i = 0; i < max && *ptr; i++) {
	while (*ptr && CDK_STRCHR(delim, *ptr)) {
            ptr++; 
        }
	if (*ptr == 0) {
            break; 
        }
	argv[i] = ptr; 
	argc++; 
	while (*ptr && !CDK_STRCHR(delim, *ptr)) {
            ptr++; 
        }
        if (*ptr) {
            *ptr++ = 0;
        } 
    }

    return argc; 
}
