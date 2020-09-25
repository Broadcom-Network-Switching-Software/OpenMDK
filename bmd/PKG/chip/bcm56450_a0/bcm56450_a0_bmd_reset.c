#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56450_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <cdk/chip/bcm56450_a0_defs.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/arch/xgsm_miim.h>

#include "bcm56450_a0_bmd.h"
#include "bcm56450_a0_internal.h"

#define RESET_SLEEP_USEC                1000
#define PLL_LOCK_MSEC                   10

#ifndef _INIT_SVK_CLK
#define _INIT_SVK_CLK(_u) (0)
#endif

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
#define C45_PMA             1
#define C45_AN              7

#if BMD_CONFIG_INCLUDE_PHY == 1
static uint32_t 
_phy_addr_get(int unit, int port)
{
    if (bcm56450_a0_phy_connection_mode(unit, 6) == PHY_CONN_WARPCORE) {
        if ((port == 25 || (port >= 35 && port <= 37)) || 
            (port == 27 || (port >= 32 && port <= 34))) {
            return 0x9 + CDK_XGSM_MIIM_IBUS(1);
        }
    }
    if (bcm56450_a0_phy_connection_mode(unit, 7) == PHY_CONN_WARPCORE) {
        if ((port == 26 || (port >= 38 && port <= 40)) || 
            (port >= 28 && port <= 31)) {
            return 0xd + CDK_XGSM_MIIM_IBUS(1);
        }
    } 
    if (port == 42) {
        return 0x3 + CDK_XGSM_MIIM_IBUS(2);
    }
    if (port >= 38) {
        port = (port - 38) + 0x6;
        return port + CDK_XGSM_MIIM_IBUS(1);
    } 
    if (port >= 35) {
        port = (port - 35) + 0x2;
        return port + CDK_XGSM_MIIM_IBUS(1);
    } 
    if (port >= 32) {
        port = 0x9;
        return port + CDK_XGSM_MIIM_IBUS(1);
    }        
    if (port >= 29) {
        port = 0xd;
        return port + CDK_XGSM_MIIM_IBUS(1);
    }
    if (port >= 25) {
        port = (port - 25) * 4 + 1;
        return port + CDK_XGSM_MIIM_IBUS(1);
    }
    return port + CDK_XGSM_MIIM_IBUS(0);
}
#endif

int
bcm56450_a0_unicore_phy_init(int unit, int port)
{
    int ioerr = 0;
#if BMD_CONFIG_INCLUDE_PHY == 1
    uint32_t phy_addr = _phy_addr_get(unit, port);
    uint32_t mreg_val;
    uint32_t speed;

    speed = bcm56450_a0_port_speed_max(unit, port);

    /* Isolate external power-down input pins */
    ioerr += cdk_xgsm_miim_iblk_read(unit, phy_addr, MREG(0x801a), &mreg_val);
    mreg_val |= (1 << 10);
    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x801a), mreg_val);

    /* Enable multi PRD mode and multi MMD mode */
    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x800d), 0xc00f);

    /* Disable PLL state machine */
    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x8051), 0x5006);

    /* PLL VCO step time */
    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x8052), 0x04ff);

    /* Turn off slowdn_xor */
    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x8055), 0x0000);

    /* CDR bandwidth */
    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x80f6), 0x8300);

    /* Select core mode */
    ioerr += cdk_xgsm_miim_iblk_read(unit, phy_addr, MREG(0x8000), &mreg_val);
    mreg_val &= ~(0xf << 8);
    mreg_val |= (speed < 10000) ? (0x6 << 8) : (0xc << 8);
    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x8000), mreg_val);

    /* Enable DTE mdio reg mapping */
    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x800e), 0x0001);

    if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
        /* Configure Tx to default value */
        ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x80a7), 0x0990);

        /* Advertise 10G, 12G and 13G HiG/CX4 by default */
        ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x8329), 0x00b8);
    } else {
        /* Configure Tx for CX4 compliance */
        ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x80a7), 0x5ff0);

        /* Advertise 2.5G and 10G by default */
        ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x8329), 0x0011);
    }

    /* Adjust 10G parallel detect link timer to 60ms */ 
    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x8133), 0x16e2); 

    /* Change 10G parallel detect lostlink timer */
    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x8134), 0x4c4a);

    /* Configure 10G parallel detect */
    ioerr += cdk_xgsm_miim_iblk_read(unit, phy_addr, MREG(0x8131), &mreg_val);
    mreg_val |= (1 << 0); /* Enable 10G parallel detect */
    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x8131), mreg_val);

    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x8015), 0xff);

    /* Enable PLL state machine */
    ioerr += cdk_xgsm_miim_iblk_write(unit, phy_addr, MREG(0x8051), 0xf01e);
#endif

    return ioerr;
}

int
bcm56450_a0_warpcore_phy_init(int unit, int port)
{
    int ioerr = 0;
#if BMD_CONFIG_INCLUDE_PHY == 1
    uint32_t phy_addr = _phy_addr_get(unit, port);
    uint32_t mreg_val;
    int speed, lanes;

    /* Get lane index */
    if (MXQPORT_SUBPORT(unit, port) != 0) {
        return 0;
    }

    speed = bcm56450_a0_port_speed_max(unit, port);
    lanes = bcm56450_a0_port_num_lanes(unit, port);

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
        mreg_val |= (0xc << 8);
        ioerr += MIIM_CL45_WRITE(unit, phy_addr, C45_PMA, 0x8000, mreg_val);
    } else /*if (lanes == 1)*/ { 
        /* Enable independent lane mode if max speed is 20G or below */
        ioerr += MIIM_CL45_READ(unit, phy_addr, C45_PMA, 0x8000, &mreg_val);
        mreg_val &= ~(0xf << 8);
        mreg_val |= (4 << 8);
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
bcm56450_a0_xport_reset(int unit, int port)
{
    int ioerr = 0;
    int idx = 0;
    uint32_t dev_in_pkg;
    TOP_XGXS_MDIO_CONFIGr_t xgxs_mdio_cfg;
    XPORT_XGXS_CTRLr_t xgxs_ctrl;
    
    dev_in_pkg = (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) ? 0x3 : 0x15;
    if (port >= 25 && port <= 28) {
        idx = port - 25;

        ioerr += READ_TOP_XGXS_MDIO_CONFIGr(unit, idx, &xgxs_mdio_cfg);
        TOP_XGXS_MDIO_CONFIGr_MD_DEVADf_SET(xgxs_mdio_cfg, 5);
        TOP_XGXS_MDIO_CONFIGr_IEEE_DEVICES_IN_PKGf_SET(xgxs_mdio_cfg, dev_in_pkg);
        ioerr += WRITE_TOP_XGXS_MDIO_CONFIGr(unit, idx, xgxs_mdio_cfg);
    } 

    /* XGXS MAC initialization steps. */
    /* Release reset (if asserted) to allow bigmac to initialize */
    ioerr += READ_XPORT_XGXS_CTRLr(unit, &xgxs_ctrl, port);
    XPORT_XGXS_CTRLr_IDDQf_SET(xgxs_ctrl, 0);
    XPORT_XGXS_CTRLr_PWRDWNf_SET(xgxs_ctrl, 0);
    XPORT_XGXS_CTRLr_RSTB_HWf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xgxs_ctrl, port);

    BMD_SYS_USLEEP(RESET_SLEEP_USEC);
    
    /* Power down and reset */
    ioerr += READ_XPORT_XGXS_CTRLr(unit, &xgxs_ctrl, port);
    XPORT_XGXS_CTRLr_IDDQf_SET(xgxs_ctrl, 1);
    XPORT_XGXS_CTRLr_PWRDWNf_SET(xgxs_ctrl, 1);
    XPORT_XGXS_CTRLr_RSTB_HWf_SET(xgxs_ctrl, 0);
    XPORT_XGXS_CTRLr_TXD1G_FIFO_RSTBf_SET(xgxs_ctrl, 0);
    XPORT_XGXS_CTRLr_TXD10G_FIFO_RSTBf_SET(xgxs_ctrl, 0);
    XPORT_XGXS_CTRLr_RSTB_MDIOREGSf_SET(xgxs_ctrl, 0);
    XPORT_XGXS_CTRLr_RSTB_PLLf_SET(xgxs_ctrl, 0);
    ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xgxs_ctrl, port);

    BMD_SYS_USLEEP(RESET_SLEEP_USEC);

    /* Bring up both digital and analog clocks */
    ioerr += READ_XPORT_XGXS_CTRLr(unit, &xgxs_ctrl, port);
    XPORT_XGXS_CTRLr_IDDQf_SET(xgxs_ctrl, 0);
    XPORT_XGXS_CTRLr_PWRDWNf_SET(xgxs_ctrl, 0);
    ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xgxs_ctrl, port);
    
    BMD_SYS_USLEEP(RESET_SLEEP_USEC);

    /* Bring XGXS out of reset */
    ioerr += READ_XPORT_XGXS_CTRLr(unit, &xgxs_ctrl, port);
    XPORT_XGXS_CTRLr_RSTB_HWf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xgxs_ctrl, port);
    BMD_SYS_USLEEP(RESET_SLEEP_USEC);

    /* Bring MDIO registers out of reset */
    ioerr += READ_XPORT_XGXS_CTRLr(unit, &xgxs_ctrl, port);
    XPORT_XGXS_CTRLr_RSTB_MDIOREGSf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xgxs_ctrl, port);

    /* Activate all clocks */
    ioerr += READ_XPORT_XGXS_CTRLr(unit, &xgxs_ctrl, port);
    XPORT_XGXS_CTRLr_RSTB_PLLf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xgxs_ctrl, port);

    ioerr += READ_XPORT_XGXS_CTRLr(unit, &xgxs_ctrl, port);
    XPORT_XGXS_CTRLr_TXD1G_FIFO_RSTBf_SET(xgxs_ctrl, 0xf);
    XPORT_XGXS_CTRLr_TXD10G_FIFO_RSTBf_SET(xgxs_ctrl, 1);
    ioerr += WRITE_XPORT_XGXS_CTRLr(unit, xgxs_ctrl, port);

    return ioerr;
}

int
bcm56450_a0_bmd_reset(int unit)
{
    int ioerr = 0;
    int wait_usec = 10000;
    int port;
    int blkidx;
    cdk_pbmp_t pbmp;
    CMIC_SBUS_TIMEOUTr_t sbus_to;
    CMIC_CPS_RESETr_t cmic_cps;
    CMIC_SBUS_RING_MAPr_t ring_map;
    TOP_MMU_PLL_INITr_t pll_init;
    TOP_MMU_PLL_STATUS0r_t mmu_pll_status0;
    TOP_MMU_PLL_STATUS1r_t mmu_pll_status1;
    TOP_SOFT_RESET_REG_2r_t soft_reset2;
    TOP_SOFT_RESET_REGr_t soft_reset;
    TOP_SW_BOND_OVRD_CTRL1r_t bond_ovrd_ctrl1;
    TOP_SW_BOND_OVRD_CTRL0r_t bond_ovrd_ctrl0;
    XPORT_XMAC_CONTROLr_t xmac_control;
    
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

    /* SBUS ring and block number:
     * Ring0: cmic -> ip-ep -> cmic
     * Ring1: cmic -> mxqport0 -> mxqport1-> mxqport2 -> mxqport3-> cmic
     * Ring2: cmic -> gport0 -> gport1 -> gport2 -> cmic
     * Ring3: cmic -> otpc -> top -> cmic
     * Ring4: cmic -> mmu0 -> mmu1 -> cmic
     * Ring5: cmic -> ci0 -> ci1 -> ci2 -> lls -> cmic
     * Ring6: cmic -> ces -> cmic
     * Ring7: unused */
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x66034000);
    ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, 0, ring_map);
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x11112222);
    ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, 1, ring_map);
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x55555553);
    ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, 2, ring_map);
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x00637777);
    ioerr += WRITE_CMIC_SBUS_RING_MAPr(unit, 3, ring_map);

    /* Bring MMUPLL out of reset */
    ioerr += READ_TOP_MMU_PLL_INITr(unit, &pll_init);
    TOP_MMU_PLL_INITr_PLL1_RESETBf_SET(pll_init, 1);
    TOP_MMU_PLL_INITr_PLL2_RESETBf_SET(pll_init, 1);
    TOP_MMU_PLL_INITr_PLL3_RESETBf_SET(pll_init, 1);
    ioerr += WRITE_TOP_MMU_PLL_INITr(unit, pll_init);

    BMD_SYS_USLEEP(wait_usec);
    
    /* Wait for MMUPLL lock */
    ioerr += READ_TOP_MMU_PLL_STATUS0r(unit, &mmu_pll_status0);
    if (!TOP_MMU_PLL_STATUS0r_MMU_PLL1_LOCKf_GET(mmu_pll_status0)) {
        CDK_WARN(("bcm56450_a0_bmd_reset[%d]: MMUPLL 1 not locked, "
                  "status = 0x%08"PRIx32"\n", unit,
                  TOP_MMU_PLL_STATUS0r_MMU_PLL1_LOCKf_GET(mmu_pll_status0)));
    }
    if (!TOP_MMU_PLL_STATUS0r_MMU_PLL2_LOCKf_GET(mmu_pll_status0)) {
        CDK_WARN(("bcm56450_a0_bmd_reset[%d]: MMUPLL 2 not locked, "
                  "status = 0x%08"PRIx32"\n", unit,
                  TOP_MMU_PLL_STATUS0r_MMU_PLL2_LOCKf_GET(mmu_pll_status0)));
    }
    ioerr += READ_TOP_MMU_PLL_STATUS1r(unit, &mmu_pll_status1);
    if (!TOP_MMU_PLL_STATUS1r_MMU_PLL3_LOCKf_GET(mmu_pll_status1)) {
        CDK_WARN(("bcm56450_a0_bmd_reset[%d]: MMUPLL 3 not locked, "
                  "status = 0x%08"PRIx32"\n", unit,
                  TOP_MMU_PLL_STATUS1r_MMU_PLL3_LOCKf_GET(mmu_pll_status1)));
    }
    ioerr += READ_TOP_MMU_PLL_INITr(unit, &pll_init);
    TOP_MMU_PLL_INITr_PLL1_POST_RESETBf_SET(pll_init, 1);
    TOP_MMU_PLL_INITr_PLL2_POST_RESETBf_SET(pll_init, 1);
    TOP_MMU_PLL_INITr_PLL3_POST_RESETBf_SET(pll_init, 1);
    ioerr += WRITE_TOP_MMU_PLL_INITr(unit, pll_init);

    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &soft_reset2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(soft_reset2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_DDR_PLL0_RST_Lf_SET(soft_reset2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, soft_reset2);

    BMD_SYS_USLEEP(wait_usec);

    /* De-assert timesync/broadsync PLL post reset */
    ioerr += READ_TOP_SOFT_RESET_REG_2r(unit, &soft_reset2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(soft_reset2, 1);
    TOP_SOFT_RESET_REG_2r_TOP_DDR_PLL0_POST_RST_Lf_SET(soft_reset2, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REG_2r(unit, soft_reset2);

    BMD_SYS_USLEEP(wait_usec);

    if (!(bcm56450_a0_port_speed_max(unit, GS_PORT) == 2500)) {
        /* Do not support GS port */
        ioerr += READ_TOP_SW_BOND_OVRD_CTRL0r(unit, &bond_ovrd_ctrl0);
        TOP_SW_BOND_OVRD_CTRL0r_OLP_ENABLEf_SET(bond_ovrd_ctrl0, 0);
        ioerr += WRITE_TOP_SW_BOND_OVRD_CTRL0r(unit, bond_ovrd_ctrl0);
    }
    
    /* Bring port blocks out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &soft_reset);
    TOP_SOFT_RESET_REGr_TOP_MXQ0_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ1_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ2_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ3_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ4_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ5_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ6_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ7_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ8_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ9_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ10_RST_Lf_SET(soft_reset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, soft_reset);

    BMD_SYS_USLEEP(wait_usec); 

    /* Bring network sync out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &soft_reset);
    TOP_SOFT_RESET_REGr_TOP_MXQ0_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ1_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ2_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ3_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ4_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ5_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ6_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ7_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ8_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ9_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MXQ10_HOTSWAP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_NS_RST_Lf_SET (soft_reset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, soft_reset);

    BMD_SYS_USLEEP(wait_usec); 

    CMIC_SBUS_TIMEOUTr_SET(sbus_to, 0x7d0);
    ioerr += WRITE_CMIC_SBUS_TIMEOUTr(unit, sbus_to);

    /* Enable packing mode before MMU is out of reset */
    ioerr += READ_TOP_SW_BOND_OVRD_CTRL1r(unit, &bond_ovrd_ctrl1);
    TOP_SW_BOND_OVRD_CTRL1r_MMU_PACKING_ENABLEf_SET(bond_ovrd_ctrl1, 1);
    ioerr += WRITE_TOP_SW_BOND_OVRD_CTRL1r(unit, bond_ovrd_ctrl1);

    /* Bring IP, EP, and MMU blocks out of reset */
    ioerr += READ_TOP_SOFT_RESET_REGr(unit, &soft_reset);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(soft_reset, 1);
    ioerr += WRITE_TOP_SOFT_RESET_REGr(unit, soft_reset);
    BMD_SYS_USLEEP(wait_usec); 

    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (MXQPORT_SUBPORT(unit, port) == 0) {
            ioerr += bcm56450_a0_xport_reset(unit, port);
            /* Bring XMAC out of reset */
            ioerr += READ_XPORT_XMAC_CONTROLr(unit, &xmac_control, port);
            XPORT_XMAC_CONTROLr_XMAC_RESETf_SET(xmac_control, 1);
            ioerr += WRITE_XPORT_XMAC_CONTROLr(unit, xmac_control, port);
            
            BMD_SYS_USLEEP(wait_usec); 
    
            XPORT_XMAC_CONTROLr_XMAC_RESETf_SET(xmac_control, 0);
            ioerr += WRITE_XPORT_XMAC_CONTROLr(unit, xmac_control, port);
            BMD_SYS_USLEEP(wait_usec); 
        }
    }
    
    /* Initialize PHY for all ports */
    CDK_PBMP_ITER(pbmp, port) {
        blkidx = MXQPORT_BLKIDX(unit, port);
        if (bcm56450_a0_phy_connection_mode(unit, blkidx) == PHY_CONN_WARPCORE) {
            bcm56450_a0_warpcore_phy_init(unit, port);
        } else {
            bcm56450_a0_unicore_phy_init(unit, port);
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56450_A0 */
