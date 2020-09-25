#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53314_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm53314_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm53314_a0_bmd.h"

#define RESET_SLEEP_USEC                100
#define PLL_LOCK_MSEC                   500

static int
bcm53314_a0_lcpll_check(int unit)
{
    int ioerr = 0;
    int msec;
    CMIC_XGXS_PLL_STATUSr_t pll_status;

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
        CDK_WARN(("bcm53314_a0_bmd_reset[%d]: "
                  "LC PLL did not lock, status = 0x%08"PRIx32"\n",
                  unit, CMIC_XGXS_PLL_STATUSr_GET(pll_status)));
    }

    return ioerr;
}

int
bcm53314_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int wait_usec = 10000;
    CMIC_CONFIGr_t cmic_config;
    CMIC_SOFT_RESET_REGr_t cmic_sreset;
    CMIC_SBUS_RING_MAPr_t ring_map;
    CMIC_CHIP_MODE_CONTROLr_t chip_mode;
    CMIC_INTR_WAIT_CYCLESr_t wait_cycles;
    CMIC_QGPHY_QSGMII_CONTROLr_t qg_ctrl;

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
    ioerr += WRITE_CMIC_SOFT_RESET_REGr(unit, cmic_sreset);

    /* Check PLL lock */
    ioerr += bcm53314_a0_lcpll_check(unit);

    /* Bring QGMII and QGPHY out of reset */
    CMIC_SOFT_RESET_REGr_CMIC_QSGMII2X0_RST_Lf_SET(cmic_sreset,1);
    CMIC_SOFT_RESET_REGr_CMIC_QSGMII2X1_RST_Lf_SET(cmic_sreset,1);
    CMIC_SOFT_RESET_REGr_CMIC_QGPHY0_RST_Lf_SET(cmic_sreset,1);
    CMIC_SOFT_RESET_REGr_CMIC_QGPHY1_RST_Lf_SET(cmic_sreset,1);
    ioerr += WRITE_CMIC_SOFT_RESET_REGr(unit, cmic_sreset);
    BMD_SYS_USLEEP(50);

    /* Bring remaining blocks out of reset */
    CMIC_SOFT_RESET_REGr_CMIC_GP_RST_Lf_SET(cmic_sreset, 1);
    CMIC_SOFT_RESET_REGr_CMIC_IP_RST_Lf_SET(cmic_sreset, 1);
    CMIC_SOFT_RESET_REGr_CMIC_EP_RST_Lf_SET(cmic_sreset, 1);
    CMIC_SOFT_RESET_REGr_CMIC_MMU_RST_Lf_SET(cmic_sreset, 1);
    ioerr += WRITE_CMIC_SOFT_RESET_REGr(unit, cmic_sreset);
    BMD_SYS_USLEEP(wait_usec);

    /*
     * BCM53314 ring map
     *
     * ring0 [00] : IPIPE[7] -> IPIPE_HI[8]
     * ring1 [01] : EPIPE[9] -> EPIPE_HI[10]
     * ring2 [10] : gport0[2] -> gport1[3] -> gport2[4] -> MMU[6]
     * ring3 [11] 
     *
     * 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1 0
     * 0000__XXXX__1101__0100__0010__1010__1010__10XX
     */
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x0ad42aaa);
    ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, ring_map);

    /* Select managed mode */
    READ_CMIC_CHIP_MODE_CONTROLr(unit, &chip_mode);  
    CMIC_CHIP_MODE_CONTROLr_UNMANAGED_MODEf_SET(chip_mode, 0);  
    WRITE_CMIC_CHIP_MODE_CONTROLr(unit, chip_mode);  

    /* Disable fatal error interrupt */
    CMIC_INTR_WAIT_CYCLESr_CLR(wait_cycles);
    WRITE_CMIC_INTR_WAIT_CYCLESr(unit, wait_cycles);

    /* Drive LEDs from CMIC */
    READ_CMIC_QGPHY_QSGMII_CONTROLr(unit, &qg_ctrl);
    CMIC_QGPHY_QSGMII_CONTROLr_SEL_LEDRAM_SERIAL_DATAf_SET(qg_ctrl, 1);
    WRITE_CMIC_QGPHY_QSGMII_CONTROLr(unit, qg_ctrl);

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53314_A0 */
