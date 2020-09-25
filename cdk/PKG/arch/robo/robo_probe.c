/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 *
 * Architecture specific probe function that extracts chip ID
 * information from the Robo PHY ID registers and optionally
 * retrieves model information from chip-specific register
 *
 * The reg_read functions has same prototype as the read
 * function in the cdk_dev_vectors_t type.
 *
 * The id.model_info for Robo devices is defined like this:
 *
 *  31  28 27         20 19  16 15          8 7           0
 * +------+-------------+------+-------------+-------------+
 * | rlen |   roffset   | mlen |    page     |   moffset   |
 * +------+-------------+------+-------------+-------------+
 *
 * rlen:        Size of revision register (in bytes)
 * roffset:     Revision register offset
 * mlen:        Size of model ID register (in bytes)
 * page:        Page containing model ID and revision registers
 * moffset:     Model ID register offset
 */

#include <cdk/cdk_device.h>
#include <cdk/arch/robo_chip.h>

int
cdk_robo_probe(void *dvc, cdk_dev_id_t *id,
               int (*reg_read)(void *, uint32_t, uint8_t *, uint32_t))
{
    cdk_dev_probe_info_t pi;
    uint8_t buf[16];

    /* Re-read PHY ID registers */
    reg_read(dvc, 0x1004, buf, 2);
    id->vendor_id = buf[0] | (buf[1] << 8);
    reg_read(dvc, 0x1006, buf, 2);
    id->device_id = buf[0] | (buf[1] << 8);

    /* Revision is lower 4 bits of device ID (usually - see below) */
    id->revision = id->device_id & 0xf;
    id->device_id &= 0xfff0;

    /* Look for additional probing info */
    cdk_dev_probe_info_get(id, &pi);
    if (pi.model_info) {
        /* Read model ID */
        uint32_t addr = pi.model_info & 0xffff;
        uint32_t len = (pi.model_info >> 16) & 0xf;
        buf[0] = buf[1] = 0;
        reg_read(dvc, addr, buf, len);
        id->model = buf[0] | (buf[1] << 8);
        /* Optionally read revision from alternative location */
        len = (pi.model_info >> 28) & 0xf;
        if (len > 0) {
            addr &= 0xff00;
            addr |= (pi.model_info >> 20) & 0xff;
            reg_read(dvc, addr, buf, len);
            id->revision = buf[0];
        }
    }

    return 0;
}
