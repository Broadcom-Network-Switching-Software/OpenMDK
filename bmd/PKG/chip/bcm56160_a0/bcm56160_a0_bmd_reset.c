/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56160_A0 == 1

#include <bmd/bmd.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/chip/bcm56160_a0_defs.h>

#include "bcm56160_a0_bmd.h"
#include "bcm56160_a0_internal.h"

#define RESET_SLEEP_USEC                100
#define PLL_LOCK_MSEC                   500

static int
_gpio_reset(int unit, int pin, int output, int val)
{
    int ioerr = 0;
    CMIC_GP_OUT_ENr_t gp_out_en;
    CMIC_GP_DATA_OUTr_t gp_data_out;
    uint32_t fval = 0;
    uint8_t mask = 0xFF;

    mask &= ~(1 << pin);

    ioerr += READ_CMIC_GP_OUT_ENr(unit, &gp_out_en);
    fval = CMIC_GP_OUT_ENr_OUT_ENABLEf_GET(gp_out_en);
    fval &= ~(1 << pin);
    if (output) {
        fval |= 1 << pin;
    }
    CMIC_GP_OUT_ENr_OUT_ENABLEf_SET(gp_out_en, fval);
    ioerr += WRITE_CMIC_GP_OUT_ENr(unit, gp_out_en);

    if (output) {
        ioerr += READ_CMIC_GP_DATA_OUTr(unit, &gp_data_out);
        fval = CMIC_GP_DATA_OUTr_DATA_OUTf_GET(gp_data_out);
        fval &= ~(1 << pin);
        if (output) {
            fval |= 1 << pin;
        }
        CMIC_GP_DATA_OUTr_DATA_OUTf_SET(gp_data_out, fval);
        ioerr += WRITE_CMIC_GP_DATA_OUTr(unit, gp_data_out);
    }

    return ioerr;
}

static int
_pll_config(unit)
{
    int ioerr = 0;
    static struct soc_pll_param_s {
        uint32_t ref_freq;
        uint32_t ndiv_int;
        uint32_t ndiv_frac;
        uint32_t pdiv;
        uint32_t mdiv;
        uint32_t ka;
        uint32_t ki;
        uint32_t kp;
        uint32_t vco_div2;
    } _pll_cfg[] = {
        { 0, 100, 0, 5,   1, 2, 2, 3, 1}, /* ts_pll for 25Mhz internal clk */
        { 0,  50, 0, 1,   5, 2, 2, 3, 1}, /* ts_pll for 50Mhz internal clk */
        { 0, 120, 0, 1, 150, 4, 1, 8, 0}, /* bs_pll for 25Mhz internal clk */
        { 0,  60, 0, 1, 150, 4, 1, 8, 0}  /* bs_pll for 50MHZ internal clk */
    };
    int strap_val, lcpll1_ref, iclock_ref;
    int ts_idx, bs_idx;
    int wait_usec = 10000;
    int bs_ndiv_high, bs_ndiv_low;
    TOP_STRAP_STATUSr_t strap_status;
    TOP_STRAP_STATUS_1r_t strap_status_1;
    TOP_PLL_BYP_AND_REFCLK_CONTROLr_t top_pll_ctrl;
    TOP_MISC_CONTROL_1r_t top_misc_ctrl_1;
    TOP_TS_PLL_CTRL_0r_t top_ts_pll_0;
    TOP_TS_PLL_CTRL_2r_t top_ts_pll_2;
    TOP_TS_PLL_CTRL_3r_t top_ts_pll_3;
    TOP_TS_PLL_CTRL_4r_t top_ts_pll_4;
    CMIC_BS0_CONFIGr_t cmic_bs0;
    CMIC_BS1_CONFIGr_t cmic_bs1;
    TOP_SOFT_RESET_REGr_t top_sreset;
    TOP_SOFT_RESET_REG_2r_t top_sreset_2;
    TOP_BS_PLL0_CTRL_0r_t top_bs_pll0_ctrl_0;
    TOP_BS_PLL0_CTRL_1r_t top_bs_pll0_ctrl_1;
    TOP_BS_PLL0_CTRL_3r_t top_bs_pll0_ctrl_3;
    TOP_BS_PLL0_CTRL_5r_t top_bs_pll0_ctrl_5;
    TOP_BS_PLL0_CTRL_6r_t top_bs_pll0_ctrl_6;
    TOP_BS_PLL0_CTRL_7r_t top_bs_pll0_ctrl_7;
    TOP_BS_PLL1_CTRL_0r_t top_bs_pll1_ctrl_0;
    TOP_BS_PLL1_CTRL_1r_t top_bs_pll1_ctrl_1;
    TOP_BS_PLL1_CTRL_3r_t top_bs_pll1_ctrl_3;
    TOP_BS_PLL1_CTRL_5r_t top_bs_pll1_ctrl_5;
    TOP_BS_PLL1_CTRL_6r_t top_bs_pll1_ctrl_6;
    TOP_BS_PLL1_CTRL_7r_t top_bs_pll1_ctrl_7;
    TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr_t top_bs0_lcpll_fbdiv;
    TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr_t top_bs1_lcpll_fbdiv;
    TOP_XG_PLL_CTRL_0r_t top_xg_pll_0;
    TOP_XG_PLL_CTRL_1r_t top_xg_pll_1;
    TOP_XG_PLL_CTRL_3r_t top_xg_pll_3;
    TOP_XG_PLL_CTRL_5r_t top_xg_pll_5;
    TOP_XG_PLL_CTRL_6r_t top_xg_pll_6;
    TOP_XG_PLL_CTRL_7r_t top_xg_pll_7;
    TOP_XG0_LCPLL_FBDIV_CTRLr_t top_xg0_lcpll_fbdiv;
    TOP_XG1_LCPLL_FBDIV_CTRLr_t top_xg1_lcpll_fbdiv;

    ioerr += READ_TOP_STRAP_STATUSr(unit, &strap_status);
    strap_val = TOP_STRAP_STATUSr_STRAP_STATUSf_GET(strap_status);
    lcpll1_ref = ((strap_val & (1<<9)) != 0);

    ioerr += READ_TOP_STRAP_STATUS_1r(unit, &strap_status_1);
    strap_val = TOP_STRAP_STATUS_1r_STRAP_STATUSf_GET(strap_status_1);
    iclock_ref = ((strap_val) & 1);

    /* Only for internal reference */
    /* Set TS_PLL_CLK_IN_SEL based on reference frequency. */
    READ_TOP_PLL_BYP_AND_REFCLK_CONTROLr(unit, &top_pll_ctrl);
    TOP_PLL_BYP_AND_REFCLK_CONTROLr_TS_PLL_REFCLK_SELf_SET(top_pll_ctrl, 0);
    ioerr += WRITE_TOP_PLL_BYP_AND_REFCLK_CONTROLr(unit, top_pll_ctrl);

    /* For internal reference, 25MHz or 50Mhz depending upon
     * strap_xtal_freq_sel
     */
    ts_idx = 0;
    bs_idx = 2;
    if (iclock_ref) {
        ts_idx = 1;
        bs_idx = 3;
    }

    /* Enable software overwrite of TimeSync PLL settings. */
    READ_TOP_MISC_CONTROL_1r(unit, &top_misc_ctrl_1);
    TOP_MISC_CONTROL_1r_CMIC_TO_TS_PLL_LOADf_SET(top_misc_ctrl_1, 1);
    ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_ctrl_1);

    READ_TOP_TS_PLL_CTRL_0r(unit, &top_ts_pll_0);
    TOP_TS_PLL_CTRL_0r_VCO_DIV2f_SET(top_ts_pll_0, _pll_cfg[ts_idx].vco_div2);
    ioerr += WRITE_TOP_TS_PLL_CTRL_0r(unit, top_ts_pll_0);

    READ_TOP_TS_PLL_CTRL_4r(unit, &top_ts_pll_4);
    TOP_TS_PLL_CTRL_4r_KAf_SET(top_ts_pll_4, _pll_cfg[ts_idx].ka);
    TOP_TS_PLL_CTRL_4r_KIf_SET(top_ts_pll_4, _pll_cfg[ts_idx].ki);
    TOP_TS_PLL_CTRL_4r_KPf_SET(top_ts_pll_4, _pll_cfg[ts_idx].kp);
    ioerr += WRITE_TOP_TS_PLL_CTRL_4r(unit, top_ts_pll_4);

    READ_TOP_TS_PLL_CTRL_3r(unit, &top_ts_pll_3);
    TOP_TS_PLL_CTRL_3r_NDIV_INTf_SET(top_ts_pll_3, _pll_cfg[ts_idx].ndiv_int);
    TOP_TS_PLL_CTRL_3r_NDIV_FRACf_SET(top_ts_pll_3, _pll_cfg[ts_idx].ndiv_frac);
    ioerr += WRITE_TOP_TS_PLL_CTRL_3r(unit, top_ts_pll_3);

    READ_TOP_TS_PLL_CTRL_2r(unit, &top_ts_pll_2);
    TOP_TS_PLL_CTRL_2r_PDIVf_SET(top_ts_pll_2, _pll_cfg[ts_idx].pdiv);
    TOP_TS_PLL_CTRL_2r_CH0_MDIVf_SET(top_ts_pll_2, _pll_cfg[ts_idx].mdiv);
    ioerr += WRITE_TOP_TS_PLL_CTRL_2r(unit, top_ts_pll_2);

    /* Strobe channel-0 load-enable to set divisors. */
    TOP_TS_PLL_CTRL_2r_LOAD_EN_CH0f_SET(top_ts_pll_2, 0);
    ioerr += WRITE_TOP_TS_PLL_CTRL_2r(unit, top_ts_pll_2);

    TOP_TS_PLL_CTRL_2r_LOAD_EN_CH0f_SET(top_ts_pll_2, 1);
    ioerr += WRITE_TOP_TS_PLL_CTRL_2r(unit, top_ts_pll_2);

    if (!(CDK_XGSD_FLAGS(unit) & CHIP_FLAG_NO_BS)) {
        READ_CMIC_BS0_CONFIGr(unit, &cmic_bs0);
        CMIC_BS0_CONFIGr_BS_CLK_OUTPUT_ENABLEf_SET(cmic_bs0, 0);
        ioerr += WRITE_CMIC_BS0_CONFIGr(unit, cmic_bs0);

        READ_CMIC_BS1_CONFIGr(unit, &cmic_bs1);
        CMIC_BS1_CONFIGr_BS_CLK_OUTPUT_ENABLEf_SET(cmic_bs1, 0);
        ioerr += WRITE_CMIC_BS1_CONFIGr(unit, cmic_bs1);

        /* BSPLL0 has not been configured, so reset/configure both BSPLL0 and BSPLL1 */
        READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
        TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_RST_Lf_SET(top_sreset_2, 0);
        TOP_SOFT_RESET_REG_2r_TOP_BS_PLL1_RST_Lf_SET(top_sreset_2, 0);
        TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_POST_RST_Lf_SET(top_sreset_2, 0);
        TOP_SOFT_RESET_REG_2r_TOP_BS_PLL1_POST_RST_Lf_SET(top_sreset_2, 0);
        ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);

        /* Set RATE_MANGER_MODE to 1 to enable on-the-fly setting of NDIV */
        READ_TOP_BS_PLL0_CTRL_3r(unit, &top_bs_pll0_ctrl_3);
        TOP_BS_PLL0_CTRL_3r_RATE_MANAGER_MODEf_SET(top_bs_pll0_ctrl_3, 1);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_3r(unit, top_bs_pll0_ctrl_3);

        READ_TOP_BS_PLL1_CTRL_3r(unit, &top_bs_pll1_ctrl_3);
        TOP_BS_PLL1_CTRL_3r_RATE_MANAGER_MODEf_SET(top_bs_pll1_ctrl_3, 1);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_3r(unit, top_bs_pll1_ctrl_3);

        /* Only for internal reference */
        /* Set BS_PLL_CLK_IN_SEL based on reference frequency.  If it is 0, use the internal reference */
        READ_TOP_PLL_BYP_AND_REFCLK_CONTROLr(unit, &top_pll_ctrl);
        TOP_PLL_BYP_AND_REFCLK_CONTROLr_BS_PLL0_REFCLK_SELf_SET(top_pll_ctrl, 0);
        TOP_PLL_BYP_AND_REFCLK_CONTROLr_BS_PLL1_REFCLK_SELf_SET(top_pll_ctrl, 0);
        ioerr += WRITE_TOP_PLL_BYP_AND_REFCLK_CONTROLr(unit, top_pll_ctrl);

        READ_TOP_BS_PLL0_CTRL_6r(unit, &top_bs_pll0_ctrl_6);
        TOP_BS_PLL0_CTRL_6r_LDO_CTRLf_SET(top_bs_pll0_ctrl_6, 0x22);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_6r(unit, top_bs_pll0_ctrl_6);

        READ_TOP_BS_PLL0_CTRL_7r(unit, &top_bs_pll0_ctrl_7);
        TOP_BS_PLL0_CTRL_7r_FREQ_DOUBLER_ONf_SET(top_bs_pll0_ctrl_7, 0);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_7r(unit, top_bs_pll0_ctrl_7);

        READ_TOP_BS_PLL0_CTRL_6r(unit, &top_bs_pll0_ctrl_6);
        TOP_BS_PLL0_CTRL_6r_MSC_CTRLf_SET(top_bs_pll0_ctrl_6,
                                          (iclock_ref == 0) ? 0x0022: 0x1022);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_6r(unit, top_bs_pll0_ctrl_6);

        READ_TOP_BS_PLL0_CTRL_7r(unit, &top_bs_pll0_ctrl_7);
        TOP_BS_PLL0_CTRL_7r_VCO_CONT_ADJf_SET(top_bs_pll0_ctrl_7, 1);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_7r(unit, top_bs_pll0_ctrl_7);

        READ_TOP_BS_PLL0_CTRL_3r(unit, &top_bs_pll0_ctrl_3);
        TOP_BS_PLL0_CTRL_3r_VCO_CURf_SET(top_bs_pll0_ctrl_3, 0);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_3r(unit, top_bs_pll0_ctrl_3);

        READ_TOP_BS_PLL0_CTRL_5r(unit, &top_bs_pll0_ctrl_5);
        TOP_BS_PLL0_CTRL_5r_VCO_GAINf_SET(top_bs_pll0_ctrl_5, 15);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_5r(unit, top_bs_pll0_ctrl_5);

        READ_TOP_MISC_CONTROL_1r(unit, &top_misc_ctrl_1);
        TOP_MISC_CONTROL_1r_CMIC_TO_BS_PLL0_SW_OVWRf_SET(top_misc_ctrl_1, 1);
        ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_ctrl_1);

        READ_TOP_BS_PLL0_CTRL_5r(unit, &top_bs_pll0_ctrl_5);
        TOP_BS_PLL0_CTRL_5r_CP1f_SET(top_bs_pll0_ctrl_5,
                                     (iclock_ref == 0) ? 0 : 1);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_5r(unit, top_bs_pll0_ctrl_5);
        TOP_BS_PLL0_CTRL_5r_CPf_SET(top_bs_pll0_ctrl_5,
                                     (iclock_ref == 0) ? 0 : 3);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_5r(unit, top_bs_pll0_ctrl_5);
        TOP_BS_PLL0_CTRL_5r_CZf_SET(top_bs_pll0_ctrl_5, 3);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_5r(unit, top_bs_pll0_ctrl_5);
        TOP_BS_PLL0_CTRL_5r_RPf_SET(top_bs_pll0_ctrl_5,
                                     (iclock_ref == 0) ? 0 : 7);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_5r(unit, top_bs_pll0_ctrl_5);
        TOP_BS_PLL0_CTRL_5r_RZf_SET(top_bs_pll0_ctrl_5,
                                     (iclock_ref == 0) ? 6 : 2);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_5r(unit, top_bs_pll0_ctrl_5);
        TOP_BS_PLL0_CTRL_5r_ICPf_SET(top_bs_pll0_ctrl_5,
                                     (iclock_ref == 0) ? 20 : 10);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_5r(unit, top_bs_pll0_ctrl_5);

        READ_TOP_BS_PLL0_CTRL_7r(unit, &top_bs_pll0_ctrl_7);
        TOP_BS_PLL0_CTRL_7r_CPPf_SET(top_bs_pll0_ctrl_7, 0x80);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_7r(unit, top_bs_pll0_ctrl_7);

        READ_TOP_BS_PLL0_CTRL_6r(unit, &top_bs_pll0_ctrl_6);
        TOP_BS_PLL0_CTRL_6r_LDO_CTRLf_SET(top_bs_pll0_ctrl_6, 0x22);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_6r(unit, top_bs_pll0_ctrl_6);

        READ_TOP_BS_PLL1_CTRL_7r(unit, &top_bs_pll1_ctrl_7);
        TOP_BS_PLL1_CTRL_7r_FREQ_DOUBLER_ONf_SET(top_bs_pll1_ctrl_7, 0);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_7r(unit, top_bs_pll1_ctrl_7);

        READ_TOP_BS_PLL1_CTRL_6r(unit, &top_bs_pll1_ctrl_6);
        TOP_BS_PLL1_CTRL_6r_MSC_CTRLf_SET(top_bs_pll1_ctrl_6,
                                          (iclock_ref == 0) ? 0x0022: 0x1022);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_6r(unit, top_bs_pll1_ctrl_6);

        READ_TOP_BS_PLL1_CTRL_7r(unit, &top_bs_pll1_ctrl_7);
        TOP_BS_PLL1_CTRL_7r_VCO_CONT_ADJf_SET(top_bs_pll1_ctrl_7, 1);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_7r(unit, top_bs_pll1_ctrl_7);

        READ_TOP_BS_PLL1_CTRL_3r(unit, &top_bs_pll1_ctrl_3);
        TOP_BS_PLL1_CTRL_3r_VCO_CURf_SET(top_bs_pll1_ctrl_3, 0);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_3r(unit, top_bs_pll1_ctrl_3);

        READ_TOP_BS_PLL1_CTRL_5r(unit, &top_bs_pll1_ctrl_5);
        TOP_BS_PLL1_CTRL_5r_VCO_GAINf_SET(top_bs_pll1_ctrl_5, 15);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_5r(unit, top_bs_pll1_ctrl_5);

        READ_TOP_MISC_CONTROL_1r(unit, &top_misc_ctrl_1);
        TOP_MISC_CONTROL_1r_CMIC_TO_BS_PLL1_SW_OVWRf_SET(top_misc_ctrl_1, 1);
        ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_ctrl_1);

        READ_TOP_BS_PLL1_CTRL_5r(unit, &top_bs_pll1_ctrl_5);
        TOP_BS_PLL1_CTRL_5r_CP1f_SET(top_bs_pll1_ctrl_5, 1);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_5r(unit, top_bs_pll1_ctrl_5);
        TOP_BS_PLL1_CTRL_5r_CPf_SET(top_bs_pll1_ctrl_5, 3);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_5r(unit, top_bs_pll1_ctrl_5);
        TOP_BS_PLL1_CTRL_5r_CZf_SET(top_bs_pll1_ctrl_5, 3);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_5r(unit, top_bs_pll1_ctrl_5);
        TOP_BS_PLL1_CTRL_5r_RPf_SET(top_bs_pll1_ctrl_5,
                                    (iclock_ref == 0) ? 0 : 7);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_5r(unit, top_bs_pll1_ctrl_5);
        TOP_BS_PLL1_CTRL_5r_RZf_SET(top_bs_pll1_ctrl_5,
                                    (iclock_ref == 0) ? 6 : 2);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_5r(unit, top_bs_pll1_ctrl_5);
        TOP_BS_PLL1_CTRL_5r_ICPf_SET(top_bs_pll1_ctrl_5,
                                    (iclock_ref == 0) ? 20 : 10);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_5r(unit, top_bs_pll1_ctrl_5);

        READ_TOP_BS_PLL1_CTRL_7r(unit, &top_bs_pll1_ctrl_7);
        TOP_BS_PLL1_CTRL_7r_CPPf_SET(top_bs_pll1_ctrl_7, 0x80);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_7r(unit, top_bs_pll1_ctrl_7);

        READ_TOP_BS_PLL0_CTRL_1r(unit, &top_bs_pll0_ctrl_1);
        TOP_BS_PLL0_CTRL_1r_KAf_SET(top_bs_pll0_ctrl_1, _pll_cfg[bs_idx].ka);
        TOP_BS_PLL0_CTRL_1r_KIf_SET(top_bs_pll0_ctrl_1, _pll_cfg[bs_idx].ki);
        TOP_BS_PLL0_CTRL_1r_KPf_SET(top_bs_pll0_ctrl_1, _pll_cfg[bs_idx].kp);
        TOP_BS_PLL0_CTRL_1r_PDIVf_SET(top_bs_pll0_ctrl_1, _pll_cfg[bs_idx].pdiv);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_1r(unit, top_bs_pll0_ctrl_1);

        READ_TOP_BS_PLL1_CTRL_1r(unit, &top_bs_pll1_ctrl_1);
        TOP_BS_PLL1_CTRL_1r_KAf_SET(top_bs_pll1_ctrl_1, _pll_cfg[bs_idx].ka);
        TOP_BS_PLL1_CTRL_1r_KIf_SET(top_bs_pll1_ctrl_1, _pll_cfg[bs_idx].ki);
        TOP_BS_PLL1_CTRL_1r_KPf_SET(top_bs_pll1_ctrl_1, _pll_cfg[bs_idx].kp);
        TOP_BS_PLL1_CTRL_1r_PDIVf_SET(top_bs_pll1_ctrl_1, _pll_cfg[bs_idx].pdiv);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_1r(unit, top_bs_pll1_ctrl_1);

        /* ndiv_int: 10 bits.  ndiv_frac: 24 bits, stored in upper 24
         * ndiv_int  =  BROAD_SYNC0_LCPLL_FBDIV_1[15:6];
         * ndiv_frac = {BROAD_SYNC0_LCPLL_FBDIV_1[5:0],
         *                      BROAD_SYNC0_LCPLL_FBDIV_0[31:14]};
         * So FBDIV_1 = (ndiv_int << 6) | (ndiv_frac >> (32-6))
         *    FBDIV_2 = (ndiv_frac >> 6)
         */
        bs_ndiv_high = ((_pll_cfg[bs_idx].ndiv_int << 6) |
                        ((_pll_cfg[bs_idx].ndiv_frac >> (32 - 6)) & 0x3f));
        bs_ndiv_low = (_pll_cfg[bs_idx].ndiv_frac << 8);

        READ_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr(unit, 1, &top_bs0_lcpll_fbdiv);
        TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr_BROAD_SYNC0_LCPLL_FBDIV_0f_SET(top_bs0_lcpll_fbdiv, bs_ndiv_high);
        ioerr += WRITE_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr(unit, 1, top_bs0_lcpll_fbdiv);

        READ_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr(unit, 0, &top_bs0_lcpll_fbdiv);
        TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr_BROAD_SYNC0_LCPLL_FBDIV_0f_SET(top_bs0_lcpll_fbdiv, bs_ndiv_low);
        ioerr += WRITE_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRLr(unit, 0, top_bs0_lcpll_fbdiv);

        READ_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr(unit, 1, &top_bs1_lcpll_fbdiv);
        TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr_BROAD_SYNC1_LCPLL_FBDIV_0f_SET(top_bs1_lcpll_fbdiv, bs_ndiv_high);
        ioerr += WRITE_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr(unit, 1, top_bs1_lcpll_fbdiv);

        READ_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr(unit, 0, &top_bs1_lcpll_fbdiv);
        TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr_BROAD_SYNC1_LCPLL_FBDIV_0f_SET(top_bs1_lcpll_fbdiv, bs_ndiv_low);
        ioerr += WRITE_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRLr(unit, 0, top_bs1_lcpll_fbdiv);

        READ_TOP_BS_PLL0_CTRL_0r(unit, &top_bs_pll0_ctrl_0);
        TOP_BS_PLL0_CTRL_0r_CH0_MDIVf_SET(top_bs_pll0_ctrl_0, _pll_cfg[bs_idx].mdiv);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_0r(unit, top_bs_pll0_ctrl_0);

        READ_TOP_BS_PLL1_CTRL_0r(unit, &top_bs_pll1_ctrl_0);
        TOP_BS_PLL1_CTRL_0r_CH0_MDIVf_SET(top_bs_pll1_ctrl_0, _pll_cfg[bs_idx].mdiv);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_0r(unit, top_bs_pll1_ctrl_0);

        READ_TOP_BS_PLL0_CTRL_1r(unit, &top_bs_pll0_ctrl_1);
        TOP_BS_PLL0_CTRL_1r_LOAD_EN_CHf_SET(top_bs_pll0_ctrl_1, 0);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_1r(unit, top_bs_pll0_ctrl_1);
        TOP_BS_PLL0_CTRL_1r_LOAD_EN_CHf_SET(top_bs_pll0_ctrl_1, 1);
        ioerr += WRITE_TOP_BS_PLL0_CTRL_1r(unit, top_bs_pll0_ctrl_1);

        READ_TOP_BS_PLL1_CTRL_1r(unit, &top_bs_pll1_ctrl_1);
        TOP_BS_PLL1_CTRL_1r_LOAD_EN_CHf_SET(top_bs_pll1_ctrl_1, 0);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_1r(unit, top_bs_pll1_ctrl_1);
        TOP_BS_PLL1_CTRL_1r_LOAD_EN_CHf_SET(top_bs_pll1_ctrl_1, 1);
        ioerr += WRITE_TOP_BS_PLL1_CTRL_1r(unit, top_bs_pll1_ctrl_1);

        READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
        TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_RST_Lf_SET(top_sreset_2, 1);
        ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
        TOP_SOFT_RESET_REG_2r_TOP_BS_PLL1_RST_Lf_SET(top_sreset_2, 1);
        ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);

        BMD_SYS_USLEEP(wait_usec);
        READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
        TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_POST_RST_Lf_SET(top_sreset_2, 1);
        TOP_SOFT_RESET_REG_2r_TOP_BS_PLL1_POST_RST_Lf_SET(top_sreset_2, 1);
        ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    }

    /* Initialize XGPLLs */
    READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_LCPLL_SOFT_RESETf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);

    /* XGPLL0 */
    READ_TOP_XG_PLL_CTRL_1r(unit, 0, &top_xg_pll_1);
    TOP_XG_PLL_CTRL_1r_PDIVf_SET(top_xg_pll_1, 1);
    ioerr += WRITE_TOP_XG_PLL_CTRL_1r(unit, 0, top_xg_pll_1);

    READ_TOP_XG_PLL_CTRL_0r(unit, 0, &top_xg_pll_0);
    TOP_XG_PLL_CTRL_0r_CH0_MDIVf_SET(top_xg_pll_0, 0x14);
    ioerr += WRITE_TOP_XG_PLL_CTRL_0r(unit, 0, top_xg_pll_0);

    READ_TOP_XG_PLL_CTRL_6r(unit, 0, &top_xg_pll_6);
    TOP_XG_PLL_CTRL_6r_LDO_CTRLf_SET(top_xg_pll_6, 0x22);
    ioerr += WRITE_TOP_XG_PLL_CTRL_6r(unit, 0, top_xg_pll_6);

    /* Use strap for setting of FREQ_DOUBLER */
    READ_TOP_XG_PLL_CTRL_7r(unit, 0, &top_xg_pll_7);
    TOP_XG_PLL_CTRL_7r_FREQ_DOUBLER_ONf_SET(top_xg_pll_7, (iclock_ref == 0));
    TOP_XG_PLL_CTRL_7r_VCO_CONT_ADJf_SET(top_xg_pll_7, 1);
    ioerr += WRITE_TOP_XG_PLL_CTRL_7r(unit, 0, top_xg_pll_7);

    READ_TOP_XG_PLL_CTRL_3r(unit, 0, &top_xg_pll_3);
    TOP_XG_PLL_CTRL_3r_VCO_CURf_SET(top_xg_pll_3, 0);
    ioerr += WRITE_TOP_XG_PLL_CTRL_3r(unit, 0, top_xg_pll_3);

    READ_TOP_XG_PLL_CTRL_5r(unit, 0, &top_xg_pll_5);
    TOP_XG_PLL_CTRL_5r_VCO_GAINf_SET(top_xg_pll_5, 15);
    ioerr += WRITE_TOP_XG_PLL_CTRL_5r(unit, 0, top_xg_pll_5);

    READ_TOP_MISC_CONTROL_1r(unit, &top_misc_ctrl_1);
    TOP_MISC_CONTROL_1r_CMIC_TO_XG_PLL0_SW_OVWRf_SET(top_misc_ctrl_1, 1);
    ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_ctrl_1);

    READ_TOP_XG0_LCPLL_FBDIV_CTRLr(unit, 0, &top_xg0_lcpll_fbdiv);
    TOP_XG0_LCPLL_FBDIV_CTRLr_XG0_LCPLL_FBDIV_0f_SET(top_xg0_lcpll_fbdiv, 0);
    ioerr += WRITE_TOP_XG0_LCPLL_FBDIV_CTRLr(unit, 0, top_xg0_lcpll_fbdiv);

    READ_TOP_XG0_LCPLL_FBDIV_CTRLr(unit, 1, &top_xg0_lcpll_fbdiv);
    TOP_XG0_LCPLL_FBDIV_CTRLr_XG0_LCPLL_FBDIV_0f_SET(top_xg0_lcpll_fbdiv, 0x0fa0);
    ioerr += WRITE_TOP_XG0_LCPLL_FBDIV_CTRLr(unit, 1, top_xg0_lcpll_fbdiv);

    READ_TOP_XG_PLL_CTRL_5r(unit, 0, &top_xg_pll_5);
    TOP_XG_PLL_CTRL_5r_CP1f_SET(top_xg_pll_5, 1);
    TOP_XG_PLL_CTRL_5r_CPf_SET(top_xg_pll_5, 3);
    TOP_XG_PLL_CTRL_5r_CZf_SET(top_xg_pll_5, 3);
    TOP_XG_PLL_CTRL_5r_RPf_SET(top_xg_pll_5, 7);
    TOP_XG_PLL_CTRL_5r_RZf_SET(top_xg_pll_5, 2);
    TOP_XG_PLL_CTRL_5r_ICPf_SET(top_xg_pll_5, 10);
    ioerr += WRITE_TOP_XG_PLL_CTRL_5r(unit, 0, top_xg_pll_5);

    READ_TOP_XG_PLL_CTRL_7r(unit, 0, &top_xg_pll_7);
    TOP_XG_PLL_CTRL_7r_CPPf_SET(top_xg_pll_7, 0x80);
    ioerr += WRITE_TOP_XG_PLL_CTRL_7r(unit, 0, top_xg_pll_7);

    /* XGPLL1 */
    if (lcpll1_ref) {
        READ_TOP_XG_PLL_CTRL_1r(unit, 1, &top_xg_pll_1);
        TOP_XG_PLL_CTRL_1r_PDIVf_SET(top_xg_pll_1, 1);
        ioerr += WRITE_TOP_XG_PLL_CTRL_1r(unit, 1, top_xg_pll_1);

        READ_TOP_XG_PLL_CTRL_0r(unit, 1, &top_xg_pll_0);
        TOP_XG_PLL_CTRL_0r_CH0_MDIVf_SET(top_xg_pll_0, 0x14);
        ioerr += WRITE_TOP_XG_PLL_CTRL_0r(unit, 1, top_xg_pll_0);

        /* Use strap for setting of FREQ_DOUBLER */
        READ_TOP_XG_PLL_CTRL_7r(unit, 1, &top_xg_pll_7);
        TOP_XG_PLL_CTRL_7r_FREQ_DOUBLER_ONf_SET(top_xg_pll_7, (iclock_ref == 0));
        ioerr += WRITE_TOP_XG_PLL_CTRL_7r(unit, 1, top_xg_pll_7);

        READ_TOP_XG_PLL_CTRL_6r(unit, 1, &top_xg_pll_6);
        TOP_XG_PLL_CTRL_6r_MSC_CTRLf_SET(top_xg_pll_6, 0x00a2);
        TOP_XG_PLL_CTRL_6r_LDO_CTRLf_SET(top_xg_pll_6, 0x22);
        ioerr += WRITE_TOP_XG_PLL_CTRL_6r(unit, 1, top_xg_pll_6);

        READ_TOP_XG_PLL_CTRL_7r(unit, 1, &top_xg_pll_7);
        TOP_XG_PLL_CTRL_7r_VCO_CONT_ADJf_SET(top_xg_pll_7, 1);
        ioerr += WRITE_TOP_XG_PLL_CTRL_7r(unit, 1, top_xg_pll_7);

        READ_TOP_XG_PLL_CTRL_3r(unit, 1, &top_xg_pll_3);
        TOP_XG_PLL_CTRL_3r_VCO_CURf_SET(top_xg_pll_3, 0);
        ioerr += WRITE_TOP_XG_PLL_CTRL_3r(unit, 1, top_xg_pll_3);

        READ_TOP_XG_PLL_CTRL_5r(unit, 1, &top_xg_pll_5);
        TOP_XG_PLL_CTRL_5r_VCO_GAINf_SET(top_xg_pll_5, 15);
        ioerr += WRITE_TOP_XG_PLL_CTRL_5r(unit, 1, top_xg_pll_5);

        READ_TOP_MISC_CONTROL_1r(unit, &top_misc_ctrl_1);
        TOP_MISC_CONTROL_1r_CMIC_TO_XG_PLL1_SW_OVWRf_SET(top_misc_ctrl_1, 1);
        ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_ctrl_1);

        READ_TOP_XG_PLL_CTRL_5r(unit, 1, &top_xg_pll_5);
        TOP_XG_PLL_CTRL_5r_CP1f_SET(top_xg_pll_5, 1);
        TOP_XG_PLL_CTRL_5r_CPf_SET(top_xg_pll_5, 3);
        TOP_XG_PLL_CTRL_5r_CZf_SET(top_xg_pll_5, 3);
        TOP_XG_PLL_CTRL_5r_RPf_SET(top_xg_pll_5, 7);
        TOP_XG_PLL_CTRL_5r_RZf_SET(top_xg_pll_5, 2);
        TOP_XG_PLL_CTRL_5r_ICPf_SET(top_xg_pll_5, 10);
        ioerr += WRITE_TOP_XG_PLL_CTRL_5r(unit, 1, top_xg_pll_5);

        READ_TOP_XG_PLL_CTRL_7r(unit, 1, &top_xg_pll_7);
        TOP_XG_PLL_CTRL_7r_CPPf_SET(top_xg_pll_7, 0x80);
        ioerr += WRITE_TOP_XG_PLL_CTRL_7r(unit, 1, top_xg_pll_7);

        READ_TOP_XG1_LCPLL_FBDIV_CTRLr(unit, 0, &top_xg1_lcpll_fbdiv);
        TOP_XG1_LCPLL_FBDIV_CTRLr_XG1_LCPLL_FBDIV_0f_SET(top_xg1_lcpll_fbdiv, 0);
        ioerr += WRITE_TOP_XG1_LCPLL_FBDIV_CTRLr(unit, 0, top_xg1_lcpll_fbdiv);

        READ_TOP_XG1_LCPLL_FBDIV_CTRLr(unit, 1, &top_xg1_lcpll_fbdiv);
        TOP_XG1_LCPLL_FBDIV_CTRLr_XG1_LCPLL_FBDIV_0f_SET(top_xg1_lcpll_fbdiv, 0x0fa0);
        ioerr += WRITE_TOP_XG1_LCPLL_FBDIV_CTRLr(unit, 1, top_xg1_lcpll_fbdiv);
    } else {
        READ_TOP_XG_PLL_CTRL_1r(unit, 1, &top_xg_pll_1);
        TOP_XG_PLL_CTRL_1r_PDIVf_SET(top_xg_pll_1, 3);
        ioerr += WRITE_TOP_XG_PLL_CTRL_1r(unit, 1, top_xg_pll_1);

        READ_TOP_XG_PLL_CTRL_0r(unit, 1, &top_xg_pll_0);
        TOP_XG_PLL_CTRL_0r_CH0_MDIVf_SET(top_xg_pll_0, 0x14);
        ioerr += WRITE_TOP_XG_PLL_CTRL_0r(unit, 1, top_xg_pll_0);

        READ_TOP_XG_PLL_CTRL_7r(unit, 1, &top_xg_pll_7);
        TOP_XG_PLL_CTRL_7r_FREQ_DOUBLER_ONf_SET(top_xg_pll_7, 0);
        ioerr += WRITE_TOP_XG_PLL_CTRL_7r(unit, 1, top_xg_pll_7);

        READ_TOP_XG_PLL_CTRL_6r(unit, 1, &top_xg_pll_6);
        TOP_XG_PLL_CTRL_6r_MSC_CTRLf_SET(top_xg_pll_6, 0xcaa4);
        ioerr += WRITE_TOP_XG_PLL_CTRL_6r(unit, 1, top_xg_pll_6);

        READ_TOP_XG_PLL_CTRL_7r(unit, 1, &top_xg_pll_7);
        TOP_XG_PLL_CTRL_7r_VCO_CONT_ADJf_SET(top_xg_pll_7, 1);
        ioerr += WRITE_TOP_XG_PLL_CTRL_7r(unit, 1, top_xg_pll_7);

        READ_TOP_XG_PLL_CTRL_3r(unit, 1, &top_xg_pll_3);
        TOP_XG_PLL_CTRL_3r_VCO_CURf_SET(top_xg_pll_3, 0);
        ioerr += WRITE_TOP_XG_PLL_CTRL_3r(unit, 1, top_xg_pll_3);

        READ_TOP_XG_PLL_CTRL_5r(unit, 1, &top_xg_pll_5);
        TOP_XG_PLL_CTRL_5r_VCO_GAINf_SET(top_xg_pll_5, 15);
        ioerr += WRITE_TOP_XG_PLL_CTRL_5r(unit, 1, top_xg_pll_5);

        READ_TOP_MISC_CONTROL_1r(unit, &top_misc_ctrl_1);
        TOP_MISC_CONTROL_1r_CMIC_TO_XG_PLL1_SW_OVWRf_SET(top_misc_ctrl_1, 1);
        ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_ctrl_1);

        READ_TOP_XG_PLL_CTRL_5r(unit, 1, &top_xg_pll_5);
        TOP_XG_PLL_CTRL_5r_CP1f_SET(top_xg_pll_5, 1);
        TOP_XG_PLL_CTRL_5r_CPf_SET(top_xg_pll_5, 1);
        TOP_XG_PLL_CTRL_5r_CZf_SET(top_xg_pll_5, 3);
        TOP_XG_PLL_CTRL_5r_RPf_SET(top_xg_pll_5, 0);
        TOP_XG_PLL_CTRL_5r_RZf_SET(top_xg_pll_5, 2);
        TOP_XG_PLL_CTRL_5r_ICPf_SET(top_xg_pll_5, 20);
        ioerr += WRITE_TOP_XG_PLL_CTRL_5r(unit, 1, top_xg_pll_5);

        READ_TOP_XG_PLL_CTRL_7r(unit, 1, &top_xg_pll_7);
        TOP_XG_PLL_CTRL_7r_CPPf_SET(top_xg_pll_7, 0x80);
        ioerr += WRITE_TOP_XG_PLL_CTRL_7r(unit, 1, top_xg_pll_7);

        READ_TOP_XG_PLL_CTRL_6r(unit, 1, &top_xg_pll_6);
        TOP_XG_PLL_CTRL_6r_LDO_CTRLf_SET(top_xg_pll_6, 0x22);
        ioerr += WRITE_TOP_XG_PLL_CTRL_6r(unit, 1, top_xg_pll_6);

        READ_TOP_XG1_LCPLL_FBDIV_CTRLr(unit, 0, &top_xg1_lcpll_fbdiv);
        TOP_XG1_LCPLL_FBDIV_CTRLr_XG1_LCPLL_FBDIV_0f_SET(top_xg1_lcpll_fbdiv, 0);
        ioerr += WRITE_TOP_XG1_LCPLL_FBDIV_CTRLr(unit, 0, top_xg1_lcpll_fbdiv);

        READ_TOP_XG1_LCPLL_FBDIV_CTRLr(unit, 1, &top_xg1_lcpll_fbdiv);
        TOP_XG1_LCPLL_FBDIV_CTRLr_XG1_LCPLL_FBDIV_0f_SET(top_xg1_lcpll_fbdiv, 0x0f00);
        ioerr += WRITE_TOP_XG1_LCPLL_FBDIV_CTRLr(unit, 1, top_xg1_lcpll_fbdiv);
    }

    READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_LCPLL_SOFT_RESETf_SET(top_sreset, 0);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);

    /* Select LCPLL0 as external PHY reference clock and output 125MHz */
    /* Pull GPIO3 low to reset the ext. PHY */
    ioerr += _gpio_reset(unit, 3, TRUE, 0);

    READ_TOP_XG_PLL_CTRL_0r(unit, 0, &top_xg_pll_0);
    TOP_XG_PLL_CTRL_0r_CH5_MDIVf_SET(top_xg_pll_0, 0x19);
    ioerr += WRITE_TOP_XG_PLL_CTRL_0r(unit, 0, top_xg_pll_0);

    READ_TOP_XG_PLL_CTRL_6r(unit, 0, &top_xg_pll_6);
    TOP_XG_PLL_CTRL_6r_MSC_CTRLf_SET(top_xg_pll_6, 0x71a2);
    ioerr += WRITE_TOP_XG_PLL_CTRL_6r(unit, 0, top_xg_pll_6);

    /* Pull  GPIO high to leave the reset state */
    ioerr += _gpio_reset(unit, 3, TRUE, 1);

    return ioerr;
}

static void
_port_gphy_mode_init(unit)
{
    TOP_DEV_REV_IDr_t rev_id;
    TOP_MISC_STATUSr_t top_misc;

    /* The support of gphy bypass mode is started from B0 */
    READ_TOP_DEV_REV_IDr(unit, &rev_id);
    if (TOP_DEV_REV_IDr_REV_IDf_GET(rev_id) < 0x10) {
        return;
    }

    READ_TOP_MISC_STATUSr(unit, &top_misc);
    if (CDK_CHIP_CONFIG(unit) & DCFG_GPHY_BYPASS) {
        TOP_MISC_STATUSr_RSVD_0f_SET(top_misc, 0x3);
    } else {
        TOP_MISC_STATUSr_RSVD_0f_SET(top_misc, 0);
    }
    WRITE_TOP_MISC_STATUSr(unit, top_misc);

    return;
}
int
bcm56160_a0_xlport_reset(int unit, int port)
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

    /* Reset XGXS */
    XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_xgxs0_ctrl, 0);
    ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_xgxs0_ctrl, port);
    BMD_SYS_USLEEP(11100);

    /* Bring XGXS out of reset */
    XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_xgxs0_ctrl, 1);
    ioerr += WRITE_XLPORT_XGXS0_CTRL_REGr(unit, xlport_xgxs0_ctrl, port);
    BMD_SYS_USLEEP(1100);

    return ioerr;
}

int
bcm56160_a0_gport_reset(int unit, int blk)
{
    int ioerr = 0;
    GPORT_XGXS0_CTRL_REGr_t gport_xgxs0_ctrl;

    /* Reference clock selection */
    ioerr += READ_GPORT_XGXS0_CTRL_REGr(unit, blk, &gport_xgxs0_ctrl);
    GPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(gport_xgxs0_ctrl, 1);
    ioerr += WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk, gport_xgxs0_ctrl);

    /* Deassert power down */
    GPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(gport_xgxs0_ctrl, 0);
    ioerr += WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk, gport_xgxs0_ctrl);
    BMD_SYS_USLEEP(1100);

    /* Reset XGXS */
    GPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(gport_xgxs0_ctrl, 0);
    ioerr += WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk, gport_xgxs0_ctrl);
    BMD_SYS_USLEEP(11100);

    /* Bring XGXS out of reset */
    GPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(gport_xgxs0_ctrl, 1);
    ioerr += WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk, gport_xgxs0_ctrl);
    BMD_SYS_USLEEP(1100);

    /* Bring reference clock out of reset */
    GPORT_XGXS0_CTRL_REGr_RSTB_REFCLKf_SET(gport_xgxs0_ctrl, 1);
    ioerr += WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk, gport_xgxs0_ctrl);

    /* Activate clocks */
    GPORT_XGXS0_CTRL_REGr_RSTB_PLLf_SET(gport_xgxs0_ctrl, 1);
    ioerr += WRITE_GPORT_XGXS0_CTRL_REGr(unit, blk, gport_xgxs0_ctrl);

    return ioerr;
}

int
bcm56160_a0_bmd_reset(int unit)
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
    PGW_CTRL_0r_t pgw_ctrl_0;
    XLPORT_MAC_CONTROLr_t xlport_mac_ctrl;
    int strap_val;
    int disable_tsc, disable_qtc;
    uint32_t ring_map[] = { 0x00110000, 0x00430000, 0x00005064,
                            0x00000000, 0x77772222, 0x00000000 };

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

    CMIC_SBUS_TIMEOUTr_SET(cmic_sbus_timeout, 0x7d0);
    ioerr += WRITE_CMIC_SBUS_TIMEOUTr(unit, cmic_sbus_timeout);

    BMD_SYS_USLEEP(wait_usec);

    ioerr += _pll_config(unit);

    /* Base on the strap pin to disable the TSC or QTC blocks */
    ioerr += READ_TOP_STRAP_STATUSr(unit, &strap_status);
    strap_val = TOP_STRAP_STATUSr_STRAP_STATUSf_GET(strap_status);
    disable_tsc = ((strap_val >> 25) & 0x3);
    disable_qtc = ((strap_val >> 27) & 0x3);
    if (disable_tsc || disable_qtc) {
        bcm56160_a0_block_disable(unit, disable_tsc, disable_qtc);
    }

    /* Unused TSCx or QTCx should be disabled */
    disable_tsc = 0x3;
    disable_qtc = 0x3;
    bcm56160_a0_gport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        disable_qtc &= ~(1 << ((port - 2) >> 4));
    }
    bcm56160_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        disable_tsc &= ~(1 << ((port - 34) >> 2));
    }

    /* Configure GPHY bypass mode */
    _port_gphy_mode_init(unit);

    READ_PGW_CTRL_0r(unit, &pgw_ctrl_0);
    PGW_CTRL_0r_SW_PM4X10_DISABLEf_SET(pgw_ctrl_0, disable_tsc);
    PGW_CTRL_0r_SW_QTC_DISABLEf_SET(pgw_ctrl_0, disable_qtc);
    ioerr += WRITE_PGW_CTRL_0r(unit, pgw_ctrl_0);

    /* Bring port blocks out of reset */
    READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_SET(top_sreset, 0xf);
    TOP_SOFT_RESET_REGr_TOP_XLP0_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP1_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_GXP0_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_GXP1_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec);

    /* Bring network sync out of reset */
    READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec);

    READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);

    /* Bring IP, EP, and MMU blocks out of reset */
    READ_TOP_SOFT_RESET_REGr(unit, &top_sreset);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec);

    /* Configure GPHY bypass mode */
    _port_gphy_mode_init(unit);

    /* Reset TSCE and QTC ports */
    for (idx = 0; idx < QTC_MAX_BLK_COUNT; idx++) {
        bcm56160_a0_gport_reset(unit, idx);
    }

    bcm56160_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (XPORT_SUBPORT(unit, BLKTYPE_XLPORT, port) == 0) {
            ioerr += bcm56160_a0_xlport_reset(unit, port);

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

#endif /* CDK_CONFIG_INCLUDE_BCM56160_A0 */
