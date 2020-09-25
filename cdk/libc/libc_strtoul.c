/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK libc string conversion function implementations.
 */

#include <cdk/cdk_stdlib.h>

unsigned long
cdk_strtoul(const char *s, char **end, int base)
{
    return (unsigned long)CDK_STRTOL(s, end, base);
}
