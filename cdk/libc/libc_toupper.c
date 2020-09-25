/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of toupper.
 */

#include <cdk/cdk_ctype.h>

char 
cdk_toupper(char c)
{
    if ((c >= 'a') && (c <= 'z')) c -= 32;
    return c;
}
