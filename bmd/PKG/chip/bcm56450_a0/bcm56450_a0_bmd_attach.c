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
#include <bmd/bmd_device.h>
#include <bmd/bmd_phy_ctrl.h>

#include <cdk/chip/bcm56450_a0_defs.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/arch/xgsm_miim.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include "bcm56450_a0_bmd.h"
#include "bcm56450_a0_internal.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm56450_phy_bus[] = {
/* PHY bus driver for Unicore/Unicore (MXQ6/MXQ7) */ 
#ifdef PHY_BUS_BCM56450_MIIM_INT_INSTALLED
    &phy_bus_bcm56450_miim_int,
#endif
#ifdef PHY_BUS_BCM956450K_MIIM_EXT_INSTALLED
    &phy_bus_bcm956450k_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56450_phy_bus_uw[] = {
/* PHY bus driver for Unicore/Warpcore (MXQ6/MXQ7) */ 
#ifdef PHY_BUS_BCM56450_MIIM_INT_INSTALLED
    &phy_bus_bcm56450_miim_int_uw,
#endif
#ifdef PHY_BUS_BCM956450K_MIIM_EXT_INSTALLED
    &phy_bus_bcm956450k_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56450_phy_bus_wu[] = {
/* PHY bus driver for Warpcore/Unicore (MXQ6/MXQ7) */ 
#ifdef PHY_BUS_BCM56450_MIIM_INT_INSTALLED
    &phy_bus_bcm56450_miim_int_wu,
#endif
#ifdef PHY_BUS_BCM956450K_MIIM_EXT_INSTALLED
    &phy_bus_bcm956450k_miim_ext,
#endif
    NULL
};

static phy_bus_t *bcm56450_phy_bus_ww[] = {
/* PHY bus driver for Warpcore/Warpcore (MXQ6/MXQ7) */ 
#ifdef PHY_BUS_BCM56450_MIIM_INT_INSTALLED
    &phy_bus_bcm56450_miim_int_ww,
#endif
#ifdef PHY_BUS_BCM956450K_MIIM_EXT_INSTALLED
    &phy_bus_bcm956450k_miim_ext,
#endif
    NULL
};

#endif /* BMD_CONFIG_INCLUDE_PHY */

/* port speed arrays */
/* Config=1, 8xF.XAUI + 2xW10G */
static int port_speed_max_1[] = {
     0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,  10000,  10000,  10000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0
};

/* Config=2, 7xF.XAUI + 1xXAUI + 2xW10G + OLP */
static int port_speed_max_2[] = {
     0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,  10000,  10000,  10000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=3, 7xF.XAUI + 1xW10G + 1xW20G + OLP */
static int port_speed_max_3[] = {
     0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,  10000,  10000,      0,  10000,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=4, 6xF.XAUI + 2xW20G + OLP */
static int port_speed_max_4[] = {
     0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,      0,  10000,  10000,      0,  10000,      0,      0,
 10000,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=5, 4xF.XAUI + 1xW20G + 1xW40G */
static int port_speed_max_5[] = {
     0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,  10000,  10000,  10000,      0,  10000,      0,      0,
 10000,      0,      0,      0,      0,      0,  10000,      0,
     0,      0
};

/* Config=6, 2xF.XAUI + 2xW40G */
static int port_speed_max_6[] = {
     0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
 10000,  10000,  10000,  10000,      0,  10000,      0,      0,
 10000,      0,      0,  10000,      0,      0,  10000,      0,
     0,      0
};

/* Config=7, 24xGE + 1xF.XAUI + 1xW40G + 1xW20G */
static int port_speed_max_7[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 10000,  10000,  10000,  10000,      0,  10000,      0,      0,
 10000,      0,      0,      0,      0,      0,  10000,      0,
     0,      0
};

/* Config=8, 8xGE + 2xF.XAUI + 1xF.HG13 + 1xX.HG13 + 2xW21G */
static int port_speed_max_8[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
 13000,  13000,  21000,  21000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=9, 24xGE + 1xF.HG13 + 1xX.HG13 + 2xW21G */
static int port_speed_max_9[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 13000,  13000,  21000,  21000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=10, 8xGE + 4xF.XAUI + 2xF.HG13 + 2xW13G */
static int port_speed_max_10[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 13000,  13000,  13000,  13000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0
};

/* Config=11, 8x1GbE + 4xF.XAUI + 1xF.HG[13] + 1xX.HG[13] + 2xW13G */
static int port_speed_max_11[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 13000,  13000,  13000,  13000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=12, 8x1GbE + 4xF.XAUI + 1xF.HG[13] + 1xW13G + 1xW21G*/
static int port_speed_max_12[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 13000,      0,  13000,  21000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=13, 8x1GbE + 4xF.XAUI + 2xW21G */
static int port_speed_max_13[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,      0,  21000,  21000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=14, 8x1GbE + 2xF.XAUI + 1xF.HG[13] + 1xX.HG[13] + 2xW20G */
static int port_speed_max_14[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
 13000,  13000,  13000,  13000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=15, 16x1GbE + 1xF.XAUI + 1xF.HG[13] + 1xX.HG[13] + 2xW20G */
static int port_speed_max_15[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 10000,      0,      0,      0,      0,      0,      0,      0,
 13000,  13000,  13000,  13000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=16, 16x2.5GbE + 1xXAUI */
static int port_speed_max_16[] = {
     0,
  2500,   2500,   2500,   2500,   2500,   2500,   2500,   2500,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
  2500,   2500,   1000,  10000,      0,      0,      0,      0,
     0,      0,   2500,   2500,   2500,   2500,   2500,   2500,
     0,      0
};

/* Config=17, 8x1GbE + 5xXAUI + 1xWC10G + 1xWC21G + OLP */
static int port_speed_max_17[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
 10000,      0,  10000,  21000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=18, 10x1GbE + 8x2.5GbE + OLP */
static int port_speed_max_18[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   2500,   2500,   2500,   2500,      0,      0,      0,
  2500,      0,      0,      0,   2500,      0,      0,      0,
  2500,      0,   2500,   1000,      0,   1000,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=19, 24x1GbE + 2x13HG + 2xW13G */
static int port_speed_max_19[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 13000,  13000,  13000,  13000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0
};

/* Config=20, 24x1GbE + 2x13HG + 2xW13G + OLP */
static int port_speed_max_20[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 13000,  13000,  13000,  13000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=21, 2xXAUI + 2x13HG + 2xW13G */
static int port_speed_max_21[] = {
     0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
 13000,  13000,  13000,  13000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0
};

/* Config=22, 2xXAUI + 2x13HG + 2xW13G + OLP */
static int port_speed_max_22[] = {
     0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
 13000,  13000,  13000,  13000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=23, 24x1GbE + 1x13HG + 1xW13G + 1xW21G + OLP */
static int port_speed_max_23[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 13000,      0,  13000,  21000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=24, 24x1GbE + 2xW21G + OLP */
static int port_speed_max_24[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
     0,      0,  21000,  21000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=25, 24x1GbE + 1xW13G + 1xW21G */
static int port_speed_max_25[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
     0,      0,  13000,  21000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0
};

/* Config=26, 3xXAUI + 1x13HG + 1xW13G + 1xW21G */
static int port_speed_max_26[] = {
     0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
 13000,  10000,  13000,  21000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0
};

/* Config=27, 2xXAUI + 1x13HG + 2xW21G + OLP */
static int port_speed_max_27[] = {
     0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
 13000,      0,  21000,  21000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=28, 8x1GbE + 8x2.5GbE + 3xXAUI + 1xW20G + OLP */
static int port_speed_max_28[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  2500,   2500,   2500,   2500,   2500,   2500,   2500,   2500,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,  10000,      0,  10000,      0,  10000,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=29, 2xXAUI + 2xW13G + OLP */
static int port_speed_max_29[] = {
     0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0,  13000,  13000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,   2500
};

/* Config=30, 2xXAUI + 2x13HG */
static int port_speed_max_30[] = {
     0,
 10000,      0,      0,      0,  10000,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
 13000,  13000,      0,      0,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0
};

/* Config=31, 24x1GbE + 2x13HG + 2xW13G */
static int port_speed_max_31[] = {
     0,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
  1000,   1000,   1000,   1000,   1000,   1000,   1000,   1000,
 13000,  13000,  13000,  13000,      0,      0,      0,      0,
     0,      0,      0,      0,      0,      0,      0,      0,
     0,      0
};


/* tdm arrays */
static int tdm_seq_1[] = {
   1,    5,    9,   27,   26,   13,   17,   21,   25,   28,   41,
   1,    5,    9,   27,   26,   13,   17,   21,   25,   28,    0,
   1,    5,    9,   27,   26,   13,   17,   21,   25,   28,   63,
   1,    5,    9,   27,   26,   13,   17,   21,   25,   28,   62,
  62,    1,    5,    9,   27,   26,   13,   17,   21,   25,   28,
  41,    1,    5,    9,   27,   26,   13,   17,   21,   25,   28,
   0,    1,    5,    9,   27,   26,   13,   17,   21,   25,   28,
  63,    1,    5,    9,   27,   26,   13,   17,   21,   25,   28,
};

static int tdm_seq_2[] = {
   1,    5,    9,   27,   26,   13,   17,   21,   25,   28,   41,
   1,    5,    9,   27,   26,   13,   17,   21,   25,   28,    0,
   1,    5,    9,   27,   26,   13,   17,   21,   25,   28,   40,
   1,    5,    9,   27,   26,   13,   17,   21,   25,   28,   62,
  62,    1,    5,    9,   27,   26,   13,   17,   21,   25,   28,
  41,    1,    5,    9,   27,   26,   13,   17,   21,   25,   28,
   0,    1,    5,    9,   27,   26,   13,   17,   21,   25,   28,
  40,    1,    5,    9,   27,   26,   13,   17,   21,   25,   28,
};

static int tdm_seq_3[] = {
   1,    5,    9,   27,   30,   13,   17,   21,   25,   28,   41,
   1,    5,    9,   27,   30,   13,   17,   21,   25,   28,    0,
   1,    5,    9,   27,   30,   13,   17,   21,   25,   28,   40,
   1,    5,    9,   27,   30,   13,   17,   21,   25,   28,   62,
  62,    1,    5,    9,   27,   30,   13,   17,   21,   25,   28,
  41,    1,    5,    9,   27,   30,   13,   17,   21,   25,   28,
   0,    1,    5,    9,   27,   30,   13,   17,   21,   25,   28,
  40,    1,    5,    9,   27,   30,   13,   17,   21,   25,   28,
};

static int tdm_seq_4[] = {
   1,    5,    9,   27,   30,   13,   17,   21,   33,   28,   41,
   1,    5,    9,   27,   30,   13,   17,   21,   33,   28,    0,
   1,    5,    9,   27,   30,   13,   17,   21,   33,   28,   40,
   1,    5,    9,   27,   30,   13,   17,   21,   33,   28,   62,
  62,    1,    5,    9,   27,   30,   13,   17,   21,   33,   28,
  41,    1,    5,    9,   27,   30,   13,   17,   21,   33,   28,
   0,    1,    5,    9,   27,   30,   13,   17,   21,   33,   28,
  40,    1,    5,    9,   27,   30,   13,   17,   21,   33,   28,
};

static int tdm_seq_5[] = {
   1,    5,   26,   27,   30,   13,    9,   39,   33,   28,   41,
   1,    5,   26,   27,   30,   13,    9,   39,   33,   28,    0,
   1,    5,   26,   27,   30,   13,    9,   39,   33,   28,   63,
   1,    5,   26,   27,   30,   13,    9,   39,   33,   28,   62,
  62,    1,    5,   26,   27,   30,   13,    9,   39,   33,   28,
  41,    1,    5,   26,   27,   30,   13,    9,   39,   33,   28,
   0,    1,    5,   26,   27,   30,   13,    9,   39,   33,   28,
  63,    1,    5,   26,   27,   30,   13,    9,   39,   33,   28,
};

static int tdm_seq_6[] = {
  39,    5,   25,   27,   30,   26,    1,   36,   33,   28,   41,
  39,    5,   25,   27,   30,   26,    1,   36,   33,   28,    0,
  39,    5,   25,   27,   30,   26,    1,   36,   33,   28,   63,
  39,    5,   25,   27,   30,   26,    1,   36,   33,   28,   62,
  62,   39,    5,   25,   27,   30,   26,    1,   36,   33,   28,
  41,   39,    5,   25,   27,   30,   26,    1,   36,   33,   28,
   0,   39,    5,   25,   27,   30,   26,    1,   36,   33,   28,
  63,   39,    5,   25,   27,   30,   26,    1,   36,   33,   28,
};

static int tdm_seq_7[] = {
   1,    5,   26,   27,   30,   25,    9,   39,   33,   28,   41,
  13,   17,   26,   27,   30,   25,   21,   39,   33,   28,    0,
   2,    6,   26,   27,   30,   25,   10,   39,   33,   28,   14,
  18,   22,   26,   27,   30,   25,    3,   39,   33,   28,   62,
  62,    7,   11,   26,   27,   30,   25,   15,   39,   33,   28,
  41,   19,   23,   26,   27,   30,   25,    4,   39,   33,   28,
   0,    8,   12,   26,   27,   30,   25,   16,   39,   33,   28,
  20,   24,   63,   26,   27,   30,   25,   63,   39,   33,   28,
};

static int tdm_seq_8[] = {
   9,   25,   27,   28,   13,   26,    9,   27,   28,   40,   25,
   0,   27,   28,   26,   13,    9,   27,   28,   25,   62,   62,
  27,   26,   28,   13,    9,   27,   25,   28,    3,   13,   26,
  27,   28,   41,    9,   25,   27,   28,    5,   26,   13,   27,
  28,   40,   25,    9,   27,   28,   26,    7,   13,   27,   28,
  25,    1,    9,   27,   26,   28,   63,   13,   27,   25,   28,
   0,    9,   26,   27,   28,   41,   13,   25,   27,   28,    6,
  26,    9,   27,   28,   40,   25,   13,   27,   28,   26,    8,
   9,   27,   28,   25,    2,   13,   27,   26,   28,   63,    9,
  27,   25,   28,    4,   13,   26,   27,   28,   41,
};

static int tdm_seq_9[] = {
   1,   25,   27,   28,    5,   26,    9,   27,   28,   40,   25,
   0,   27,   28,   26,   13,   17,   27,   28,   25,   62,   62,
  27,   26,   28,   21,    2,   27,   25,   28,    6,   10,   26,
  27,   28,   41,   14,   25,   27,   28,   18,   26,   22,   27,
  28,   40,   25,    3,   27,   28,   26,    7,   11,   27,   28,
  25,   15,   19,   27,   26,   28,   23,    4,   27,   25,   28,
   0,    8,   26,   27,   28,   41,   12,   25,   27,   28,   16,
  26,   20,   27,   28,   40,   25,   24,   27,   28,   26,   63,
  63,   27,   28,   25,   63,   63,   27,   26,   28,   63,   63,
  27,   25,   28,   63,   63,   26,   27,   28,   41,
};

static int tdm_seq_10[] = {
  62,   25,    9,   28,   21,   26,   17,   27,   13,    0,   25,
   1,   28,    9,   26,   21,   27,   17,   63,   25,   13,   28,
  41,   26,    9,   27,   21,    2,   25,   17,   28,   13,   26,
   0,   27,    9,    3,   25,   21,   28,   17,   26,   13,   27,
   7,    9,   25,    4,   28,   21,   26,   17,   27,   13,   63,
  25,    9,   28,   41,   26,   21,   27,   17,   13,   25,    5,
  28,    9,   26,    0,   27,   21,   17,   25,   13,   28,    6,
  26,    9,   27,   62,   21,   25,   17,   28,   13,   26,   41,
  27,    9,   63,   25,   21,   28,   17,   26,   13,   27,    8,
   9,   25,   21,   28,   17,   26,   13,   27,   62,
};

static int tdm_seq_11[] = {
  62,   25,    9,   28,   21,   26,   17,   27,   13,    0,   25,
   1,   28,    9,   26,   21,   27,   17,   40,   25,   13,   28,
  41,   26,    9,   27,   21,    2,   25,   17,   28,   13,   26,
   0,   27,    9,    3,   25,   21,   28,   17,   26,   13,   27,
   7,    9,   25,    4,   28,   21,   26,   17,   27,   13,   40,
  25,    9,   28,   41,   26,   21,   27,   17,   13,   25,    5,
  28,    9,   26,    0,   27,   21,   17,   25,   13,   28,    6,
  26,    9,   27,   62,   21,   25,   17,   28,   13,   26,   41,
  27,    9,   40,   25,   21,   28,   17,   26,   13,   27,    8,
   9,   25,   21,   28,   17,   26,   13,   27,   62,
};

static int tdm_seq_12[] = {
  62,   25,    9,   28,   21,   27,   17,   28,   13,    0,   25,
   1,   28,    9,   27,   21,   28,   17,   40,   25,   13,   28,
  41,   27,    9,   28,   21,    2,   25,   17,   28,   13,   27,
   0,   28,    9,    3,   25,   21,   28,   17,   27,   13,   28,
   7,    9,   25,    4,   28,   21,   27,   17,   28,   13,   40,
  25,    9,   28,   41,   27,   21,   28,   17,   13,   25,    5,
  28,    9,   27,    0,   28,   21,   17,   25,   13,   28,    6,
  27,    9,   28,   62,   21,   25,   17,   28,   13,   27,   41,
  28,    9,   40,   25,   21,   28,   17,   27,   13,   28,    8,
   9,   25,   21,   28,   17,   27,   13,   28,   62,
};

static int tdm_seq_13[] = {
  62,   27,    9,   28,   21,   27,   17,   28,   13,    0,   27,
   1,   28,    9,   27,   21,   28,   17,   40,   27,   13,   28,
  41,   27,    9,   28,   21,    2,   27,   17,   28,   13,   27,
   0,   28,    9,    3,   27,   21,   28,   17,   27,   13,   28,
   7,    9,   27,    4,   28,   21,   27,   17,   28,   13,   40,
  27,    9,   28,   41,   27,   21,   28,   17,   13,   27,    5,
  28,    9,   27,    0,   28,   21,   17,   27,   13,   28,    6,
  27,    9,   28,   62,   21,   27,   17,   28,   13,   27,   41,
  28,    9,   40,   27,   21,   28,   17,   27,   13,   28,    8,
   9,   27,   21,   28,   17,   27,   13,   28,   62,
};

static int tdm_seq_14[] = {
  62,   25,    9,   28,   63,   26,   63,   27,   13,    0,   25,
   1,   28,    9,   26,   63,   27,   63,   40,   25,   13,   28,
  41,   26,    9,   27,   63,    2,   25,   63,   28,   13,   26,
   0,   27,    9,    3,   25,   63,   28,   63,   26,   13,   27,
   7,    9,   25,    4,   28,   63,   26,   63,   27,   13,   40,
  25,    9,   28,   41,   26,   63,   27,   63,   13,   25,    5,
  28,    9,   26,    0,   27,   63,   63,   25,   13,   28,    6,
  26,    9,   27,   62,   63,   25,   63,   28,   13,   26,   41,
  27,    9,   40,   25,   63,   28,   63,   26,   13,   27,    8,
   9,   25,   63,   28,   63,   26,   13,   27,   62,
};

static int tdm_seq_15[] = {
  62,   25,    1,   28,    5,   26,    9,   27,   17,    0,   25,
  13,   28,    2,   26,    6,   27,   10,   40,   25,   17,   28,
  41,   26,   14,   27,    3,    7,   25,   11,   28,   17,   26,
   0,   27,   15,    4,   25,    8,   28,   12,   26,   17,   27,
  16,   63,   25,   63,   28,   63,   26,   63,   27,   17,   40,
  25,   63,   28,   41,   26,   63,   27,   63,   17,   25,   63,
  28,   63,   26,    0,   27,   63,   63,   25,   17,   28,   63,
  26,   63,   27,   62,   63,   25,   63,   28,   17,   26,   41,
  27,   63,   40,   25,   63,   28,   63,   26,   17,   27,   63,
  63,   25,   63,   28,   63,   26,   17,   27,   62,
};

static int tdm_seq_16[] = {
  63,   25,   63,    1,   63,    5,   63,   26,   27,   28,   41,
  63,   35,   63,    2,   63,    6,   63,   38,   63,   28,    0,
  63,   36,   63,    3,   63,    7,   63,   39,   63,   28,   63,
  63,   37,   63,    4,   63,    8,   63,   40,   63,   28,   62,
  62,   63,   25,   63,    1,   63,    5,   63,   26,   27,   28,
  41,   63,   35,   63,    2,   63,    6,   63,   38,   63,   28,
   0,   63,   36,   63,    3,   63,    7,   63,   39,   63,   28,
  63,   63,   37,   63,    4,   63,    8,   63,   40,   63,   28,
};

static int tdm_seq_17[] = {
  62,    9,   28,   13,   25,   17,    5,   21,   28,   27,   41,
  25,    9,   28,   13,    1,   17,   25,   21,   28,   27,   40,
   0,    9,   28,   13,   25,   17,    6,   21,   28,   27,   63,
  25,    9,   28,   13,    2,   17,   25,   21,   28,   27,   41,
  40,    9,   28,   13,   25,   17,    7,   21,   28,   27,   63,
  25,    9,   28,   13,    3,   17,   25,   21,   28,   27,   41,
   0,    9,   28,   13,   25,   17,    8,   21,   28,   27,   40,
  25,    9,   28,   13,    4,   17,   25,   21,   28,   27,   62,
};

static int tdm_seq_18[] = {
   1,   63,   13,   27,   41,    2,   10,   17,   63,    0,    3,
  11,   21,   63,   40,    4,   12,   25,   63,   62,   63,    9,
  13,   27,    5,   41,   10,   17,   28,    6,    0,   11,   21,
  30,    7,   40,   12,   25,   63,    8,
};

static int tdm_seq_19[] = {
  41,    9,   17,   25,   26,   27,   28,    0,   10,   18,   25,
  26,   27,   28,    1,   41,   19,   25,   26,   27,   28,    2,
  11,   20,   25,   26,   27,   28,    3,   12,   41,   25,   26,
  27,   28,    4,   13,    0,   25,   26,   27,   28,   41,   14,
  21,   25,   26,   27,   28,   63,   15,   22,   25,   26,   27,
  28,    5,   41,   23,   25,   26,   27,   28,    6,    0,   24,
  25,   26,   27,   28,    7,   16,   41,   25,   26,   27,   28,
   8,   62,   62,   25,   26,   27,   28,
};

static int tdm_seq_20[] = {
   1,    9,   41,   20,   27,   28,   25,   26,    2,   10,   17,
  40,   27,   28,   25,   26,    3,   11,    0,   21,   27,   28,
  25,   26,    4,   12,   18,   22,   27,   28,   25,   26,    5,
  13,   41,   23,   27,   28,   25,   26,    6,   14,   19,   40,
  27,   28,   25,   26,    7,   15,    0,   24,   27,   28,   25,
  26,    8,   16,   62,   62,   27,   28,   25,   26,
};

static int tdm_seq_21[] = {
   1,    5,   41,   63,   27,   28,   25,   26,    1,    5,   63,
  63,   27,   28,   25,   26,    1,    5,    0,   63,   27,   28,
  25,   26,    1,    5,   63,   63,   27,   28,   25,   26,    1,
   5,   41,   63,   27,   28,   25,   26,    1,    5,   63,   63,
  27,   28,   25,   26,    1,    5,    0,   63,   27,   28,   25,
  26,    1,    5,   62,   62,   27,   28,   25,   26,
};

static int tdm_seq_22[] = {
   1,    5,   41,   63,   27,   28,   25,   26,    1,    5,   63,
  40,   27,   28,   25,   26,    1,    5,    0,   63,   27,   28,
  25,   26,    1,    5,   63,   63,   27,   28,   25,   26,    1,
   5,   41,   63,   27,   28,   25,   26,    1,    5,   63,   40,
  27,   28,   25,   26,    1,    5,    0,   63,   27,   28,   25,
  26,    1,    5,   62,   62,   27,   28,   25,   26,
};

static int tdm_seq_23[] = {
  63,    1,   27,   28,    5,   41,   25,    9,   28,   27,   13,
  40,   28,   17,   25,   63,   21,   27,   28,    2,   41,   25,
   6,   28,   27,   10,    0,   28,   14,   25,   40,   18,   27,
  28,   22,   63,   25,    3,   28,   27,    7,   63,   28,   11,
  25,    0,   15,   27,   28,   19,   41,   25,   23,   28,   27,
   4,   62,   62,   28,    8,   25,   12,   27,   28,   16,   40,
  25,   20,   28,   27,   24,   63,   28,   63,   25,   41,   63,
  27,   28,   63,    0,   25,   63,   28,   27,   63,   63,   28,
  63,   25,
};

static int tdm_seq_24[] = {
   1,   27,    5,   28,    9,   13,   27,   63,   28,   63,   17,
  27,   21,   28,    2,    0,   27,    6,   28,   41,   10,   27,
  14,   28,   63,   18,   27,   40,   28,   22,    3,   27,    7,
  28,   62,   62,   27,   11,   28,   15,   19,   27,    0,   28,
  23,    4,   27,    8,   28,   41,   63,   27,   12,   28,   16,
  20,   27,   40,   28,   24,
};

static int tdm_seq_25[] = {
  63,    1,   27,   28,    5,   41,   63,    9,   28,   27,   13,
  40,   28,   17,   63,   63,   21,   27,   28,    2,   41,   63,
   6,   28,   27,   10,    0,   28,   14,   63,   40,   18,   27,
  28,   22,   63,   63,    3,   28,   27,    7,   63,   28,   11,
  63,    0,   15,   27,   28,   19,   41,   63,   23,   28,   27,
   4,   62,   62,   28,    8,   63,   12,   27,   28,   16,   40,
  63,   20,   28,   27,   24,   63,   28,   63,   63,   41,   63,
  27,   28,   63,    0,   63,   63,   28,   27,   63,   63,   28,
  63,   63,
};

static int tdm_seq_26[] = {
  63,    1,   27,   28,    5,   41,   25,   26,   28,   27,    1,
  40,   28,    5,   25,   63,   26,   27,   28,    1,   41,   25,
   5,   28,   27,   26,    0,   28,    1,   25,   40,    5,   27,
  28,   26,   63,   25,    1,   28,   27,    5,   63,   28,   26,
  25,    0,    1,   27,   28,    5,   41,   25,   26,   28,   27,
   1,   62,   62,   28,    5,   25,   26,   27,   28,    1,   40,
  25,    5,   28,   27,   26,   63,   28,    1,   25,   41,    5,
  27,   28,   26,    0,   25,    1,   28,   27,    5,   63,   28,
  26,   25,
};

static int tdm_seq_27[] = {
   1,   27,   25,   28,    5,    1,   27,   63,   28,   63,   25,
  27,    5,   28,    1,    0,   27,   25,   28,   41,    5,   27,
   1,   28,   63,   25,   27,   40,   28,    5,    1,   27,   25,
  28,   62,   62,   27,    5,   28,    1,   25,   27,    0,   28,
   5,    1,   27,   25,   28,   41,   63,   27,    5,   28,    1,
  25,   27,   40,   28,    5,
};

static int tdm_seq_28[] = {
  62,    9,   13,   28,   26,   21,   17,   30,    1,    5,   10,
  14,   28,   26,   21,   17,   30,    0,    2,   11,   15,   28,
  26,   21,   17,   30,   40,    6,   12,   16,   28,   26,   21,
  17,   30,   63,    3,    9,   13,   28,   26,   21,   17,   30,
  63,    7,   10,   14,   28,   26,   21,   17,   30,    0,    4,
  11,   15,   28,   26,   21,   17,   30,   40,    8,   12,   16,
  28,   26,   21,   17,   30,   62,
};

static int tdm_seq_29[] = {
   1,    5,   27,   28,   41,    1,    5,   27,   28,    0,    1,
   5,   27,   28,   40,    1,    5,   27,   28,   62,   62,    1,
   5,   27,   28,   41,    1,    5,   27,   28,    0,    1,    5,
  27,   28,   40,    1,    5,   27,   28,
};

static int tdm_seq_30[] = {
   1,    5,   25,   26,   41,    1,    5,   25,   26,    0,    1,
   5,   25,   26,   63,    1,    5,   25,   26,   62,   62,    1,
   5,   25,   26,   41,    1,    5,   25,   26,    0,    1,    5,
  25,   26,   63,    1,    5,   25,   26,
};

static int tdm_seq_31[] = {
  41,    9,   17,   25,   26,   27,   28,    0,   10,   18,   25,
  26,   27,   28,    1,   41,   19,   25,   26,   27,   28,    2,
  11,   20,   25,   26,   27,   28,    3,   12,   41,   25,   26,
  27,   28,    4,   13,    0,   25,   26,   27,   28,   41,   14,
  21,   25,   26,   27,   28,   63,   15,   22,   25,   26,   27,
  28,    5,   41,   23,   25,   26,   27,   28,    6,    0,   24,
  25,   26,   27,   28,    7,   16,   41,   25,   26,   27,   28,
   8,   62,   62,   25,   26,   27,   28,
};

static struct _support_cfgs_s {
    int def_cfg;
    int count;
    int cfgs[17];
} _support_cfgs[] = {
    /* 0, Default */
    { 4, 16, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}},
    /* 1, CHIP_FLAG_ACCESS */
    { 4, 17, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 18}},
    /* 2, CHIP_FLAG_BW75G */
    {19, 10, {19, 20, 21, 22, 23, 24, 25, 26, 27, 28}},
    /* 3, CHIP_FLAG_BW44G */
    {29,  2, {29, 30}},
    /* 4, CHIP_FLAG_BW76G */
    {31,  1, {31}}
};

#define _CFG_MAP(_cfg) { COUNTOF(_cfg), _cfg }
typedef struct _cfg_map_s {
    int size;
    const int *val;
} _cfg_map_t;


#define _PORT_CFG(_id) { \
    _id, \
    _CFG_MAP(port_speed_max_##_id), \
    _CFG_MAP(tdm_seq_##_id) \
}

typedef struct _port_cfg_s {
    int id;
    _cfg_map_t speed_max;
    _cfg_map_t tdm;
} _port_cfg_t;


static _port_cfg_t _port_cfgs[] = {
    _PORT_CFG(1),
    _PORT_CFG(2),
    _PORT_CFG(3),
    _PORT_CFG(4),
    _PORT_CFG(5),
    _PORT_CFG(6),
    _PORT_CFG(7),
    _PORT_CFG(8),
    _PORT_CFG(9),
    _PORT_CFG(10),
    _PORT_CFG(11),
    _PORT_CFG(12),
    _PORT_CFG(13),
    _PORT_CFG(14),
    _PORT_CFG(15),
    _PORT_CFG(16),
    _PORT_CFG(17),
    _PORT_CFG(18),
    _PORT_CFG(19),
    _PORT_CFG(20),
    _PORT_CFG(21),
    _PORT_CFG(22),
    _PORT_CFG(23),
    _PORT_CFG(24),
    _PORT_CFG(25),
    _PORT_CFG(26),
    _PORT_CFG(27),
    _PORT_CFG(28),
    _PORT_CFG(29),
    _PORT_CFG(30),
    _PORT_CFG(31)
};

/* Port configuration index for this unit */
static int _cfg_idx[BMD_CONFIG_MAX_UNITS];
static struct _mxqblock_map_s {
    struct {
        int port[MXQPORTS_PER_BLOCK];
    } blk[NUM_MXQBLOCKS];
} _mxqblock_map[BMD_CONFIG_MAX_UNITS];

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1

static struct _port_map_s {
    cdk_port_map_port_t map[BMD_CONFIG_MAX_PORTS];
} _port_map[BMD_CONFIG_MAX_UNITS];

static void
_init_port_map(int unit)
{
    cdk_pbmp_t pbmp;
    int port, pidx = 1;

    CDK_MEMSET(&_port_map[unit], -1, sizeof(_port_map[unit]));

    _port_map[unit].map[0] = CMIC_PORT;

    bcm56450_a0_mxqport_pbmp_get(unit, &pbmp);
    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        CDK_PBMP_ITER(pbmp, port) {
            if (P2L(unit, port) > 0) {
                pidx = CDK_PORT_CONFIG_SYS_PORT(unit, port);
                if (pidx >= 0) {
                    _port_map[unit].map[pidx] = port;
                }
            }
        }
    } else {
        CDK_PBMP_ITER(pbmp, port) {
            if (P2L(unit, port) > 0) {
                _port_map[unit].map[pidx] = port;
                pidx++;
            }
        }
    }

    CDK_PORT_MAP_SET(unit, _port_map[unit].map, pidx + 1);
}
#endif /* CDK_CONFIG_INCLUDE_PORT_MAP */

static int
_port_cfg_index(int port_cfg)
{
    int idx;

    for (idx = 0; idx < COUNTOF(_port_cfgs); idx++) {
        if (port_cfg == _port_cfgs[idx].id) {
            return idx;
        }
    }
    return -1;
}

int
bcm56450_a0_mxqport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, pbmp);
    CDK_PBMP_ITER(*pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    return 0;
}

int
bcm56450_a0_mxqport_block_port_get(int unit, int port)
{
    cdk_xgsm_pblk_t pblk;

    if (cdk_xgsm_port_block(unit, port, &pblk, BLKTYPE_MXQPORT)) {
        return -1 ;
    } 
    
    return pblk.bport;
}

int
bcm56450_a0_mxqport_block_index_get(int unit, int port)
{
    cdk_xgsm_pblk_t pblk;

    if (cdk_xgsm_port_block(unit, port, &pblk, BLKTYPE_MXQPORT)) {
        return -1 ;
    } 
    
    return pblk.bindex;
}

int
bcm56450_a0_mxqport_from_index(int unit, int blkidx, int blkport)
{
    if (blkidx >= NUM_MXQBLOCKS || blkport >= MXQPORTS_PER_BLOCK) {
        return -1;
    }
    
    return _mxqblock_map[unit].blk[blkidx].port[blkport];
}

uint32_t
bcm56450_a0_port_speed_max(int unit, int port)
{
    _cfg_map_t *pmap = &_port_cfgs[_cfg_idx[unit]].speed_max;

    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        return CDK_PORT_CONFIG_SPEED_MAX(unit, port);
    }

    if (port < 0 || port >= pmap->size) {
        return -1;
    }
    if (pmap->val[port] < 0) {
        return 0;
    }

    return pmap->val[port];
}

int
bcm56450_a0_port_num_lanes(int unit, int port)
{
    int blkidx, subidx;
    int num = 0, lanes;
    
    blkidx = MXQPORT_BLKIDX(unit, port);

    for (subidx = 0; subidx < MXQPORTS_PER_BLOCK; subidx++) {
        if (bcm56450_a0_mxqport_from_index(unit, blkidx, subidx) > 0) {
            num++;
        }
    }
    if (num == 1) {
        lanes = 4;
    } else if (num == 2) {
        lanes = 2;
    } else {
        lanes = 1;
    }
    
    return lanes;
}

int
bcm56450_a0_p2l(int unit, int port, int inverse)
{
    if (inverse) {
        if (port == GS_LPORT) {
            return GS_PORT;
        }
    } else {
        if (port == GS_PORT) {
            return GS_LPORT;
        }
    }
    return port;
}

int
bcm56450_a0_tdm_default(int unit, const int **tdm_seq)
{
    _cfg_map_t *tdm = &_port_cfgs[_cfg_idx[unit]].tdm;

    *tdm_seq = tdm->val;

    return tdm->size;
}

int 
bcm56450_a0_phy_connection_mode(int unit, uint32_t mxqblock)
{
    int idx, speed, count = 0;
    int port_list[2][8] = {{25, 35, 36, 37, 27, 32, 33, 34},
                           {26, 38, 39, 40, 28, 29, 30, 31}};
    
    if (mxqblock == 8 || mxqblock == 9) {
        return PHY_CONN_WARPCORE;
    } 
    
    if (mxqblock == 6 || mxqblock == 7) {
        for (idx = 0; idx < 8; idx++) {
            speed = bcm56450_a0_port_speed_max(unit, port_list[mxqblock-6][idx]);
            if (speed == 10000) {
                count++;
            }
        }
      
        if (count == MXQPORTS_PER_BLOCK) {
            return PHY_CONN_WARPCORE;
        }
    }
    
    return PHY_CONN_UNICORE;
}

int
bcm56450_a0_phy_mode_get(int unit, int port, int speed, 
                                    int *phy_mode, int *wc_sel)
{
    int blkidx, phy_conn_mode;

    blkidx = MXQPORT_BLKIDX(unit, port);
    phy_conn_mode = bcm56450_a0_phy_connection_mode(unit, blkidx);

    /* Get PHY Port Mode according to speed */
    *phy_mode = 0;
    if (speed <= 2500) {
        *phy_mode = 2;
    } else if (phy_conn_mode == PHY_CONN_WARPCORE) {
        *phy_mode = (speed == 10000) ? 2 : 0;
    } 
    
    *wc_sel = 0;
    if (phy_conn_mode == PHY_CONN_WARPCORE) {
        if ((speed == 10000) || (speed == 21000)) {
            *wc_sel = 1;
        }
    }
    
    return CDK_E_NONE;
}

int
bcm56450_a0_bmd_attach(int unit)
{
    int port, blkidx, blkport;
    cdk_pbmp_t pbmp;
    int port_cfg, list_id;
#if BMD_CONFIG_INCLUDE_PHY == 1
    int mxq_phy_mode6, mxq_phy_mode7;
#endif

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    CDK_MEMSET(_mxqblock_map, 0, sizeof(_mxqblock_map));
    
    /* Get support config list ID */
    list_id = 0;
    if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_ACCESS) {
        list_id = 1;
    } else if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_BW75G) {
        list_id = 2;
    } else if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_BW44G) {
        list_id = 3;
    } else if (CDK_XGSM_FLAGS(unit) & CHIP_FLAG_BW76G) {
        list_id = 4;
    }
    
    /* Select port configuration */
    _cfg_idx[unit] = -1;
    port_cfg = CDK_CHIP_CONFIG(unit);
    if (port_cfg > 0 && port_cfg <= _support_cfgs[list_id].count) {
         _cfg_idx[unit] = _port_cfg_index(_support_cfgs[list_id].cfgs[port_cfg - 1]);
    }
    if (_cfg_idx[unit] < 0) {
        if (port_cfg > 0) {
            /* Warn if unsupported configuration was requested */
            CDK_WARN(("bcm56450_a0_bmd_attach[%d]: Invalid port config (%d)\n",
                      unit, port_cfg));
        }
        /* Fall back to default port configuration */
        _cfg_idx[unit] = _port_cfg_index(_support_cfgs[list_id].def_cfg);
    }

#if BMD_CONFIG_INCLUDE_HIGIG == 1 
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        int speed_max;

        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        
        speed_max = bcm56450_a0_port_speed_max(unit, port);
        if (speed_max <= 0) {
            continue;
        }
        
        if (speed_max < 10000) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_GE;
        } else if (speed_max > 10000) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_HG;
        } else {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
        }
    }
#endif /* BMD_CONFIG_INCLUDE_HIGIG */

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    /* Attach the PHY bus driver */
#if BMD_CONFIG_INCLUDE_PHY == 1
    mxq_phy_mode6 = bcm56450_a0_phy_connection_mode(unit, 6);
    mxq_phy_mode7 = bcm56450_a0_phy_connection_mode(unit, 7);
    
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);

        if (mxq_phy_mode6 == PHY_CONN_WARPCORE && 
                                mxq_phy_mode7 == PHY_CONN_WARPCORE) {
            bmd_phy_bus_set(unit, port, bcm56450_phy_bus_ww);
        } else if (mxq_phy_mode6 == PHY_CONN_WARPCORE && 
                                mxq_phy_mode7 == PHY_CONN_UNICORE) {
            bmd_phy_bus_set(unit, port, bcm56450_phy_bus_wu);
        } else if (mxq_phy_mode6 == PHY_CONN_UNICORE && 
                                mxq_phy_mode7 == PHY_CONN_WARPCORE) {
            bmd_phy_bus_set(unit, port, bcm56450_phy_bus_uw);
        } else {
            bmd_phy_bus_set(unit, port, bcm56450_phy_bus);
        }
    }
#endif /* BMD_CONFIG_INCLUDE_PHY */

    /* 
     * _mxqblock_ports is used to store the MXQ block information 
     * That is required to process the ports in serdes order instead of in physical order. 
     */
    bcm56450_a0_mxqport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        blkidx = MXQPORT_BLKIDX(unit, port);
        blkport = MXQPORT_SUBPORT(unit, port);
        if ((blkidx >= 0 && blkidx < NUM_MXQBLOCKS) && 
            (blkport >= 0 && blkport < MXQPORTS_PER_BLOCK)) {
            _mxqblock_map[unit].blk[blkidx].port[blkport] = port;
        }
    }
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    /* Match default API port map to configured logical ports */
    _init_port_map(unit);
#endif /* CDK_CONFIG_INCLUDE_PORT_MAP */

    /* Initialize debug functions */
    BMD_PORT_SPEED_MAX(unit) = bcm56450_a0_port_speed_max;
    BMD_PORT_P2L(unit) = bcm56450_a0_p2l;
    BMD_PORT_P2M(unit) = bcm56450_a0_p2l;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56450_A0 */
