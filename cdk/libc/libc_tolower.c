/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK implementation of tolower.
 */

#include <cdk/cdk_ctype.h>

char 
cdk_tolower(char c)
{
    if ((c >= 'A') && (c <= 'Z')) c += 32;
    return c;
}
