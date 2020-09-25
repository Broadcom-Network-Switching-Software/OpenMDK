/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of strrchr.
 */

#include <cdk/cdk_string.h>

char *
cdk_strrchr(const char *dest, int c)
{
    char *ret = NULL;

    while (*dest) {
	if (*dest == c) ret = (char *) dest;
	dest++;
    }

    return ret;
}
