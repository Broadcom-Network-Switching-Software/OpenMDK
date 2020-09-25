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
cdk_memcpy(void *dest, const void *src, size_t cnt)
{
    unsigned char *d;
    const unsigned char *s;

    d = (unsigned char *) dest;
    s = (const unsigned char *) src;

    while (cnt) {
	*d++ = *s++;
	cnt--;
    }

    return dest;
}
