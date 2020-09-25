/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Special string conversion function that accept 0b prefix
 * to specify binary numbers and 0 prefix for octal.
 */

#include <cdk/cdk_stdlib.h>

int 
cdk_ctoi(const char *s, char **end)
{
    if (s == 0) {
	return 0;
    }

    if (s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        return (int)CDK_STRTOL(&s[2], end, 2);
    }

    if (s[0] == '0' && (s[1] >= '0' && s[1] <= '7')) {
        return (int)CDK_STRTOL(s, end, 8);
    }

    return (int)CDK_STRTOL(s, end, 0);
}
