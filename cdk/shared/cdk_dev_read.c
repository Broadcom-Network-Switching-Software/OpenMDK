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
 *	cdk_dev_read
 * Purpose:
 *	Read a device buffer.
 * Parameters:
 *      unit - unit number
 *      addr - device register/memory address
 *      data - (OUT) data buffer
 *      len - number of bytes to read
 * Returns:
 *      CDK_E_NONE on success.
 * Notes:
 *      This function is used for reading registers and
 *      and memories of arbitrary size, which is required
 *      on certain non-PCI device architectures.
 *      This function relies on the device vectors
 *      supplied when the device was created.
 */
int
cdk_dev_read(int unit, uint32_t addr, uint8_t *data, uint32_t len)
{
    int rv = CDK_E_NONE;
    uint32_t idx;

    CDK_ASSERT(CDK_UNIT_VALID(unit)); 

    if (CDK_DEV_VECT(unit)->read) {
	rv = CDK_DEV_VECT(unit)->read(CDK_DEV_DVC(unit), addr, data, len); 
    } else {    
	CDK_ASSERT(0); 
    }

    CDK_DEBUG_DEV(("cdk_dev_read[%d]: addr: 0x%08"PRIx32" len: %"PRIu32" data:", 
                   unit, addr, len)); 
    for (idx = 0; idx < len; idx++) {
        CDK_DEBUG_DEV((" 0x%02x", data[idx]));
    }
    CDK_DEBUG_DEV(("\n"));

    return rv; 
}
