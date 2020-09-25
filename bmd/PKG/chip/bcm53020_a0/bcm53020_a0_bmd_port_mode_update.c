#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53020_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>

#include <bmdi/bmd_link.h>

#include <cdk/chip/bcm53020_a0_defs.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>

#include "bcm53020_a0_bmd.h"

#define LED_DATA_OFFSET 0xa0

int
bcm53020_a0_bmd_port_mode_update(int unit, int port)
{
    int rv = CDK_E_NONE;
    int ioerr = 0;
    STS_OVERRIDE_P5r_t sts_override_p5;
    STS_OVERRIDE_P7r_t sts_override_p7;
    STS_OVERRIDE_GMIIPr_t sts_override_gp;
    int status_change;
    bmd_port_mode_t mode;
    uint32_t flags;

    BMD_CHECK_UNIT(unit);
    BMD_CHECK_PORT(unit, port);

    rv = bmd_link_update(unit, port, &status_change);
    if (CDK_SUCCESS(rv) && status_change) {
        rv = bcm53020_a0_bmd_port_mode_get(unit, port, &mode, &flags);
        if (CDK_SUCCESS(rv)) {
            if (port == 5) {
                /* Set link down before changing port mode */
                ioerr += READ_STS_OVERRIDE_P5r(unit, &sts_override_p5);
                STS_OVERRIDE_P5r_LINK_STSf_SET(sts_override_p5, 0);
                ioerr += WRITE_STS_OVERRIDE_P5r(unit, sts_override_p5);
            } else if (port == 7) {
                /* Set link down before changing port mode */
                ioerr += READ_STS_OVERRIDE_P7r(unit, &sts_override_p7);
                STS_OVERRIDE_P7r_LINK_STSf_SET(sts_override_p7, 0);
                ioerr += WRITE_STS_OVERRIDE_P7r(unit, sts_override_p7);
            } else {
                /* Set link down before changing port mode */
                ioerr += READ_STS_OVERRIDE_GMIIPr(unit, port, &sts_override_gp);
                STS_OVERRIDE_GMIIPr_LINK_STSf_SET(sts_override_gp, 0);
                ioerr += WRITE_STS_OVERRIDE_GMIIPr(unit, port, sts_override_gp);
            }

            flags |= BMD_PORT_MODE_F_INTERNAL;
            rv = bcm53020_a0_bmd_port_mode_set(unit, port, mode, flags);

            /* Update link status */
            if (BMD_PORT_STATUS(unit, port) & BMD_PST_LINK_UP) {
                if (port == 5) {
                    ioerr += READ_STS_OVERRIDE_P5r(unit, &sts_override_p5);
                    STS_OVERRIDE_P5r_LINK_STSf_SET(sts_override_p5, 1);
                    ioerr += WRITE_STS_OVERRIDE_P5r(unit, sts_override_p5);
                } else if (port == 7) {
                    ioerr += READ_STS_OVERRIDE_P7r(unit, &sts_override_p7);
                    STS_OVERRIDE_P7r_LINK_STSf_SET(sts_override_p7, 1);
                    ioerr += WRITE_STS_OVERRIDE_P7r(unit, sts_override_p7);
                } else {
                    ioerr += READ_STS_OVERRIDE_GMIIPr(unit, port, &sts_override_gp);
                    STS_OVERRIDE_GMIIPr_LINK_STSf_SET(sts_override_gp, 1);
                    ioerr += WRITE_STS_OVERRIDE_GMIIPr(unit, port, sts_override_gp);
                }
            }
        }
    }

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53020_A0 */
