/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of strncpy.
 */

#include <cdk/cdk_string.h>

char *
cdk_strncpy(char *dest, const char *src, size_t cnt)
{
    char *ptr = dest;

    while (*src && (cnt > 0)) {
	*ptr++ = *src++;
	cnt--;
    }

    while (cnt > 0) {
	*ptr++ = 0;
	cnt--;
    }

    return dest;
}
