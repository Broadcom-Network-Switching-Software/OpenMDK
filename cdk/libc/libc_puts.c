/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK libc printf function implementations.
 */

#include <cdk/cdk_printf.h>

int (*cdk_printhook)(const char *str) = NULL;

int
cdk_puts(const char *buf)
{
    if (cdk_printhook) {
        return (*cdk_printhook)(buf);
    }
#ifdef CDK_PUTCHAR
    while (*buf) {
        CDK_PUTCHAR(*buf);
        buf++;
    }
    return 0;
#else
    return -1;
#endif
}
