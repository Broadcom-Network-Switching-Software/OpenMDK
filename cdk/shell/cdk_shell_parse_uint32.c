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
 *	cdk_shell_parse_uint32
 * Purpose:
 *	Parse string into a 32-bit word
 * Parameters:
 *	s - string to parse
 *	d - (OUT) pointer to 32-bit word
 * Returns:
 *      -1 if string is not a valid integer, otherwise 0
 * Notes:
 *      Supports prefixes 0x (hex), 0b (binary) and 0 (octal).
 */
int 
cdk_shell_parse_uint32(const char *s, uint32_t *d)
{
    if (!cdk_shell_parse_is_int(s)) {
	return -1; 
    }
    if (d) {
	*d = CDK_CTOI(s, NULL); 
    }
    return 0; 
}
