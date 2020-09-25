/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

/*
 * Function:
 *	cdk_dev_read32
 * Purpose:
 *	Read a 32-bit device register.
 * Parameters:
 *      unit - unit number
 *      addr - register address
 *      pval - (OUT) register value
 * Returns:
 *      CDK_E_NONE on success.
 * Notes:
 *      This function relies on the device vectors
 *      supplied when the device was created.
 */
int
cdk_dev_read32(int unit, uint32_t addr, uint32_t *pval)
{
    int rv = CDK_E_NONE;

    CDK_ASSERT(CDK_UNIT_VALID(unit)); 

    if (CDK_DEV_BASE_ADDR(unit)) {
	*pval = CDK_DEV_BASE_ADDR(unit)[addr/4]; 
    } else if (CDK_DEV_VECT(unit)->read32) {
	rv = CDK_DEV_VECT(unit)->read32(CDK_DEV_DVC(unit), addr, pval); 
    } else {    
	CDK_ASSERT(0); 
    }

    CDK_DEBUG_DEV(("cdk_dev_read32[%d]: addr: 0x%08"PRIx32" data: 0x%08"PRIx32"\n", 
                   unit, addr, *pval)); 

    return rv; 
}
