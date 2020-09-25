#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53262_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm53262_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm53262_a0_bmd.h"

#define ROBO_RESET_MSEC 500

int
bcm53262_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int msec;
    CHIP_RST_CTLr_t cr_ctrl;

    BMD_CHECK_UNIT(unit);

    CHIP_RST_CTLr_CLR(cr_ctrl);
    CHIP_RST_CTLr_RST_CHIPf_SET(cr_ctrl, 1);
    ioerr += WRITE_CHIP_RST_CTLr(unit, cr_ctrl);

    /* Wait for LC PLL locks */
    for (msec = 0; msec < ROBO_RESET_MSEC; msec++) {
        ioerr += READ_CHIP_RST_CTLr(unit, &cr_ctrl);
        if (CHIP_RST_CTLr_RST_CHIPf_GET(cr_ctrl) == 0) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (msec >= ROBO_RESET_MSEC) {
        CDK_WARN(("bcm53262_a0_bmd_reset[%d]: watchdog timeout\n", unit));
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53262_A0 */
