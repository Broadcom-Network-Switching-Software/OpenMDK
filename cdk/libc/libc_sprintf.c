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
cdk_sprintf(char *buf, const char *fmt, ...)
{
    va_list ap;
    int r;

    va_start(ap,fmt);
    r = CDK_VSNPRINTF(buf, CDK_VSNPRINTF_X_INF, fmt, ap);
    va_end(ap);

    return r;
}
