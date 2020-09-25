/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * ROBO register access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include <cdk/arch/robo_chip.h>
#include <cdk/arch/robo_reg.h>

/*******************************************************************************
 *
 * Common register routines
 */

int
cdk_robo_reg_read(int unit, uint32_t addr, void *entry_data, int size)
{
    int rv;
    int wsize = CDK_BYTES2WORDS(size);
    uint32_t *wdata = (uint32_t *)entry_data;

    /* CDK internals are word-based, so fill clear fill bytes if any */
    if (size & 3) {
        wdata[wsize - 1] = 0;
    }

    /* Read data from device */
    rv = cdk_dev_read(unit, addr, (uint8_t *)entry_data, size);
    if (CDK_FAILURE(rv)) {
        CDK_ERR(("cdk_robo_reg_read[%d]: error reading addr=%08"PRIx32"\n",
                 unit, addr));
        return rv;
    }

    /* Debug output */
    CDK_DEBUG_REG(("cdk_robo_reg_read[%d]: addr=0x%08"PRIx32" data: 0x",
                   unit, addr));
    while (size) {
        size--;
        CDK_DEBUG_REG(("%02x", (int)((uint8_t *)entry_data)[size]));
    }
    CDK_DEBUG_REG(("\n"));

    /* Byte-swap each word if necessary */
    if (CDK_DEV_FLAGS(unit) & CDK_DEV_BE_HOST) {
        while (wsize) {
            wsize--;
            wdata[wsize] = cdk_util_swap32(wdata[wsize]);
        }
    }
    return rv;
}

int
cdk_robo_reg_write(int unit, uint32_t addr, void *entry_data, int size)
{
    int rv;
    int wsize = CDK_BYTES2WORDS(size);
    uint8_t *bdata = (uint8_t *)entry_data;
    uint32_t *wdata = (uint32_t *)entry_data;
    uint32_t swap_data[2];

    /* Byte-swap each word if necessary */
    if (CDK_DEV_FLAGS(unit) & CDK_DEV_BE_HOST) {
        if (wsize > COUNTOF(swap_data)) {
            return CDK_E_PARAM;
        }
        while (wsize) {
            wsize--;
            swap_data[wsize] = cdk_util_swap32(wdata[wsize]);
        }
        bdata = (uint8_t *)swap_data;
    }

    /* Write data to device */
    rv = cdk_dev_write(unit, addr, bdata, size);

    if (CDK_FAILURE(rv)) {
        CDK_ERR(("cdk_robo_reg_write[%d]: error writing addr=%08"PRIx32"\n",
                 unit, addr));
        return rv;
    }

    /* Debug output */
    CDK_DEBUG_REG(("cdk_robo_reg_write[%d]: addr=0x%08"PRIx32" data: 0x",
                   unit, addr));
    while (size) {
        size--;
        CDK_DEBUG_REG(("%02x", (int)bdata[size]));
    }
    CDK_DEBUG_REG(("\n"));

    return rv;
}
