#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM89500_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm89500_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm89500_a0_bmd.h"
#include "bcm89500_a0_internal.h"

#define ROBO_SW_RESET_POLL_MAX  100000

int
bcm89500_a0_max_port(int unit)
{
    int bond_pad;
    int max_port = 0;
    BONDING_PAD_STATUSr_t bonding_pad;

    /*
     * 0: RGMIIMII/RvMII through mii3_mode (4 BR-PHY + 3 MII)
     * 1: BR/TX100 PHY. (5 BR-PHY + 2 MII)
     * 2: BR/TX100 PHY and one BR-PHY (2 BR-PHY + 2 MII)
     * 3: Unused
     */
    READ_BONDING_PAD_STATUSr(unit, &bonding_pad);
    bond_pad = BONDING_PAD_STATUSr_BOND_PADf_GET(bonding_pad);
    switch (bond_pad) {
    case 0:
        /* bcm89500, br phy port max to port 3 (start from 0) */
        max_port = 3;
        break;
    case 1:
        /* bcm89501, br phy port max to port 4 (start from 0) */
        max_port = 4;
        break;
    case 2:
        /* bcm89200, br phy port 0, 4 */
        max_port = 4;
        break;
    default:
        return CDK_E_PARAM;
    }
    return max_port;    
}

int
bcm89500_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    uint32_t cnt;
    uint32_t model;
    WATCH_DOG_CTRLr_t wd_ctrl;
    MODEL_IDr_t model_id;

    BMD_CHECK_UNIT(unit);

    WATCH_DOG_CTRLr_CLR(wd_ctrl);
    WATCH_DOG_CTRLr_EN_SW_RESETf_SET(wd_ctrl, 1);
    WATCH_DOG_CTRLr_SOFTWARE_RESETf_SET(wd_ctrl, 1);
    ioerr += WRITE_WATCH_DOG_CTRLr(unit, wd_ctrl);

    /* Wait for chip reset complete */
    for (cnt = 0; cnt < ROBO_SW_RESET_POLL_MAX; cnt++) {
        ioerr += READ_WATCH_DOG_CTRLr(unit, &wd_ctrl);
        if (ioerr) {
            break;
        } 
        if (WATCH_DOG_CTRLr_SOFTWARE_RESETf_GET(wd_ctrl) == 0) {
            /* Reset is complete */
            break;
        }
    }
    if (cnt >= ROBO_SW_RESET_POLL_MAX) {
        rv = CDK_E_TIMEOUT;
    } else {
        /* Wait for internal CPU to boot up completely */
        do {
            ioerr += READ_MODEL_IDr(unit, &model_id);
            model = MODEL_IDr_GET(model_id);
            model &= 0xffff0;
        } while (model != 0x89500);
    }

    return ioerr ? CDK_E_IO : rv;
}


#endif /* CDK_CONFIG_INCLUDE_BCM89500_A0 */
