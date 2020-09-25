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
 *	cdk_dev_write32
 * Purpose:
 *	Write a 32-bit device register.
 * Parameters:
 *      unit - unit number
 *      addr - register address
 *      val - register value
 * Returns:
 *      CDK_E_NONE on success.
 * Notes:
 *      This function relies on the device vectors
 *      supplied when the device was created.
 */
int
cdk_dev_write32(int unit, uint32_t addr, uint32_t val)
{
    int rv = CDK_E_NONE;

    CDK_ASSERT(CDK_UNIT_VALID(unit)); 

    CDK_DEBUG_DEV(("cdk_dev_write32[%d]: addr: 0x%08"PRIx32" data: 0x%08"PRIx32"\n", 
                   unit, addr, val)); 

    if (CDK_DEV_BASE_ADDR(unit)) {
	CDK_DEV_BASE_ADDR(unit)[addr/4] = val; 
    } else if (CDK_DEV_VECT(unit)->write32) {
	rv = CDK_DEV_VECT(unit)->write32(CDK_DEV_DVC(unit), addr, val); 
    } else {    
	CDK_ASSERT(0); 
    }

    return rv; 
}

