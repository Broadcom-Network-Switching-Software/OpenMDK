/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of strupr (non-ANSI/ISO).
 */

#include <cdk/cdk_ctype.h>
#include <cdk/cdk_string.h>

void 
cdk_strupr(char *str)
{
    while (*str) {
	*str = CDK_TOUPPER(*str);
	str++;
    }
}
