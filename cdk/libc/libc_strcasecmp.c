/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of strcasecmp (non-ANSI/ISO).
 */

#include <cdk/cdk_ctype.h>
#include <cdk/cdk_string.h>

int 
cdk_strcasecmp(const char *dest, const char *src)
{
    char dc,sc;
    int rv = 0;

    while (1) {
	dc = CDK_TOUPPER(*dest++);
	sc = CDK_TOUPPER(*src++);
        if ((rv = dc - sc) != 0 || !dc) {
            break;
        }
    }
    return rv;
}
