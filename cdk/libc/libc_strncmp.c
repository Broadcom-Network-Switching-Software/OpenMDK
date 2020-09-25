/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of strncmp.
 */

#include <cdk/cdk_string.h>

int 
cdk_strncmp(const char *dest, const char *src, size_t cnt)
{
    int rv = 0;

    while (cnt) {
        if ((rv = *dest - *src++) != 0 || !*dest++) {
            break;
        }
        cnt--;
    }
    return rv;
}
