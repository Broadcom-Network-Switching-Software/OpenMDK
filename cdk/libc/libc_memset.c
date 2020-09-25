/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of memcpy.
 */

#include <cdk/cdk_string.h>

void *
cdk_memset(void *dest, int c, size_t cnt)
{
    unsigned char *d;

    d = dest;

    while (cnt) {
	*d++ = (unsigned char) c;
	cnt--;
    }

    return d;
}
