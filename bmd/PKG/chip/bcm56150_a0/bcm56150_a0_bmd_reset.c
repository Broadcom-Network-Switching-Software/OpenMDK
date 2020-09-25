/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56150_A0 == 1

#include <bmd/bmd.h>

#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/arch/xgsm_miim.h>
#include <cdk/chip/bcm56150_a0_defs.h>

#include "bcm56150_a0_bmd.h"
#include "bcm56150_a0_internal.h"

#define RESET_SLEEP_USEC                100
#define PLL_LOCK_MSEC                   500

int
bcm56150_a0_xlport_reset(int unit, int port)
{
    int ioerr = 0;
    XLPORT_XGXS0_CTRL_REGr_t xlport_xgxs0_ctrl;

    /* Reference clock selection */
    ioerr += READ_XLPORT_XGXS0_CTRL_REGr(unit, &xlport_xgxs0_ctrl, port);
    XLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(xlport_xgxs0_ctrl, 1);
    ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_xgxs0_ctrl, port);
    
    /* Deassert power down */
    XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlport_xgxs0_ctrl, 0);
    ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_xgxs0_ctrl, port);
    BMD_SYS_USLEEP(1100);
    
    /* Bring XGXS out of reset */
    XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_xgxs0_ctrl, 1);
    ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_xgxs0_ctrl, port);
    BMD_SYS_USLEEP(1100);
    
    /* Bring reference clock out of reset */
    XLPORT_XGXS0_CTRL_REGr_RSTB_REFCLKf_SET(xlport_xgxs0_ctrl, 1);
    ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_xgxs0_ctrl, port);
    
    /* Activate clocks */
    XLPORT_XGXS0_CTRL_REGr_RSTB_PLLf_SET(xlport_xgxs0_ctrl, 1);
    ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_xgxs0_ctrl, port);

    return ioerr;
}

int
bcm56150_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int wait_usec = 10000;
    int idx, port;
    cdk_pbmp_t pbmp;
    CMIC_CPS_RESETr_t cmic_cps;
    CMIC_SBUS_RING_MAPr_t sbus_ring_map;
    TOP_SOFT_RESET_REGr_t top_sreset;
    TOP_SOFT_RESET_REG_2r_t top_sreset_2;
    TOP_PVTMON_CTRL_0r_t top_pvtmon_ctrl_0;
    TOP_PVTMON_CTRL_1r_t top_pvtmon_ctrl_1;
    CMIC_SBUS_TIMEOUTr_t cmic_sbus_timeout;
    XLPORT_MAC_CONTROLr_t xlport_mac_ctrl;
    uint32_t ring_map[] = { 0x11122200, 0x00430000, 0x00005004 };

    BMD_CHECK_UNIT(unit);
    
    /* Initialize endian mode for correct reset access */
    ioerr += cdk_xgsm_cmic_init(unit);

    /* Pull reset line */
    ioerr += READ_CMIC_CPS_RESETr(unit, &cmic_cps);
    CMIC_CPS_RESETr_CPS_RESETf_SET(cmic_cps, 1);
    ioerr += WRITE_CMIC_CPS_RESETr(unit, cmic_cps);

    /* Wait for all tables to initialize */
    BMD_SYS_USLEEP(wait_usec);
 
    /* Re-initialize endian mode after reset */
    ioerr += cdk_xgsm_cmic_init(unit);

    /*
     * BCM56150 ring map
     *
     * ring 0: IP(10), EP(11)
     * ring 1: XLPORT0(5), XLPORT1(6)
     * ring 2: GPORT0(2)..GPORT2(4)
     * ring 3: MMU(12)
     * ring 4: OTPC(13), TOP(16)
     * ring 5: SER(19)
     * ring 6: unused
     * ring 7: unused
     */
    for (idx = 0; idx < COUNTOF(ring_map); idx++) {
        CMIC_SBUS_RING_MAPr_SET(sbus_ring_map, ring_map[idx]);
        ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, idx, sbus_ring_map);
    }

    /* Bring LCPLL out of reset */
    
    BMD_SYS_USLEEP(wait_usec);

    /* Bring port blocks out of reset */
    READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_XLP0_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP1_RST_Lf_SET(top_sreset, 1); 
    TOP_SOFT_RESET_REGr_TOP_QSGMII2X2_RST_Lf_SET(top_sreset, 1);  
    TOP_SOFT_RESET_REGr_TOP_QSGMII2X1_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_QSGMII2X0_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_QSGMII2X2_FIFO_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_QSGMII2X1_FIFO_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_QSGMII2X0_FIFO_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_GP2_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_GP1_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_GP0_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_SET(top_sreset, 0xf);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec); 

    /* Bring network sync out of reset */
    READ_TOP_SOFT_RESET_REGr(unit, &top_sreset); 
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_sreset, 1);     
    TOP_SOFT_RESET_REGr_TOP_SPARE_RST_Lf_SET(top_sreset, 1); 
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);

    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);

    
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    BMD_SYS_USLEEP(wait_usec);
    CMIC_SBUS_TIMEOUTr_SET(cmic_sbus_timeout, 0x7d0);
    ioerr += WRITE_CMIC_SBUS_TIMEOUTr(unit, cmic_sbus_timeout);

    /* Bring IP, EP, and MMU blocks out of reset */
    READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(top_sreset, 1); 
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(top_sreset, 1);
    ioerr +=  WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec);

    /* Reset PVTMON */
    ioerr += READ_TOP_PVTMON_CTRL_0r(unit, &top_pvtmon_ctrl_0);
    TOP_PVTMON_CTRL_0r_FUNC_MODE_SELf_SET(top_pvtmon_ctrl_0, 0);
    TOP_PVTMON_CTRL_0r_MEASUREMENT_CALLIBRATIONf_SET(top_pvtmon_ctrl_0, 5);
    TOP_PVTMON_CTRL_0r_BG_ADJf_SET(top_pvtmon_ctrl_0, 3); 
    ioerr += WRITE_TOP_PVTMON_CTRL_0r(unit, top_pvtmon_ctrl_0);

    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TEMP_MON_PEAK_RST_Lf_SET(top_sreset_2, 0);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TEMP_MON_PEAK_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);

    ioerr += READ_TOP_PVTMON_CTRL_1r(unit, &top_pvtmon_ctrl_1);
    TOP_PVTMON_CTRL_1r_PVTMON_SELECTf_SET(top_pvtmon_ctrl_1, 0);
    TOP_PVTMON_CTRL_1r_PVTMON_RESET_Nf_SET(top_pvtmon_ctrl_1, 0);
    ioerr += WRITE_TOP_PVTMON_CTRL_1r(unit, top_pvtmon_ctrl_1);
    TOP_PVTMON_CTRL_1r_PVTMON_RESET_Nf_SET(top_pvtmon_ctrl_1, 1);
    ioerr += WRITE_TOP_PVTMON_CTRL_1r(unit, top_pvtmon_ctrl_1);  

    /* Reset all XLPORTs */
    bcm56150_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        /* We only need to reset first port in each block */
        if (XLPORT_SUBPORT(port) == 0) {
            rv += bcm56150_a0_xlport_reset(unit, port);
            
            ioerr += READ_XLPORT_MAC_CONTROLr(unit, &xlport_mac_ctrl, port);
            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlport_mac_ctrl, 1);
            ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlport_mac_ctrl, port);
            
            ioerr += READ_XLPORT_MAC_CONTROLr(unit, &xlport_mac_ctrl, port);
            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlport_mac_ctrl, 0);
            ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlport_mac_ctrl, port); 
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM56150_A0 */
