/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Parse command line tokens into token structure.
 */

#include <cdk/cdk_assert.h>
#include <cdk/cdk_string.h>
#include <cdk/cdk_shell.h>

/*
 * Function:
 *	cdk_shell_tokenize
 * Purpose:
 *	Split string into tokens.
 * Parameters:
 *	str - string to split (will not be modified)
 *	tok - (OUT) token structure
 *	delim - string of allowed token delimiters
 * Returns:
 *      Always 0
 */
int
cdk_shell_tokenize(const char *str, cdk_shell_tokens_t *tok, const char *delim)
{	
    CDK_ASSERT(tok); 
    
    /* Clear the tokens structure */
    CDK_MEMSET(tok, 0, sizeof(*tok));

    /* Do not modify source string */
    CDK_STRLCPY(tok->str, str, sizeof(tok->str));

    tok->argc = cdk_shell_split(tok->str, tok->argv, CDK_CONFIG_SHELL_MAX_ARGS, delim);

    return 0;
}
