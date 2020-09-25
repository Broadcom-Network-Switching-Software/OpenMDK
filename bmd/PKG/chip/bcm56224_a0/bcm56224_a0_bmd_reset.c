#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56224_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56224_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm56224_a0_bmd.h"

#define RESET_SLEEP_USEC                100
#define PLL_LOCK_MSEC                   500

static int
bcm56224_a0_lcpll_init(int unit)
{
    int ioerr = 0;
    int msec;
    CMIC_XGXS_PLL_CONTROL_1r_t pll_ctrl1;
    CMIC_XGXS_PLL_STATUSr_t pll_status;

    /* Reset PLL */
    ioerr += READ_CMIC_XGXS_PLL_CONTROL_1r(unit, &pll_ctrl1);
    CMIC_XGXS_PLL_CONTROL_1r_RESETf_SET(pll_ctrl1, 1);
    ioerr += WRITE_CMIC_XGXS_PLL_CONTROL_1r(unit, pll_ctrl1);
    BMD_SYS_USLEEP(50);
    CMIC_XGXS_PLL_CONTROL_1r_RESETf_SET(pll_ctrl1, 0);
    ioerr += WRITE_CMIC_XGXS_PLL_CONTROL_1r(unit, pll_ctrl1);

    /* Wait for LC PLL locks */
    for (msec = 0; msec < PLL_LOCK_MSEC; msec++) {
        ioerr += READ_CMIC_XGXS_PLL_STATUSr(unit, &pll_status);
#if BMD_CONFIG_SIMULATION
        if (msec == 0) break;
#endif
        if (CMIC_XGXS_PLL_STATUSr_CMIC_XG_PLL_LOCKf_GET(pll_status) == 1) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (msec >= PLL_LOCK_MSEC) {
        CDK_WARN(("bcm56224_a0_bmd_reset[%d]: "
                  "LC PLL did not lock, status = 0x%08"PRIx32"\n",
                  unit, CMIC_XGXS_PLL_STATUSr_GET(pll_status)));
    }

    return ioerr;
}

int
bcm56224_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int wait_usec = 10000;
    CMIC_CONFIGr_t cmic_config;
    CMIC_SOFT_RESET_REGr_t cmic_sreset;
    CMIC_SBUS_RING_MAPr_t ring_map;

    BMD_CHECK_UNIT(unit);

    /* Initialize endian mode for correct reset access */
    ioerr += cdk_xgs_cmic_init(unit);

    /* Pull reset line */
    ioerr += READ_CMIC_CONFIGr(unit, &cmic_config);
    CMIC_CONFIGr_RESET_CPSf_SET(cmic_config, 1);
    ioerr += WRITE_CMIC_CONFIGr(unit, cmic_config);

    /* Wait for all tables to initialize */
    BMD_SYS_USLEEP(wait_usec);

    /* Re-initialize endian mode after reset */
    ioerr += cdk_xgs_cmic_init(unit);

    /* Reset all blocks */
    CMIC_SOFT_RESET_REGr_CLR(cmic_sreset);
    ioerr += WRITE_CMIC_SOFT_RESET_REGr(unit, cmic_sreset);

    /* Bring PLL blocks out of reset */
    CMIC_SOFT_RESET_REGr_CMIC_XG_PLL_RST_Lf_SET(cmic_sreset, 1);
    CMIC_SOFT_RESET_REGr_CMIC_G2P51_RST_Lf_SET(cmic_sreset,1);
    CMIC_SOFT_RESET_REGr_CMIC_G2P50_RST_Lf_SET(cmic_sreset,1);
    CMIC_SOFT_RESET_REGr_CMIC_G2P52_RST_Lf_SET(cmic_sreset,1);
    CMIC_SOFT_RESET_REGr_CMIC_G2P53_RST_Lf_SET(cmic_sreset,1);
    CMIC_SOFT_RESET_REGr_CMIC_GX12_RST_Lf_SET(cmic_sreset,1);
    ioerr += WRITE_CMIC_SOFT_RESET_REGr(unit, cmic_sreset);
    BMD_SYS_USLEEP(50);

    /* Initialize LC PLL */
    ioerr += bcm56224_a0_lcpll_init(unit);

    /* Bring remaining blocks out of reset */
    CMIC_SOFT_RESET_REGr_CMIC_FP_RST_Lf_SET(cmic_sreset, 1);
    CMIC_SOFT_RESET_REGr_CMIC_GP_RST_Lf_SET(cmic_sreset, 1);
    CMIC_SOFT_RESET_REGr_CMIC_IP_RST_Lf_SET(cmic_sreset, 1);
    CMIC_SOFT_RESET_REGr_CMIC_EP_RST_Lf_SET(cmic_sreset, 1);
    CMIC_SOFT_RESET_REGr_CMIC_MMU_RST_Lf_SET(cmic_sreset, 1);
    ioerr += WRITE_CMIC_SOFT_RESET_REGr(unit, cmic_sreset);
    BMD_SYS_USLEEP(wait_usec);

    /*
     * BCM56224 ring map
     *
     * ring0 [00] : IPIPE[7] -> IPIPE_HI[8]
     * ring1 [01] : EPIPE[9] -> EPIPE_HI[10]
     * ring2 [10] : gsport0[1] -> gport0[2] ->
     *              gport1[3] ->  gport2[4] -> gport3[5] -> MMU[6]
     * ring3 [11] 
     *
     * 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1 0
     * 0000__XXXX__1101__0100__0010__1010__1010__10XX
     */
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x0ad42aaa);
    ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, ring_map);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56224_A0 */
