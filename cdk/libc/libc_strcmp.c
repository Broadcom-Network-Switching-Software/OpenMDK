/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of strcmp.
 */

#include <cdk/cdk_string.h>

int 
cdk_strcmp(const char *dest, const char *src)
{
    int rv = 0;

    while (1) {
        if ((rv = *dest - *src++) != 0 || !*dest++) {
            break;
        }
    }
    return rv;
}
