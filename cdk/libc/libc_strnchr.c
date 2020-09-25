/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of strlcpy (non-ANSI/ISO).
 */

#include <cdk/cdk_string.h>

char *
cdk_strnchr(const char *dest, int c, size_t cnt)
{
    while (*dest && (cnt > 0)) {
	if (*dest == c) return (char *) dest;
	dest++;
	cnt--;
    }
    return NULL;
}

