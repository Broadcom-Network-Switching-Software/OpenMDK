/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of strlen.
 */

#include <cdk/cdk_string.h>

size_t
cdk_strlen(const char *str)
{
    size_t cnt = 0;

    while (*str) {
	str++;
	cnt++;
    }

    return cnt;
}
