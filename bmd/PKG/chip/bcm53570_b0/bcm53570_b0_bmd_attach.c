/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53570_B0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <bmd/bmd_phy_ctrl.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm53570_b0_defs.h>
#include <cdk/cdk_util.h>

#include "bcm53570_b0_bmd.h"
#include "bcm53570_b0_internal.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm53570_phy_bus[] = {
#ifdef PHY_BUS_BCM53570_B0_MIIM_INT_INSTALLED
    &phy_bus_bcm53570_b0_miim_int,
#endif
    NULL
};
#endif /* BMD_CONFIG_INCLUDE_PHY */

/* OPTION_1_0 */
static int p2l_mapping_10[] = {
     0, -1,
    /* TSC4L 0~5 */
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};

static int port_speed_max_10[] = {
     0, -1,
    /* TSC4L 0~5 */
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_2_0, 24P 2.5G */
static int p2l_mapping_20[] = {
     0, -1,
    /* TSC4L 0~5 */
     2, 3, 4, 5,
     6, 7, 8, 9,
     10, 11, 12, 13,
     14, 15, 16, 17,
     18, 19, 20, 21,
     22, 23, 24, 25,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_20[] = {
     0, -1,
    /* TSC4L 0~5 */
     25, 25, 25, 25,
     25, 25, 25, 25,
     25, 25, 25, 25,
     25, 25, 25, 25,
     25, 25, 25, 25,
     25, 25, 25, 25,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_3_1, 4P 1G + 32P 1G */
static int p2l_mapping_31[] = {
     0, -1,
    /* TSC4L 0~5 */
     2, 3, 4, 5,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     6, 7, 8, 9, 10, 11, 12, 13,
     14, 15, 16, 17, 18, 19, 20, 21,
     22, 23, 24, 25, 26, 27, 28, 29,
     30, 31, 32, 33, 34, 35, 36, 37,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_31[] = {
     0, -1,
    /* TSC4L 0~5 */
     10, 10, 10, 10,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     10, 10, 10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_4_1, 48P 2.5G + 4P 25G + 2P 40G  */
static int p2l_mapping_41[] = {
     0, -1,
    /* TSC4L 0~5 */
     2, 3, 4, 5,
     6, 7, 8, 9,
     10, 11, 12, 13,
     14, 15, 16, 17,
     18, 19, 20, 21,
     22, 23, 24, 25,
    /* TSC4Q 0~1 */
     26, 27, 28, 29, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     30, 31, 32, 33, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_41[] = {
     0, -1,
    /* TSC4L 0~5 */
     25, 25, 25, 25,
     25, 25, 25, 25,
     25, 25, 25, 25,
     25, 25, 25, 25,
     25, 25, 25, 25,
     25, 25, 25, 25,
    /* TSC4Q 0~1 */
     25, 25, 25, 25, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     25, 25, 25, 25, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_5_0, 24P 2.5G + 4P 25G + 8P 10G */
static int p2l_mapping_50[] = {
     0, -1,
    /* TSC4L 0~5 */
     2, 3, 4, 5,
     6, 7, 8, 9,
     10, 11, 12, 13,
     14, 15, 16, 17,
     18, 19, 20, 21,
     22, 23, 24, 25,
    /* TSC4Q 0~1 */
     26, 27, 28, 29, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     30, 31, 32, 33, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_50[] = {
     0, -1,
    /* TSC4L 0~5 */
     25, 25, 25, 25,
     25, 25, 25, 25,
     25, 25, 25, 25,
     25, 25, 25, 25,
     25, 25, 25, 25,
     25, 25, 25, 25,
    /* TSC4Q 0~1 */
     25, 25, 25, 25, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     25, 25, 25, 25, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_6_0, 36P 1G + 12P 10G + 4P 25G + 8P 10G */
static int p2l_mapping_60[] = {
     0, -1,
    /* TSC4L 0~5 */
     2, 3, 4, 5,
     6, 7, 8, 9,
     10, 11, 12, 13,
     14, 15, 16, 17,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     18, 19, 20, 21, 22, 23, 24, 25,
     26, 27, 28, 29, 30, 31, 32, 33,
     34, 35, 36, 37, 38, 39, 40, 41,
     42, 43, 44, 45, 46, 47, 48, 49,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_60[] = {
     0, -1,
    /* TSC4L 0~5 */
     10, 10, 10, 10,
     10, 10, 10, 10,
     10, 10, 10, 10,
     10, 10, 10, 10,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     10, 10, 10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_7_0, 12P 1G + 12P 10G + 4P 25G + 8P 10G */
static int p2l_mapping_70[] = {
     0, -1,
    /* TSC4L 0~5 */
     2, 3, 4, 5,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_70[] = {
     0, -1,
    /* TSC4L 0~5 */
     10, 10, 10, 10,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_7_2, 12P 1G + 12P 10G + 2P 50G + 8P 10G */
static int p2l_mapping_72[] = {
     0, -1,
    /* TSC4L 0~5 */
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_72[] = {
     0, -1,
    /* TSC4L 0~5 */
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_8_2, 56P 1G + 2P 50G  + 4P 10G */
static int p2l_mapping_82[] = {
     0, -1,
    /* TSC4L 0~5 */
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     2, 3, 4, 5,
    /* TSC4Q 0~1 */
     6, 7, 8, 9, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     10, 11, 12, 13, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_82[] = {
     0, -1,
    /* TSC4L 0~5 */
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     25, 25, 25, 25,
    /* TSC4Q 0~1 */
     25, 25, 25, 25, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     25, 25, 25, 25, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_9_2, 48P 1G + 2P 50G  + 8P 10G */
static int p2l_mapping_92[] = {
     0, -1,
    /* TSC4L 0~5 */
     2, 3, 4, 5,
     6, 7, 8, 9,
     10, 11, 12, 13,
     14, 15, 16, 17,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     18, 19, 20, 21, 22, 23, 24, 25,
     26, 27, 28, 29, 30, 31, 32, 33,
     34, 35, 36, 37, 38, 39, 40, 41,
     42, 43, 44, 45, 46, 47, 48, 49,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_92[] = {
     0, -1,
    /* TSC4L 0~5 */
     10, 10, 10, 10,
     10, 10, 10, 10,
     10, 10, 10, 10,
     10, 10, 10, 10,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     10, 10, 10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_10_2, 16P 1G (SGMII) + 16P 1G (QSGMII) + 2P 50G + 8P 10G */
static int p2l_mapping_102[] = {
     0, -1,
    /* TSC4L 0~5 */
     2, 3, 4, 5,
     6, 7, 8, 9,
     10, 11, 12, 13,
     14, 15, 16, 17,
     18, 19, 20, 21,
     22, 23, 24, 25,
    /* TSC4Q 0~1 */
     26, 27, 28, 29, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     30, 31, 32, 33, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_102[] = {
     0, -1,
    /* TSC4L 0~5 */
     10, 10, 10, 10,
     10, 10, 10, 10,
     10, 10, 10, 10,
     10, 10, 10, 10,
     10, 10, 10, 10,
     10, 10, 10, 10,
    /* TSC4Q 0~1 */
     10, 10, 10, 10, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     10, 10, 10, 10, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_11_2, 32P 1G (QSGMII)+ 2P 50G + 8P 10G */
static int p2l_mapping_112[] = {
     0, -1,
    /* TSC4L 0~5 */
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     2, 3, 4, 5, 6, 7, 8, 9,
     10, 11, 12, 13, 14, 15, 16, 17,
     18, 19, 20, 21, 22, 23, 24, 25,
     26, 27, 28, 29, 30, 31, 32, 33,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_112[] = {
     0, -1,
    /* TSC4L 0~5 */
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     10, 10, 10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10,
     10, 10, 10, 10, 10, 10, 10, 10,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_12_B, 8P 1G (QGMII) + 4P 10G */
static int p2l_mapping_122[] = {
     0, -1,
    /* TSC4L 0~5 */
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_122[] = {
     0, -1,
    /* TSC4L 0~5 */
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_12_0, 8P 1G (QGMII) + 4P 10G */
static int p2l_mapping_120[] = {
     0, -1,
    /* TSC4L 0~5 */
     2, 3, 4, 5,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_120[] = {
     0, -1,
    /* TSC4L 0~5 */
     10, 10, 10, 10,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


/* OPTION_13_0, 8P 1G (SGMII) + 4P 10G */
static int p2l_mapping_130[] = {
     0, -1,
    /* TSC4L 0~5 */
     2, 3, 4, 5,
     6, 7, 8, 9,
     10, 11, 12, 13,
     14, 15, 16, 17,
     18, 19, 20, 21,
     22, 23, 24, 25,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};
static int port_speed_max_130[] = {
     0, -1,
    /* TSC4L 0~5 */
     10, 10, 10, 10,
     10, 10, 10, 10,
     10, 10, 10, 10,
     10, 10, 10, 10,
     10, 10, 10, 10,
     10, 10, 10, 10,
    /* TSC4Q 0~1 */
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
    /* TSC 0~6*/
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
     -1, -1, -1, -1,
    /* TSCF 0*/
     -1, -1, -1, -1
};


#define _CFG_MAP(_cfg) { COUNTOF(_cfg), _cfg }
typedef struct _cfg_map_s {
    int size;
    int *val;
} _cfg_map_t;

#define _PORT_CFGS(_id) { \
    _id, \
    _CFG_MAP(p2l_mapping_##_id), \
    _CFG_MAP(port_speed_max_##_id), \
}

typedef struct _port_cfg_s {
    uint32_t id;
    _cfg_map_t p2l;
    _cfg_map_t speed_max;
} _port_cfg_t;

static _port_cfg_t _port_cfgs[] = {
    _PORT_CFGS(10),
    _PORT_CFGS(20),
    _PORT_CFGS(31),
    _PORT_CFGS(41),
    _PORT_CFGS(50),
    _PORT_CFGS(60),
    _PORT_CFGS(70),
    _PORT_CFGS(72),
    _PORT_CFGS(82),
    _PORT_CFGS(92),
    _PORT_CFGS(102),
    _PORT_CFGS(112),
    _PORT_CFGS(122),
    _PORT_CFGS(120),
    _PORT_CFGS(130),
};

/*
 * Table to translate between external (PRD) and internal port
 * configuration IDs.
 */
static struct _port_cfg_xlate_s {
    int count;
    uint32_t cfg_opt[30];   /* Port configuration options as per PRD */
    int cfg_id[30];    /* Internal port configuration IDs */
} _port_cfg_xlate[] = {
    /* default, keep option1 */
    {1, {0x10},
         { 10}},
    /* op1_0 */
    {1, {0x10},
         { 10}},
    /* op2_0 */
    {1, {0x20},
        {  20}},
    /* op3_1 */
    {1, {0x31},
        {  31}},
    /* op4_1 */
    {1, {0x41},
        {  41}},
    /* op5_0 */
    {1, {0x50},
        {  50}},
    /* op6_0 */
    {1, {0x60},
        {  60}},
    /* op7_0,op7_2 */
    {2, {0x70, 0x72},
        {  70,   72}},
    /* op8_2 */
    {1, { 0x82},
        {   82}},
    /* op9_2 */
    {1, { 0x92},
        {   92}},
    /* op10_2 */
    {1, { 0x102},
        {   102}},
    /* op11_2 */
    {1, { 0x112},
        {   112}},
    /* op12_2, 12_0 */
    {2, {0x122, 0x120},
        {  122,   120}},
    /* op13_0 */
    {1, {0x130},
        {  130}}
};

static int _pcfg_idx[BMD_CONFIG_MAX_UNITS];

static const int tsc_phy_port[TSC_MAX_BLK_COUNT] = {58, 62, 66, 70, 74, 78, 82};
static const int qtc_phy_port[QTC_MAX_BLK_COUNT] = {26, 42};

/*
 * Unused SerDes cores can be disabled through H/W pin strapping.
 * This table tracks which ports have been disabled this way.
 */
static int _serdes_disabled[NUM_PHYS_PORTS] = { 0 };

static struct _mmu_port_map_s {
    cdk_port_map_port_t map[BMD_CONFIG_MAX_PORTS];
} _mmu_port_map[BMD_CONFIG_MAX_UNITS];

static int freq = 583;

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static struct _port_map_s {
    cdk_port_map_port_t map[BMD_CONFIG_MAX_PORTS];
} _port_map[BMD_CONFIG_MAX_UNITS];

static void
_init_port_map(int unit)
{
    cdk_pbmp_t pbmp;
    int lport, lport_max = 0;
    int port;

    CDK_MEMSET(&_port_map[unit], -1, sizeof(_port_map[unit]));

    _port_map[unit].map[0] = CMIC_PORT;

    bcm53570_b0_all_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        lport = P2L(unit, port);
        _port_map[unit].map[lport] = port;
        if (lport > lport_max) {
            lport_max = lport;
        }
    }

    CDK_PORT_MAP_SET(unit, _port_map[unit].map, lport_max + 1);
}
#endif /* CDK_CONFIG_INCLUDE_PORT_MAP */

int
bcm53570_b0_gport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, pbmp);
    CDK_PBMP_ITER(*pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    return 0;
}

int
bcm53570_b0_tscq_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, pbmp);
    CDK_PBMP_ITER(*pbmp, port) {
        if ((BMD_PORT_PROPERTIES(unit, port) == 0) || (port < MIN_TSCQ_PHY_PORT)) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    return 0;
}

int
bcm53570_b0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, pbmp);
    CDK_PBMP_ITER(*pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    return 0;
}

int
bcm53570_b0_clport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_CLPORT, pbmp);
    CDK_PBMP_ITER(*pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    return 0;
}

/* All front ports without CMIC_port */
int
bcm53570_b0_all_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    cdk_pbmp_t pbmp_add;
    int port;

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, pbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp_add);
    CDK_PBMP_OR(*pbmp, pbmp_add);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_CLPORT, &pbmp_add);
    CDK_PBMP_OR(*pbmp, pbmp_add);
    CDK_PBMP_ITER(*pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }

    return 0;
}

/*
 * This function is called after the chip is out of reset and
 * the H/W pin strapping has been read from a chip register.
 * Port mappings and port properties will be adjusted accordingly
 * before the chip is initialized.
 */
int
bcm53570_b0_block_disable(int unit, uint32_t disable_tsc, uint32_t disable_qtc)
{
    int tidx, pidx;
    int start_port;
    cdk_pbmp_t pbmp;

    if (disable_tsc) {
        for (tidx = 0; tidx < TSC_MAX_BLK_COUNT; tidx++) {
            if ((disable_tsc >> tidx) & 0x1) {
                start_port = tsc_phy_port[tidx];

                for (pidx = 0; pidx < PORT_COUNT_PER_TSC; pidx++) {
                    _serdes_disabled[start_port + pidx] = 1;
                    BMD_PORT_PROPERTIES(unit, start_port + pidx) = 0;
                }
            }
        }
    }

    if (disable_qtc) {
        bcm53570_b0_tscq_pbmp_get(unit, &pbmp);

        for (tidx = 0; tidx < QTC_MAX_BLK_COUNT; tidx++) {
            if ((disable_qtc >> tidx) & 0x1) {
                start_port = qtc_phy_port[tidx];

                for (pidx = 0; pidx < PORT_COUNT_PER_QTC; pidx++) {
                    if (CDK_PBMP_MEMBER(pbmp, start_port + pidx)) {
                        continue;
                    }
                    _serdes_disabled[start_port + pidx] = 1;
                    BMD_PORT_PROPERTIES(unit, start_port + pidx) = 0;
                }
            }
        }
    }

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    _init_port_map(unit);
#endif /* CDK_CONFIG_INCLUDE_PORT_MAP */

    return 0;
}

/* inverse=0: input port is physical port, return is logical port
          =1: input port is logical port, return is physical port */
int
bcm53570_b0_p2l(int unit, int port, int inverse)
{
    _cfg_map_t *pmap = &_port_cfgs[_pcfg_idx[unit]].p2l;
    int pp;

    /* Fixed mappings */
    /* logical=physical=mmu ports for CMIC port */
    if (port == CMIC_PORT) {
        return CMIC_PORT;
    }

    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        if (inverse) {
            for (pp = 0; pp < MAX_PHY_PORTS; pp++) {
                if (port == CDK_PORT_CONFIG_SYS_PORT(unit, pp)) {
                    if (_serdes_disabled[pp] == 1) {
                        return -1;
                    } else {
                        return pp;
                    }
                }
            }

            return -1;
        } else {
            if (_serdes_disabled[port] == 1) {
                return -1;
            } else {
                return CDK_PORT_CONFIG_SYS_PORT(unit, port);
            }
        }
    }

    if (inverse) {
        for (pp = 0; pp < pmap->size; pp++) {
            if (port == pmap->val[pp]) {
                if (_serdes_disabled[pp] == 1) {
                    return -1;
                } else {
                    return pp;
                }
            }
        }
        return -1;
    }
    if (port >= pmap->size) {
        return -1;
    }

    if (_serdes_disabled[port] == 1) {
        return -1;
    } else {
        return pmap->val[port];
    }
}

/* inverse=0: input port is physical port, return is mmu port
          =1: input port is mmu port, return is physical port */
int
bcm53570_b0_p2m(int unit, int port, int inverse)
{
    int pp;

    if (inverse) {
        if (port == CMIC_PORT) {
            return CMIC_PORT;
        } else {
            for (pp = 0; pp < MAX_PHY_PORTS; pp++) {
                if (port == _mmu_port_map[unit].map[pp]) {
                    return pp;
                }
            }
        }
        return -1;
    } else {
        if (port == CMIC_PORT) {
            return CMIC_PORT;
        } else {
            return _mmu_port_map[unit].map[port];
        }
    }
}

uint32_t
bcm53570_b0_port_speed_max(int unit, int port)
{
    _cfg_map_t *pmap = &_port_cfgs[_pcfg_idx[unit]].speed_max;

    if (_serdes_disabled[port] == 1) {
        return 0;
    }

    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        /* Double check if the port default config is valid */
        if (pmap->val[port] == 0) {
            return 0;
        }
        return CDK_PORT_CONFIG_SPEED_MAX(unit, port);
    }

    if (port <= 0 || port > pmap->size) {
        return 0;
    }

    if (pmap->val[port] < 0) {
        return 0;
    }

    return 100 * pmap->val[port];
}

static void
_mmu_ports_assign(int unit)
{
    cdk_pbmp_t pbmp;
    int phy_port;
    cdk_pbmp_t pbmp_add;
    int mmu_port_max, num_mmu_port = MAX_MMU_PORTS;

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp_add);
    CDK_PBMP_OR(pbmp, pbmp_add);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_CLPORT, &pbmp_add);
    CDK_PBMP_OR(pbmp, pbmp_add);

    /* Clear MMU port map */
    CDK_MEMSET(&_mmu_port_map[unit], -1, sizeof(_mmu_port_map[unit]));

    mmu_port_max = num_mmu_port - 1;
    for (phy_port = (MAX_PHY_PORTS - 1); phy_port > 1; phy_port--) {
        if (P2L(unit, phy_port) >= 0) {
            _mmu_port_map[unit].map[phy_port] =  mmu_port_max;
            mmu_port_max--;
        }
    }
}

/* input port is physical port */
int
bcm53570_b0_port_is_tscq(int port)
{
    if ((port >= MIN_TSCQ_PHY_PORT) && (port <= MAX_TSCQ_PHY_PORT)) {
        return 1;
    }
    return 0;
}

int
bcm53570_b0_port_is_xlport(int unit, int port)
{
    cdk_pbmp_t xlport_pbmp;

    bcm53570_b0_xlport_pbmp_get(unit, &xlport_pbmp);

    if (CDK_PBMP_MEMBER(xlport_pbmp, port)) {
        return 1;
    }
    return 0;
}

int
bcm53570_b0_port_is_clport(int unit, int port)
{
    cdk_pbmp_t clport_pbmp;

    bcm53570_b0_clport_pbmp_get(unit, &clport_pbmp);

    if (CDK_PBMP_MEMBER(clport_pbmp, port)) {
        return 1;
    }
    return 0;
}

int
bcm53570_b0_port_num_lanes(int unit, int port)
{

    int blkidx, subidx;
    int num = 0, lanes;
    cdk_pbmp_t gport_pbmp, clport_pbmp;

    bcm53570_b0_gport_pbmp_get(unit, &gport_pbmp);
    bcm53570_b0_clport_pbmp_get(unit, &clport_pbmp);

    if (CDK_PBMP_MEMBER(gport_pbmp, port)) {
        if (IS_TSCQ(port) && SPEED_MAX(unit, port) == 2500) {
            return 4;
        } else if (IS_TSCQ(port)) {
        }
        return 1;
    } else if (CDK_PBMP_MEMBER(clport_pbmp, port)) {
        blkidx = BLK_PORT(unit, port);

        for (subidx = 0; subidx < PORT_COUNT_PER_TSC; subidx++) {
            if (SPEED_MAX(unit, blkidx + subidx) > 0) {
                num++;
            }
        }
    } else {
        blkidx = BLK_PORT(unit, port);

        for (subidx = 0; subidx < PORT_COUNT_PER_TSC; subidx++) {
            if (SPEED_MAX(unit, blkidx + subidx) > 0) {
                num++;
            }
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

/* Get the primary port of the block */
int
bcm53570_b0_blk_port_get(int unit, int port)
{
    int val;
    cdk_pbmp_t clport_pbmp, tscq_pbmp, gport_pbmp;

    bcm53570_b0_clport_pbmp_get(unit, &clport_pbmp);
    bcm53570_b0_tscq_pbmp_get(unit, &tscq_pbmp);
    bcm53570_b0_gport_pbmp_get(unit, &gport_pbmp);

    if (CDK_PBMP_MEMBER(clport_pbmp, port)) {
        return MIN_TSCF_PHY_PORT;
    } else if (IS_XL(unit, port)) {
        val = (port - MIN_TSCX_PHY_PORT) / 4;
        return (val * 4) + MIN_TSCX_PHY_PORT;
    } else if (CDK_PBMP_MEMBER(tscq_pbmp, port)) {
        val = (port - MIN_TSCQ_PHY_PORT) / 16;
        return (val * 16) + MIN_TSCQ_PHY_PORT;
    } else if (CDK_PBMP_MEMBER(gport_pbmp, port)) {
        val = (port - 2) / 4;
        return (val * 4) + 2;
    }

    return 0;
}

int
bcm53570_b0_block_index_get(int unit, int blk_type, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, blk_type)) {
        return -1 ;
    }

    return pblk.bindex;
}

int
bcm53570_b0_subport_get(int unit, int port)
{
    return port - BLK_PORT(unit, port);
}

int
bcm53570_b0_port_mode_get(int unit, int port)
{
    int lanes;
    uint32_t pmode;

    lanes = bcm53570_b0_port_num_lanes(unit, port);
    pmode = XPORT_MODE_QUAD;
    if (lanes == 2) {
        pmode = XPORT_MODE_DUAL;
    } else if (lanes == 4) {
        pmode = XPORT_MODE_SINGLE;
    }

    return pmode;
}

extern int
bcm53570_b0_freq_set(int unit)
{
    return freq;
}


int
bcm53570_b0_bmd_attach(int unit)
{
    int port;
    cdk_pbmp_t pbmp, pbmp_all;
    int pidx = -1;
    uint32_t main_opt;
    uint32_t chip_cfg;
    int cfg_id, idx;
    int speed_max;
    struct _port_cfg_xlate_s *pcx;

    if (!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    /* Get port configurations */
    chip_cfg = CDK_CHIP_CONFIG(unit) & PORT_CONFIG_MASK;
    switch (chip_cfg) {
        case DCFG_OP1:
            main_opt = 0x10;
            pidx = 1;
            freq = 583;
            break;
        case DCFG_OP10b:
            main_opt = 0x102;
            pidx = 10;
            freq = 450;
            break;
        case DCFG_OP11b:
            main_opt = 0x112;
            pidx = 11;
            freq = 450;
            break;
        case DCFG_OP12:
            main_opt = 0x120;
        case DCFG_OP12b:
            main_opt = 0x122;
            pidx = 12;
            freq = 392;
            break;
        case DCFG_OP13:
            main_opt = 0x130;
            pidx = 13;
            freq = 125;
            break;
        case DCFG_OP2:
            main_opt = 0x20;
            pidx = 2;
            freq = 583;
            break;
        case DCFG_OP3a:
            main_opt = 0x31;
            pidx = 3;
            freq = 583;
            break;
        case DCFG_OP4a:
            main_opt = 0x41;
            pidx = 4;
            freq = 583;
            break;
        case DCFG_OP6:
            main_opt = 0x60;
            pidx = 6;
            freq = 392;
            break;
        case DCFG_OP7:
            main_opt = 0x70;
        case DCFG_OP7b:
            main_opt = 0x72;
            pidx = 7;
            freq = 500;
            break;
        case DCFG_OP8b:
            main_opt = 0x82;
            pidx = 8;
            freq = 500;
            break;
        case DCFG_OP9b:
            main_opt = 0x92;
            pidx = 9;
            freq = 500;
            break;
        default:
            /* Option 5 is full set, set as default */
            main_opt = 0x50;
            pidx = 5;
            freq = 500;
            break;
    }

    if (pidx >= COUNTOF(_port_cfg_xlate)) {
        return CDK_E_CONFIG;
    }

    pcx = &_port_cfg_xlate[pidx];

    /* Using port configuration as list_id */
    /* For cfg 1:b, main_opt = 0x12
       For cfg 2:1, main_opt = 0x21, etc .. */
    cfg_id = -1;
    for (idx = 0; idx < pcx->count; idx++) {
        if ((pcx->cfg_opt[idx]) == main_opt) {
            cfg_id = pcx->cfg_id[idx];
            break;
        }
    }

    if (cfg_id == -1) {
        CDK_WARN(("bcm53570_b0_bmd_attach[%d]: Invalid port config specified (%x). "
                  "Using default port config instead.\n",
                  unit, chip_cfg));
        cfg_id = pcx->cfg_id[0];
    }

    _pcfg_idx[unit] = -1;
    for (idx = 0; idx < COUNTOF(_port_cfgs); idx++) {
        if (cfg_id == (int)_port_cfgs[idx].id) {
            _pcfg_idx[unit] = idx;
            break;
        }
    }

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp_all);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_OR(pbmp_all, pbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_CLPORT, &pbmp);
    CDK_PBMP_OR(pbmp_all, pbmp);
    CDK_PBMP_ITER(pbmp_all, port) {
        BMD_PORT_PROPERTIES(unit, port) = 0;

        speed_max = bcm53570_b0_port_speed_max(unit, port);

        if (speed_max <= 0) {
            continue;
        }
        if (speed_max > 2500) {
            if (speed_max == 11000 || speed_max == 13000 ||
                speed_max == 21000 || speed_max == 42000) {
                BMD_PORT_PROPERTIES(unit, port) |= BMD_PORT_HG;
            } else {
                BMD_PORT_PROPERTIES(unit, port) |= BMD_PORT_XE;
            }
        } else {
            BMD_PORT_PROPERTIES(unit, port) |= BMD_PORT_GE;
        }

#if BMD_CONFIG_INCLUDE_PHY == 1
        bmd_phy_bus_set(unit, port, bcm53570_phy_bus);
#endif /* BMD_CONFIG_INCLUDE_PHY */
    }
    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    /* Initialize MMU port mappings */
    _mmu_ports_assign(unit);

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    _init_port_map(unit);
#endif /* CDK_CONFIG_INCLUDE_PORT_MAP */

    /* Initialize debug functions */
    BMD_PORT_SPEED_MAX(unit) = bcm53570_b0_port_speed_max;
    BMD_PORT_P2L(unit) = bcm53570_b0_p2l;
    BMD_PORT_P2M(unit) = bcm53570_b0_p2m;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return CDK_E_NONE;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53570_B0 */
