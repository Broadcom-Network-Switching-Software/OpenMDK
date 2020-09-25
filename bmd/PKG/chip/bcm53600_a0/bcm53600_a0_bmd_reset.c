#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53600_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm53600_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm53600_a0_bmd.h"

#define ROBO_RESET_MSEC 500

int
bcm53600_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    uint32_t msec;
    CHIP_RST_CTLr_t cr_ctrl;

    BMD_CHECK_UNIT(unit);
    
    /* Chip reset */
    ioerr += READ_CHIP_RST_CTLr(unit, &cr_ctrl);
    CHIP_RST_CTLr_SWITCH_RST_CTLf_SET(cr_ctrl, 1);
    ioerr += WRITE_CHIP_RST_CTLr(unit, cr_ctrl);
    BMD_SYS_USLEEP(500000); /* 500 ms */
    
    /* Wait for chip reset complete */
    for (msec = 0; msec < 100000; msec++) {
        ioerr += READ_CHIP_RST_CTLr(unit, &cr_ctrl);
        if (ioerr < 0) {
            BMD_SYS_USLEEP(3000000);
            break;
        }
        if (CHIP_RST_CTLr_SWITCH_RST_CTLf_GET(cr_ctrl) == 0) {
            break;
        }
    }

    if (msec >= ROBO_RESET_MSEC) {
        CDK_WARN(("bcm53600_a0_bmd_reset[%d]: reset timeout\n", unit));
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53600_A0 */
