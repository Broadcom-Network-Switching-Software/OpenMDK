/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk_config.h>
#include <cdk/cdk_chip.h>

/*
 * Function:
 *	cdk_pbmp_is_null
 * Purpose:
 *	Check if any bits are set in a port bitmap.
 * Parameters:
 *      pbmp - port biitmap to check
 * Returns:
 *      1 if no bits are set, otherwise 0.
 */
int
cdk_pbmp_is_null(const cdk_pbmp_t *pbmp)
{
    int	i;

    for (i = 0; i < CDK_PBMP_WORD_MAX; i++) {
	if (CDK_PBMP_WORD_GET(*pbmp, i) != 0) {
	    return 0;
	}
    }
    return 1;
}
