#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53125_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm53125_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm53125_a0_bmd.h"
#include "bcm53125_a0_internal.h"

#define ROBO_RESET_MSEC 100

int
bcm53125_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int msec;
    WATCH_DOG_CTRLr_t wd_ctrl;

    BMD_CHECK_UNIT(unit);
    WATCH_DOG_CTRLr_CLR(wd_ctrl);
    WATCH_DOG_CTRLr_EN_SW_RESETf_SET(wd_ctrl, 1);
    WATCH_DOG_CTRLr_SOFTWARE_RESETf_SET(wd_ctrl, 1);
    ioerr += WRITE_WATCH_DOG_CTRLr(unit, wd_ctrl);

    /* Wait for LC PLL locks */
    for (msec = 0; msec < ROBO_RESET_MSEC; msec++) {
        ioerr += READ_WATCH_DOG_CTRLr(unit, &wd_ctrl);
        if (WATCH_DOG_CTRLr_SOFTWARE_RESETf_GET(wd_ctrl) == 0) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (msec >= ROBO_RESET_MSEC) {
        CDK_WARN(("bcm53125_a0_bmd_reset[%d]: watchdog timeout\n", unit));
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

int
bcm53125_a0_int_cpu_lock(int unit, int lock)
{
    int ioerr = 0;
    int msec;
    CPU_RESOURCE_ARBITERr_t cpu_arb;
    ioerr += READ_CPU_RESOURCE_ARBITERr(unit ,&cpu_arb);

    if (lock) {
        /* Require an arbiter grant */
        CPU_RESOURCE_ARBITERr_EXT_CPU_REQf_SET(cpu_arb, 1);
        ioerr += WRITE_CPU_RESOURCE_ARBITERr(unit, cpu_arb);

        /* wait for grant */
        for (msec = 0; msec < ROBO_RESET_MSEC; msec++) {
            ioerr += READ_CPU_RESOURCE_ARBITERr(unit, &cpu_arb);
            if (CPU_RESOURCE_ARBITERr_EXT_CPU_GNTf_GET(cpu_arb)) {
                break;
            }
            BMD_SYS_USLEEP(1000);
        }
    } else {
        /* Release the arbiter grant */
        CPU_RESOURCE_ARBITERr_EXT_CPU_REQf_SET(cpu_arb, 0);
        ioerr += WRITE_CPU_RESOURCE_ARBITERr(unit, cpu_arb);
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53125_A0 */
