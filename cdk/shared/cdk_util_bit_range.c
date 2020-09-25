/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_printf.h>
#include <cdk/cdk_shell.h>
#include <cdk/cdk_util.h>

/*
 * Function:
 *	cdk_util_bit_range
 * Purpose:
 *	Create bit range string
 * Parameters:
 *	buf - (OUT) output buffer for bit range string
 *	size - size of output buffer
 *	minbit - first bit in bit range
 *	maxbit - last bit in bit range
 * Returns:
 *      Always 0
 */
int
cdk_util_bit_range(char *buf, int size, int minbit, int maxbit)
{
    if (minbit == maxbit) {
        CDK_SNPRINTF(buf, size, "<%d>", minbit);
    } else {
        CDK_SNPRINTF(buf, size, "<%d:%d>", maxbit, minbit);
    }
    return 0;
}
