/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of strncasecmp.
 */

#include <cdk/cdk_string.h>

char *
cdk_strchr(const char *dest, int c)
{
    while (*dest) {
	if (*dest == c) return (char *) dest;
	dest++;
    }
    return NULL;
}
