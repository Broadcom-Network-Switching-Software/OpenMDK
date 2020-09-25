#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56340_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56340_a0_defs.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/arch/xgsm_miim.h>

#include "bcm56340_a0_bmd.h"
#include "bcm56340_a0_internal.h"

#define RESET_SLEEP_USEC                100
#define PLL_LOCK_MSEC                   10
#define TXPLL_LOCK_MSEC                 100

/* Transform datasheet mapped address to MIIM address used by software API */
#define MREG(_b) ((((_b) & 0xfff0) << 8) | 0x10 | ((_b) & 0xf))

#define MIIM_CL22_WRITE(_u, _addr, _reg, _val) \
    cdk_xgsm_miim_write(_u, _addr, _reg, _val)
#define MIIM_CL22_READ(_u, _addr, _reg, _val) \
    cdk_xgsm_miim_read(_u, _addr, _reg, _val)

#define MIIM_CL45_WRITE(_u, _addr, _dev, _reg, _val) \
    cdk_xgsm_miim_write(_u, _addr, LSHIFT32(_dev, 16) | _reg, _val)
#define MIIM_CL45_READ(_u, _addr, _dev, _reg, _val) \
    cdk_xgsm_miim_read(_u, _addr, LSHIFT32(_dev, 16) | _reg, _val)

/* Clause 45 devices */
#define C45_PMA         1
#define C45_AN          7

#if BMD_CONFIG_INCLUDE_PHY == 1
static uint32_t
_phy_addr_get(int port)
{
    if ((port == 49) || (port == 50)) {
        return 0x3 + CDK_XGSM_MIIM_IBUS(3);
    } 
    if (port > 52) {
        return (port - 52) + CDK_XGSM_MIIM_IBUS(2);
    } 
    if (port > 24) {
        return (port - 24) + CDK_XGSM_MIIM_IBUS(1);
    }
    return port + CDK_XGSM_MIIM_IBUS(0);    
}
#endif

int
bcm56340_a0_warpcore_phy_init(int unit, int port)
{
    int ioerr = 0;
#if BMD_CONFIG_INCLUDE_PHY == 1
    uint32_t phy_addr = _phy_addr_get(port);
    uint32_t mreg_val;
    uint32_t speed, lanes;
    int mode_10g;

    if (XPORT_SUBPORT(port) != 0) {
        return 0;
    }

    speed = bcm56340_a0_port_speed_max(unit, port);
    lanes = bcm56340_a0_port_num_lanes(unit, port);
    
    /* Enable multi MMD mode to allow clause 45 access */
    ioerr += MIIM_CL22_WRITE(unit, phy_addr, 0x1f, 0x8000);
    ioerr += MIIM_CL22_READ(unit, phy_addr, 0x1d, &mreg_val);
    mreg_val |= 0x400f;
    mreg_val &= ~0x8000;
    ioerr += MIIM_CL22_WRITE(unit, phy_addr, 0x1d, mreg_val);

    if (lanes == 4) {
        /* Enable Combocore for 4 lane ports */
        ioerr += MIIM_CL45_READ(unit, phy_addr, C45_PMA, 0x8000, &mreg_val);
        mreg_val &= ~(0xf << 8);
        mode_10g = 0xc;
        mreg_val |= (mode_10g << 8);
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x8000, mreg_val);
    } else /*if (lanes == 1)*/ { 
        /* Enable independent lane mode if max speed is 20G or below */
        ioerr += MIIM_CL45_READ(unit, phy_addr, C45_PMA, 0x8000, &mreg_val);
        mreg_val &= ~(0xf << 8);
        mode_10g = 4;
        mreg_val |= (mode_10g << 8);
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x8000, mreg_val);
    } 

    /* Enable broadcast */
    ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0xffde, 0x01ff);
    if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_GE) {
        /* Advertise 2.5G */
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x8329, 0x0001);
    } else if (speed <= 10000) {
        /* Do not advertise 1G */
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0xffe4, 0x0020);
        /* Do not advertise 10G CX4 */
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x8329, 0x0000);
        /* Advertise clause 72 capability */
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x832b, 0x0404);
        /* Advertise 10G KR and 1G KX */
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_AN, 0x0011, 0x00a0);
    } else if (speed <= 20000) {
        /* Do not advertise 1G */
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0xffe4, 0x0020);
        /* Do not advertise 10G CX4 */
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x8329, 0x0000);
        /* Advertise clause 72 capability */
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x832b, 0x0004);
        /* Do not advertise DXGXS speeds */
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x835d, 0x0400);
        /* Do not advertise >20G */
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_AN, 0x0011, 0x0000);
    } else {
        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
            /* Do not advertise 1G */
            ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0xffe4, 0x0020);
            /* Advertise 10G (HiG/CX4), 12G, 13G, 15G, 16G and 20G */
            ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x8329, 0x07b8);
            /* Advertise clause 72, 21G, 25G, 31.5G and 40G */
            ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x832b, 0x03a4);
            /* Advertise 20G */
            ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x835d, 0x0401);
            /* Do not advertise 40G KR4/CR4 */
            ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_AN, 0x0011, 0x0000);
        } else {
            /* Do not advertise 1G */
            ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0xffe4, 0x0020);
            /* Advertise 10G CX4 */
            ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x8329, 0x0010);
            /* Advertise clause 72 capability */
            ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x832b, 0x0404);
            /* Advertise 40G KR4 and 10G KX4 */
            ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_AN, 0x0011, 0x0140);
        }
    }
    /* Disable 10G parallel detect */
    ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x8131, 0x0000);

    /* Disable broadcast */
    ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0xffde, 0x0000);

    /* Enable PLL state machine */
    ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x8051, 0xd006);
#endif

    return ioerr;
}

int
bcm56340_a0_xport_inst(int unit, int port)
{
    static uint8_t xgxs1_ports[] = { 5, 17, 29, 41 };
    int idx;

    port = ((port - 1) & ~0x3) + 1;

    for (idx = 0; idx < COUNTOF(xgxs1_ports); idx++) {
        if (port == xgxs1_ports[idx]) {
            return 1;
        }
        if (port == (xgxs1_ports[idx] + 4)) {
            return 2;
        }
    }
    return 0;
}

int
bcm56340_a0_xport_reset(int unit, int port)
{
    int ioerr = 0;
    int inst, msec;
    PORT_XGXS_CTRL_REGr_t xgxs_ctrl;
    PORT_XGXS_STATUS_GEN_REGr_t xgxs_stat;

    /* Use instance instead of XGXS0, XGXS1 and XGXS2 */
    inst = bcm56340_a0_xport_inst(unit, port);

    /* Configure clock source */
    ioerr += READ_PORT_XGXS_CTRL_REGr(unit, inst, &xgxs_ctrl, port);
    PORT_XGXS_CTRL_REGr_LCREF_ENf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_PORT_XGXS_CTRL_REGr(unit, inst, xgxs_ctrl, port);
    
    if (port == 49) {
        /* REFSEL=2'b01, REFDIV=2'b00, REF_TERM_SEL=1'b1. */
        ioerr += READ_PORT_XGXS_CTRL_REGr(unit, inst, &xgxs_ctrl, port);
        PORT_XGXS_CTRL_REGr_REFSELf_SET(xgxs_ctrl, 1);
        PORT_XGXS_CTRL_REGr_REF_TERM_SELf_SET(xgxs_ctrl, 1);
        PORT_XGXS_CTRL_REGr_REFDIVf_SET(xgxs_ctrl, 0);
        ioerr += WRITE_PORT_XGXS_CTRL_REGr(unit, inst, xgxs_ctrl, port);
    }
    
    /* Force XMAC into reset before initialization */
    ioerr += READ_PORT_XGXS_CTRL_REGr(unit, inst, &xgxs_ctrl, port);
    PORT_XGXS_CTRL_REGr_IDDQf_SET(xgxs_ctrl, 1);
    PORT_XGXS_CTRL_REGr_PWRDWNf_SET(xgxs_ctrl, 1);
    PORT_XGXS_CTRL_REGr_PWRDWN_PLLf_SET(xgxs_ctrl, 1);
    PORT_XGXS_CTRL_REGr_RSTB_HWf_SET(xgxs_ctrl, 0);
    PORT_XGXS_CTRL_REGr_RSTB_PLLf_SET(xgxs_ctrl, 0);
    PORT_XGXS_CTRL_REGr_RSTB_MDIOREGSf_SET(xgxs_ctrl, 0);
    PORT_XGXS_CTRL_REGr_TXD1G_FIFO_RSTBf_SET(xgxs_ctrl, 0);
    PORT_XGXS_CTRL_REGr_TXD10G_FIFO_RSTBf_SET(xgxs_ctrl, 0);
    ioerr += WRITE_PORT_XGXS_CTRL_REGr(unit, inst, xgxs_ctrl, port);

    /*
     * XGXS MAC initialization steps.
     *
     * A minimum delay is required between various initialization steps.
     * There is no maximum delay.  The values given are very conservative
     * including the timeout for TX PLL lock.
     */

    /* Powerup Unicore interface (digital and analog clocks) */
    ioerr += READ_PORT_XGXS_CTRL_REGr(unit, inst, &xgxs_ctrl, port);
    PORT_XGXS_CTRL_REGr_IDDQf_SET(xgxs_ctrl, 0);
    PORT_XGXS_CTRL_REGr_PWRDWNf_SET(xgxs_ctrl, 0);
    PORT_XGXS_CTRL_REGr_PWRDWN_PLLf_SET(xgxs_ctrl, 0);
    ioerr += WRITE_PORT_XGXS_CTRL_REGr(unit, inst, xgxs_ctrl, port);
    BMD_SYS_USLEEP(RESET_SLEEP_USEC);

    /* Bring Warpcore out of reset */
    PORT_XGXS_CTRL_REGr_RSTB_HWf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_PORT_XGXS_CTRL_REGr(unit, inst, xgxs_ctrl, port);
    BMD_SYS_USLEEP(RESET_SLEEP_USEC);

    /* Bring MDIO registers out of reset */
    PORT_XGXS_CTRL_REGr_RSTB_MDIOREGSf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_PORT_XGXS_CTRL_REGr(unit, inst, xgxs_ctrl, port);

    /* Activate all clocks */
    PORT_XGXS_CTRL_REGr_RSTB_PLLf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_PORT_XGXS_CTRL_REGr(unit, inst, xgxs_ctrl, port);

    /* Check if unused XPORT */
    if (BMD_PORT_PROPERTIES(unit, port) == 0) {
        return ioerr;
    }

    /* Wait for TX PLL lock */
    for (msec = 0; msec < TXPLL_LOCK_MSEC; msec++) {
#if BMD_CONFIG_SIMULATION
        if (msec == 0) break;
#endif
        ioerr += READ_PORT_XGXS_STATUS_GEN_REGr(unit, inst, &xgxs_stat, port);
        if (PORT_XGXS_STATUS_GEN_REGr_TXPLL_LOCKf_GET(xgxs_stat) != 0) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (msec >= TXPLL_LOCK_MSEC) {
        CDK_WARN(("bcm56340_a0_xport_reset[%d]: "
                  "TX PLL did not lock on port %d\n", unit, port));
    }

    /* Enable Tx FIFO */
    PORT_XGXS_CTRL_REGr_TXD1G_FIFO_RSTBf_SET(xgxs_ctrl, 0xf);
    PORT_XGXS_CTRL_REGr_TXD10G_FIFO_RSTBf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_PORT_XGXS_CTRL_REGr(unit, inst, xgxs_ctrl, port);

    return ioerr;
}

static int
_lcpll_init(int unit)
{
    int ioerr = 0;
    int msec, idx, locks;
    TOP_XG_PLL_STATUSr_t pll_status[2];

    /* Wait for LC PLL locks */
    for (msec = 0; msec < PLL_LOCK_MSEC; msec++) {
#if BMD_CONFIG_SIMULATION
        if (msec == 0) break;
#endif
        locks = 0;
        for (idx = 0; idx < COUNTOF(pll_status); idx++) {
            ioerr += READ_TOP_XG_PLL_STATUSr(unit, idx, &pll_status[idx]);
            locks += TOP_XG_PLL_STATUSr_TOP_XGPLL_LOCKf_GET(pll_status[idx]);
        }
        if (locks == COUNTOF(pll_status)) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (msec >= PLL_LOCK_MSEC) {
        CDK_WARN(("bcm56340_a0_bmd_reset[%d]: LC PLL did not lock, status = ",
                  unit));
        for (idx = 0; idx < COUNTOF(pll_status); idx++) {
            CDK_WARN(("0x%08"PRIx32" ",
                      TOP_XG_PLL_STATUSr_GET(pll_status[idx])));
        }
        CDK_WARN(("\n"));
    }

    return ioerr;
}

static int
_tsbspll_init(int unit)
{
    int ioerr = 0;
    int msec;
    TOP_TS_PLL_STATUSr_t ts_pll_status;
    TOP_BS_PLL0_STATUSr_t bs_pll0_status;

    /* Wait for LC PLL locks */
    for (msec = 0; msec < PLL_LOCK_MSEC; msec++) {
#if BMD_CONFIG_SIMULATION
        if (msec == 0) break;
#endif
        ioerr += READ_TOP_TS_PLL_STATUSr(unit, &ts_pll_status);
        ioerr += READ_TOP_BS_PLL0_STATUSr(unit, &bs_pll0_status);
        if (TOP_TS_PLL_STATUSr_PLL_LOCKf_GET(ts_pll_status) == 1 &&
            TOP_BS_PLL0_STATUSr_PLL_LOCKf_GET(bs_pll0_status) == 1) {
            break;
        }
        BMD_SYS_USLEEP(1000);
    }
    if (msec >= PLL_LOCK_MSEC) {
        CDK_WARN(("bcm56340_a0_bmd_reset[%d]: "
                  "TS/BS PLL did not lock, status = "
                  "0x%08"PRIx32" 0x%08"PRIx32"\n",
                  unit,
                  TOP_TS_PLL_STATUSr_GET(ts_pll_status), 
                  TOP_BS_PLL0_STATUSr_GET(bs_pll0_status)));
    }
    return ioerr;
}

static int
_xmac_cclk_cfg(int unit)
{
    int ioerr = 0;
    TOP_CORE_PLL_CTRL0r_t pll_ctrl0;
    TOP_CORE_PLL_CTRL2r_t pll_ctrl2;
    TOP_CORE_PLL_CTRL3r_t pll_ctrl3;
    TOP_CORE_PLL_CTRL4r_t pll_ctrl4;
    TOP_MISC_CONTROL_1r_t misc_ctrl1;

    /* TOP_CORE_PLL_CTRL0 -> FAST_LOCK */
    ioerr += READ_TOP_CORE_PLL_CTRL0r(unit, &pll_ctrl0);
    TOP_CORE_PLL_CTRL0r_FAST_LOCKf_SET(pll_ctrl0, 1);
    ioerr += WRITE_TOP_CORE_PLL_CTRL0r(unit, pll_ctrl0);

    /* TOP_CORE_PLL_CTRL3 -> MSTR_NDIV_INTf */
    ioerr += READ_TOP_CORE_PLL_CTRL3r(unit, &pll_ctrl3);
    TOP_CORE_PLL_CTRL3r_MSTR_NDIV_INTf_SET(pll_ctrl3, 111);
    ioerr += WRITE_TOP_CORE_PLL_CTRL3r(unit, pll_ctrl3);

    /* TOP_CORE_PLL_CTRL4 -> MSTR_CH0_MDIVf */
    ioerr += READ_TOP_CORE_PLL_CTRL4r(unit, &pll_ctrl4);
    TOP_CORE_PLL_CTRL4r_MSTR_CH0_MDIVf_SET(pll_ctrl4, 11);
    ioerr += WRITE_TOP_CORE_PLL_CTRL4r(unit, pll_ctrl4);

    /* TOP_MISC_CONTROL -> CMIC_TO_CORE_PLL_LOAD */
    ioerr += READ_TOP_MISC_CONTROL_1r(unit, &misc_ctrl1);
    TOP_MISC_CONTROL_1r_CMIC_TO_CORE_PLL_LOADf_SET(misc_ctrl1, 1);
    ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, misc_ctrl1);

    /* TOP_CORE_PLL_CTRL2 -> LOAD_EN_CH0 */
    ioerr += READ_TOP_CORE_PLL_CTRL2r(unit, &pll_ctrl2);
    TOP_CORE_PLL_CTRL2r_LOAD_EN_CH0f_SET(pll_ctrl2, 1);
    ioerr += WRITE_TOP_CORE_PLL_CTRL2r(unit, pll_ctrl2);

    return ioerr;
}


int
bcm56340_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int wait_usec = 100000;
    int port, idx, inst;
    cdk_pbmp_t pbmp, pbmp_all, pbmp_xport;
    CMIC_CPS_RESETr_t cps_reset;
    TOP_SOFT_RESET_REGr_t top_sreset;
    TOP_SOFT_RESET_REG_2r_t top_sreset_2;
    CMIC_SBUS_RING_MAPr_t sbus_ring_map;
    CMIC_SBUS_TIMEOUTr_t sbus_to;
    TOP_XG_PLL_CTRL_0r_t pll_ctrl_0;
    TOP_MISC_CONTROL_1r_t misc_ctrl;
    PORT_MAC_CONTROLr_t mac_ctrl;
    uint32_t ring_map[] = { 0x07752100, 0x66666666, 0x00000643 };

    BMD_CHECK_UNIT(unit);

    /* Initialize endian mode for correct reset access */
    ioerr += cdk_xgsm_cmic_init(unit);

    /* Pull reset line */
    ioerr += READ_CMIC_CPS_RESETr(unit, &cps_reset);
    CMIC_CPS_RESETr_CPS_RESETf_SET(cps_reset, 1);
    ioerr += WRITE_CMIC_CPS_RESETr(unit, cps_reset);

    /* Wait for all tables to initialize */
    BMD_SYS_USLEEP(wait_usec);

    /* Re-initialize endian mode after reset */
    ioerr += cdk_xgsm_cmic_init(unit);

    /*
     * BCM56340 ring map
     *
     * ring 0: IP(1)
     * ring 1: EP(2)
     * ring 2: MMU(3)
     * ring 3: ISM(16)
     * ring 4: SER(17)
     * ring 5: AXP(4)
     * ring 6: XTP0(8)..XTP3(11), XLP0(12), IBOD(18), 
     *         XWP0(13)..XWP2(15)
     * ring 7: OTPC(5), TOP_REGS(6)
     */
    for (idx = 0; idx < COUNTOF(ring_map); idx++) {
        CMIC_SBUS_RING_MAPr_SET(sbus_ring_map, ring_map[idx]);
        ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, idx, sbus_ring_map);
    }

    /* Reset all blocks */
    TOP_SOFT_RESET_REGr_CLR(top_sreset);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);

    /* Reconfig XMAC cclk for XL port XMAC FIFO UR */
    if ((CDK_XGSM_FLAGS(unit) & CHIP_FLAG_BW150G) && 
        !(CDK_XGSM_FLAGS(unit) & CHIP_FLAG_NH1K)) {
        ioerr += _xmac_cclk_cfg(unit);
    }
        
    /* Set the OOBFC clock speed to 125MHz */
    for (idx = 0; idx < 4; idx++) {
        ioerr += READ_TOP_XG_PLL_CTRL_0r(unit, idx, &pll_ctrl_0);
        TOP_XG_PLL_CTRL_0r_CH3_MDIVf_SET(pll_ctrl_0, 25);
        ioerr += WRITE_TOP_XG_PLL_CTRL_0r(unit, idx, pll_ctrl_0);
    }

    /* Enable software override of PLL parameters */
    ioerr += READ_TOP_MISC_CONTROL_1r(unit, &misc_ctrl);
    TOP_MISC_CONTROL_1r_CMIC_TO_XG_PLL0_SW_OVWRf_SET(misc_ctrl, 1);
    TOP_MISC_CONTROL_1r_CMIC_TO_XG_PLL1_SW_OVWRf_SET(misc_ctrl, 1);
    TOP_MISC_CONTROL_1r_CMIC_TO_XG_PLL2_SW_OVWRf_SET(misc_ctrl, 1);
    TOP_MISC_CONTROL_1r_CMIC_TO_XG_PLL3_SW_OVWRf_SET(misc_ctrl, 1);
    ioerr += WRITE_TOP_MISC_CONTROL_1r(unit, misc_ctrl);

    /* Bring LCPLL out of reset */
    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &top_sreset_2);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL0_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL1_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    BMD_SYS_USLEEP(50);

    /* Initialize LC PLLs */
    ioerr += _lcpll_init(unit);

    /* De-assert LCPLL's post reset */
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL0_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_XG_PLL1_POST_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    BMD_SYS_USLEEP(50);

	/* put broadsync pll in reset */
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_RST_Lf_SET(top_sreset_2, 0);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_POST_RST_Lf_SET(top_sreset_2, 0);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    BMD_SYS_USLEEP(50);

    /* Timesync and broadsync PLLs out of reset */
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    BMD_SYS_USLEEP(50);

    /* Initialize timesync/broadsync PLLs */
    ioerr += _tsbspll_init(unit);

    /* De-assert timesync/broadsync PLL post reset */
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_sreset_2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_BS_PLL0_POST_RST_Lf_SET(top_sreset_2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, top_sreset_2);
    BMD_SYS_USLEEP(50);

    /* Bring port blocks out of reset */
    TOP_SOFT_RESET_REGr_TOP_XTP0_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XTP1_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XTP2_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XTP3_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP0_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XWP0_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XWP1_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_XWP2_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(50);

    /* Bring network sync out of reset */
    TOP_SOFT_RESET_REGr_TOP_NS_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(50);

    CMIC_SBUS_TIMEOUTr_SET(sbus_to, 0x7d0);
    ioerr += WRITE_CMIC_SBUS_TIMEOUTr(unit, sbus_to);

    /* Bring IP, EP, ISM, ETU, AXP, and MMU blocks out of reset */
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_ISM_RST_Lf_SET(top_sreset, 1);
    TOP_SOFT_RESET_REGr_TOP_AXP_RST_Lf_SET(top_sreset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, top_sreset);
    BMD_SYS_USLEEP(wait_usec);

    /* Reset all XPORTs */
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp_all);
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_XTPORT, &pbmp);
    CDK_PBMP_OR(pbmp_all, pbmp);
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_XWPORT, &pbmp);
    CDK_PBMP_OR(pbmp_all, pbmp);
    CDK_PBMP_ASSIGN(pbmp_xport, pbmp_all);

    CDK_PBMP_ITER(pbmp_all, port) {
        /* Ensure that the first XPORT/WC in each block is reset */
        if (XPORT_SUBPORT(port) == 0) {
            inst = bcm56340_a0_xport_inst(unit, port);
            if (inst > 0) {
                CDK_PBMP_PORT_ADD(pbmp_xport, port - (4 * inst));
            }
        }
    }
    CDK_PBMP_ITER(pbmp_xport, port) {
        /* We only need to reset first port in each block */
        if (XPORT_SUBPORT(port) == 0) {
            ioerr += bcm56340_a0_xport_reset(unit, port);
        }
    }

    CDK_PBMP_ITER(pbmp_all, port) {
        /* Initialize PHY for all ports */
        if (port > 52) {
            ioerr += bcm56340_a0_warpcore_phy_init(unit, port);
        } 
    }

    /* Bring XMACs out of reset */
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_XTPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (XPORT_SUBPORT(port) == 0 && bcm56340_a0_xport_inst(unit, port) == 0) {
			ioerr += READ_PORT_MAC_CONTROLr(unit, &mac_ctrl, port);
			PORT_MAC_CONTROLr_XMAC0_RESETf_SET(mac_ctrl, 1);
			if (CDK_PBMP_MEMBER(pbmp, port+4)) {
				PORT_MAC_CONTROLr_XMAC1_RESETf_SET(mac_ctrl, 1);
			}
			if (CDK_PBMP_MEMBER(pbmp, port+8)) {
				PORT_MAC_CONTROLr_XMAC2_RESETf_SET(mac_ctrl, 1);
			}
			ioerr += WRITE_PORT_MAC_CONTROLr(unit, mac_ctrl, port);
			BMD_SYS_USLEEP(50);
			PORT_MAC_CONTROLr_XMAC0_RESETf_SET(mac_ctrl, 0);
			if (CDK_PBMP_MEMBER(pbmp, port+4)) {
				PORT_MAC_CONTROLr_XMAC1_RESETf_SET(mac_ctrl, 0);
			}
			if (CDK_PBMP_MEMBER(pbmp, port+8)) {
				PORT_MAC_CONTROLr_XMAC2_RESETf_SET(mac_ctrl, 0);
			}
			ioerr += WRITE_PORT_MAC_CONTROLr(unit, mac_ctrl, port);
		}
	}
	
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (XPORT_SUBPORT(port) == 0 && bcm56340_a0_xport_inst(unit, port) == 0) {
			ioerr += READ_PORT_MAC_CONTROLr(unit, &mac_ctrl, port);
			PORT_MAC_CONTROLr_XMAC0_RESETf_SET(mac_ctrl, 1);
			ioerr += WRITE_PORT_MAC_CONTROLr(unit, mac_ctrl, port);
			BMD_SYS_USLEEP(50);
			PORT_MAC_CONTROLr_XMAC0_RESETf_SET(mac_ctrl, 0);
			ioerr += WRITE_PORT_MAC_CONTROLr(unit, mac_ctrl, port);
		}
	}

    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_XWPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (XPORT_SUBPORT(port) == 0 && bcm56340_a0_xport_inst(unit, port) == 0) {
			ioerr += READ_PORT_MAC_CONTROLr(unit, &mac_ctrl, port);
			PORT_MAC_CONTROLr_XMAC0_RESETf_SET(mac_ctrl, 1);
			ioerr += WRITE_PORT_MAC_CONTROLr(unit, mac_ctrl, port);
			BMD_SYS_USLEEP(50);
			PORT_MAC_CONTROLr_XMAC0_RESETf_SET(mac_ctrl, 0);
			ioerr += WRITE_PORT_MAC_CONTROLr(unit, mac_ctrl, port);
		}
	}

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56340_A0 */
