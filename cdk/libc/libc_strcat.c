/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of strcat.
 */

#include <cdk/cdk_string.h>

char *
cdk_strcat(char *dest, const char *src)
{
    char *ptr = dest;

    while (*ptr) ptr++;
    while (*src) *ptr++ = *src++;
    *ptr = '\0';

    return dest;
}
