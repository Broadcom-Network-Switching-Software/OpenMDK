/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of strlcpy (non-ANSI/ISO).
 */

#include <cdk/cdk_string.h>

size_t
cdk_strlcpy(char *dest, const char *src, size_t cnt)
{
    char *ptr = dest;
    size_t copied = 0;

    while (*src && (cnt > 1)) {
	*ptr++ = *src++;
	cnt--;
	copied++;
    }
    *ptr = '\0';

    return copied;
}
