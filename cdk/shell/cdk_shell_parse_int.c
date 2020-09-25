/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_stdlib.h>
#include <cdk/cdk_shell.h>

/*
 * Function:
 *	cdk_shell_parse_int
 * Purpose:
 *	Parse string into an integer
 * Parameters:
 *	s - string to parse
 *	d - (OUT) pointer to interger
 * Returns:
 *      -1 if string is not a valid integer, otherwise 0
 * Notes:
 *      Supports prefixes 0x (hex), 0b (binary) and 0 (octal).
 */
int 
cdk_shell_parse_int(const char *s, int *d)
{
    if (!cdk_shell_parse_is_int(s)) {
	return -1; 
    }
    if (d) {
	*d = CDK_CTOI(s, NULL); 
    }
    return 0; 
}
