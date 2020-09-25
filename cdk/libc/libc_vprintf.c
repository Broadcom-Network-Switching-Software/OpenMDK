/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK libc printf function implementations.
 */

#include <cdk/cdk_printf.h>

int
cdk_vprintf(const char *fmt, va_list ap)
{
    int count;
    char buffer[512];

    count = CDK_VSNPRINTF(buffer,sizeof(buffer),fmt,ap);

    CDK_PUTS(buffer);

    return count;
}
