/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of strstr.
 */

#include <cdk/cdk_string.h>

char *
cdk_strstr(const char *dest, const char *src)
{
    int dl, sl;

    if ((sl = CDK_STRLEN(src)) == 0)
        return (char *) dest;
    dl = CDK_STRLEN(dest);
    while (dl >= sl) {
        dl--;
        if (CDK_MEMCMP(dest,src,sl) == 0)
            return (char *) dest;
        dest++;
    }
    return NULL;
}
