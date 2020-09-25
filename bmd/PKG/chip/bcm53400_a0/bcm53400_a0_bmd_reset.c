/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53400_A0 == 1

#include <bmd/bmd.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/chip/bcm53400_a0_defs.h>

#include "bcm53400_a0_bmd.h"
#include "bcm53400_a0_internal.h"

#define RESET_SLEEP_USEC                100
#define PLL_LOCK_MSEC                   500

int
bcm53400_a0_xlport_reset(int unit, int port)
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
    
    return ioerr;
}

int
bcm53400_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    int wait_usec = 10000;
    int idx, port;
    cdk_pbmp_t pbmp;
    CMIC_CPS_RESETr_t cmic_cps;
    CMIC_SBUS_RING_MAPr_t sbus_ring_map;
    CMIC_SBUS_TIMEOUTr_t cmic_sbus_timeout;
    TOP_SOFT_RESET_REGr_t top_sreset;
    TOP_SOFT_RESET_REG_2r_t top_sreset_2;
    TOP_STRAP_STATUSr_t strap_status;
    TOP_CORE_PLL_CTRL4r_t top_core_pll4;
    TOP_XG_PLL_CTRL_1r_t xg_pll_ctrl1;
    TOP_XG_PLL_CTRL_6r_t xg_pll_ctrl6;
    TOP_MISC_CONTROL_1r_t misc_ctrl1;
    TOP_XG1_LCPLL_FBDIV_CTRLr_t lcpll_fbdiv_ctrl;
    PGW_CTRL_0r_t pgw_ctrl;
    XLPORT_MAC_CONTROLr_t xlport_mac_ctrl;
    SPEED_LIMIT_ENTRY_PHYSICAL_PORT_NUMBERr_t speed_limit;
    uint32_t disable_4x10q, disable_4x10;
    uint32_t disable_tsc;
    uint32_t ring_map[] = { 0x11112200, 0x00430001, 0x00005064, 
                            0x00000000, 0x00002222 };

    BMD_CHECK_UNIT(unit);

    /* Initialize endian mode for correct reset access */
    ioerr += cdk_xgsd_cmic_init(unit);

    /* Pull reset line */
    ioerr += READ_CMIC_CPS_RESETr(unit, &cmic_cps);
    CMIC_CPS_RESETr_CPS_RESETf_SET(cmic_cps, 1);
    ioerr += WRITE_CMIC_CPS_RESETr(unit, cmic_cps);

    /* Wait for all tables to initialize */
    BMD_SYS_USLEEP(wait_usec);

    /* Re-initialize endian mode after reset */
    ioerr += cdk_xgsd_cmic_init(unit);
    for (idx = 0; idx < COUNTOF(ring_map); idx++) {
        CMIC_SBUS_RING_MAPr_SET(sbus_ring_map, ring_map[idx]);
        ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, idx, sbus_ring_map);
    }
    /* Bring LCPLL out of reset */
    BMD_SYS_USLEEP(wait_usec);

    if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ_CHG) {
        if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ176) {
            ioerr += READ_TOP_CORE_PLL_CTRL4r(unit, &top_core_pll4);
            TOP_CORE_PLL_CTRL4r_MSTR_CH0_MDIVf_SET(top_core_pll4, 0x11);
            ioerr += WRITE_TOP_CORE_PLL_CTRL4r(unit, top_core_pll4);
        }
    }
    
    ioerr += READ_TOP_STRAP_STATUSr(unit, &strap_status);
    if (!((TOP_STRAP_STATUSr_STRAP_STATUSf_GET(strap_status) >> 9) & 0x1)) {
        ioerr += READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
        TOP_SOFT_RESET_REGr_TOP_LCPLL_SOFT_RESETf_SET(top_sreset, 1);
        ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);

        ioerr += READ_TOP_XG_PLL_CTRL_1r(unit, 1, &xg_pll_ctrl1);
        TOP_XG_PLL_CTRL_1r_PDIVf_SET(xg_pll_ctrl1, 3);
        ioerr += WRITE_TOP_XG_PLL_CTRL_1r(unit, 1, xg_pll_ctrl1);

        ioerr += READ_TOP_MISC_CONTROL_1r(unit, &misc_ctrl1);
        TOP_MISC_CONTROL_1r_CMIC_TO_XG_PLL1_SW_OVWRf_SET(misc_ctrl1, 1);
        ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, misc_ctrl1);

        ioerr += READ_TOP_XG1_LCPLL_FBDIV_CTRLr(unit, 0, &lcpll_fbdiv_ctrl);
        TOP_XG1_LCPLL_FBDIV_CTRLr_XG1_LCPLL_FBDIV_0f_SET(lcpll_fbdiv_ctrl, 0);
        ioerr += WRITE_TOP_XG1_LCPLL_FBDIV_CTRLr(unit, 0, lcpll_fbdiv_ctrl);

        ioerr += READ_TOP_XG1_LCPLL_FBDIV_CTRLr(unit, 1, &lcpll_fbdiv_ctrl);
        TOP_XG1_LCPLL_FBDIV_CTRLr_XG1_LCPLL_FBDIV_0f_SET(lcpll_fbdiv_ctrl, 0xf00);
        ioerr += WRITE_TOP_XG1_LCPLL_FBDIV_CTRLr(unit, 1, lcpll_fbdiv_ctrl);

        ioerr += READ_TOP_XG_PLL_CTRL_6r(unit, 1, &xg_pll_ctrl6);
        TOP_XG_PLL_CTRL_6r_MSC_CTRLf_SET(xg_pll_ctrl6, 0xcba4);
        ioerr += WRITE_TOP_XG_PLL_CTRL_6r(unit, 1, xg_pll_ctrl6);

        ioerr += READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
        TOP_SOFT_RESET_REGr_TOP_LCPLL_SOFT_RESETf_SET(top_sreset, 0);
        ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
        BMD_SYS_USLEEP(wait_usec); 
    }

    disable_tsc = ((TOP_STRAP_STATUSr_STRAP_STATUSf_GET(strap_status) >> 26) & 0x3F);
    if (disable_tsc) {
        bcm53400_a0_tsc_block_disable(unit, disable_tsc);
    }

    /* Bring port blocks out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_XLP0_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP1_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP2_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP3_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP4_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_GXP0_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec); 

    /* Bring network sync out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &top_sreset); 
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec); 

    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);

    CMIC_SBUS_TIMEOUTr_SET(cmic_sbus_timeout, 0x7d0);
    ioerr += WRITE_CMIC_SBUS_TIMEOUTr(unit, cmic_sbus_timeout);

    /* Bring IP, EP, and MMU blocks out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(top_sreset, 1); 
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(top_sreset, 1);
    ioerr +=  WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec);

    /* Unused TSCx should be disabled */
    disable_4x10q = 1;
    disable_4x10 = 0x1f;
    bcm53400_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (port == 2) {
            disable_4x10q = 0;
        } else if (port >= 18) {
            disable_4x10 &= ~(1 << ((port - 18) >> 2));
        }
    }
    ioerr += READ_PGW_CTRL_0r(unit, &pgw_ctrl);
    PGW_CTRL_0r_SW_PM4X10_DISABLEf_SET(pgw_ctrl, disable_4x10); 
    PGW_CTRL_0r_SW_PM4X10Q_DISABLEf_SET(pgw_ctrl, disable_4x10q);
    ioerr +=  WRITE_PGW_CTRL_0r(unit, pgw_ctrl);
    BMD_SYS_USLEEP(wait_usec);

    /* Speed limit settings for some SKUs option */
    if ((CDK_XGSD_FLAGS(unit) & CHIP_FLAG_MIXED) &&
        ((CDK_CHIP_CONFIG(unit) & DCFG_H10G) || 
         (CDK_CHIP_CONFIG(unit) & DCFG_L10G))) {
        int pport[] = {  0x2,  0x3,  0x4,  0x5, 0x1b, 0x13, 0x14, 0x15, 
                        0x1c, 0x17, 0x18, 0x19, 0x1a, 0x12, 0x16, 0x1d, 
                        0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25};
        
        for (idx = 0; idx < COUNTOF(pport); idx++) {
            SPEED_LIMIT_ENTRY_PHYSICAL_PORT_NUMBERr_SET(speed_limit, pport[idx]);
            WRITE_SPEED_LIMIT_ENTRY_PHYSICAL_PORT_NUMBERr(unit, idx, speed_limit);
        }
    } else if ((CDK_XGSD_FLAGS(unit) & CHIP_FLAG_X10G) &&
               (CDK_CHIP_CONFIG(unit) & DCFG_XAUI)) {
        int pport[] = { 0x24,  0x3,  0x4,  0x5, 0x25, 0x13, 0x14, 0x15, 
                        0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 
                        0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,  0x2, 0x12};
        
        for (idx = 0; idx < COUNTOF(pport); idx++) {
            SPEED_LIMIT_ENTRY_PHYSICAL_PORT_NUMBERr_SET(speed_limit, pport[idx]);
            WRITE_SPEED_LIMIT_ENTRY_PHYSICAL_PORT_NUMBERr(unit, idx, speed_limit);
        }
    } else if ((CDK_XGSD_FLAGS(unit) & CHIP_FLAG_GE) &&
               (CDK_CHIP_CONFIG(unit) & DCFG_2QSGMII)) {
        int pport[] = {  0x2,  0x3,  0x4,  0x5, 0x12, 0x13, 0x14, 0x15, 
                        0x16, 0x17, 0x18, 0x19, 0x1a, 0x1e, 0x1c, 0x20, 
                        0x1b, 0x1f, 0x1d, 0x21, 0x22, 0x23, 0x24, 0x25};
        
        for (idx = 0; idx < COUNTOF(pport); idx++) {
            SPEED_LIMIT_ENTRY_PHYSICAL_PORT_NUMBERr_SET(speed_limit, pport[idx]);
            WRITE_SPEED_LIMIT_ENTRY_PHYSICAL_PORT_NUMBERr(unit, idx, speed_limit);
        }
    }

    bcm53400_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (XLPORT_SUBPORT(unit, port) == 0) {
            if ((port == 2) || (port >= 18)) {
                ioerr += bcm53400_a0_xlport_reset(unit, port);
            }
        }
    }
    
    bcm53400_a0_xport_pbmp_get(unit, XPORT_FLAG_XLPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        /* We only need to reset first port in each block */
        if (XLPORT_SUBPORT(unit, port) == 0) {
            ioerr += READ_XLPORT_MAC_CONTROLr(unit, &xlport_mac_ctrl, port);
            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlport_mac_ctrl, 1);
            ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlport_mac_ctrl, port);
    
            BMD_SYS_USLEEP(10);
            
            ioerr += READ_XLPORT_MAC_CONTROLr(unit, &xlport_mac_ctrl, port);
            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlport_mac_ctrl, 0);
            ioerr += WRITE_XLPORT_MAC_CONTROLr(unit, xlport_mac_ctrl, port); 
        }
    }

    return ioerr ? CDK_E_IO : rv;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53400_A0 */
