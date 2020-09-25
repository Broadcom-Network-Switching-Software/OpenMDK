/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of memcmp.
 */

#include <cdk/cdk_string.h>

int 
cdk_memcmp(const void *dest, const void *src, size_t cnt)
{
    const unsigned char *d;
    const unsigned char *s;

    d = (const unsigned char *) dest;
    s = (const unsigned char *) src;

    while (cnt) {
	if (*d < *s) return -1;
	if (*d > *s) return 1;
	d++; s++; cnt--;
    }

    return 0;
}
