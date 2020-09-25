/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK libc string conversion function implementations.
 */

#include <cdk/cdk_stdlib.h>

long
cdk_strtol(const char *s, char **end, int base)
{
    long n;
    int neg;

    if (s == 0) {
	return 0;
    }

    s += (neg = (*s == '-'));

    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
	    base = 16;
	    s += 2;
	} else {
            base = 10;
        }
    }

    for (n = 0; ((*s >= 'a' && *s < 'a' + base - 10) ||
		 (*s >= 'A' && *s < 'A' + base - 10) ||
		 (*s >= '0' && *s <= '9')); s++) {
	n = n * base + ((*s <= '9' ? *s : *s + 9) & 0xf);
    }

    if (end != 0) {
	*end = (char *) s;
    }

    return (neg ? -n : n);
}
