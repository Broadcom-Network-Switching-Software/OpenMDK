/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK libc absolute function implementations.
 */

#include <cdk/cdk_stdlib.h>

int 
cdk_abs(int j)
{
    int pos;
    
    pos = j;
    if (j < 0) {
        pos = -j;
    }
    
    return pos;
}
