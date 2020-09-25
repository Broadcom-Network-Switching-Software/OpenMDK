/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53540_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <bmd/bmd_phy_ctrl.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm53540_a0_defs.h>

#include "bcm53540_a0_bmd.h"
#include "bcm53540_a0_internal.h"

#if BMD_CONFIG_INCLUDE_PHY == 1
#include <phy/phy_buslist.h>

static phy_bus_t *bcm53540_phy_bus[] = {
#ifdef PHY_BUS_BCM53540_MIIM_INT_INSTALLED
    &phy_bus_bcm53540_miim_int,
#endif
    NULL,
    NULL
};

#define PHY_BUS_SET(_u,_p,_b) bmd_phy_bus_set(_u,_p,_b)

#else

#define PHY_BUS_SET(_u,_p,_b)

#endif /* BMD_CONFIG_INCLUDE_PHY */

/*
* The dynamic configuration options for BCM53540 are defined as
* follows:
*
* Bit [7:0] : Select port configuration option
*
*
* Port configuration option:
*
* The port configuration options was used the following encoding:
*
*
* Option 1 :  0x1
* ...
* Option 10 : 0xa
* ...
* Option 18 : 0x12
*
*
* For example to select port configuration option 10:
*
* #include <cdk/chip/bcm53540_a0_defs.h>
*
* CDK_CHIP_CONFIG(unit) = 0xa;
 */

/* Option 0: default */
static int p2l_mapping_0[] = {
    0, -1,  2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, -1, -1,
   -1, -1, 14, 15, 16, 17, 18, 19,
   20, 21, 22, 23, 24, 25, -1, -1,
   -1, -1, 26, 27, 28, 29
};
static int port_speed_max_0[] = {
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};
static int tdm_seq_0[34] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22, 34,  7, 27, 23, 35,
     8, 28, 24, 36,  9, 29, 25, 37,
     0, 63
};
static int gp3_mode_0[3] = {
    0x3F, 0xF, 0
};

/* Option 1 */
static int p2l_mapping_1[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};
static int port_speed_max_1[] = {
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 25, 25, 25, 25, -1, -1,
    -1, -1, 25, 25, 25, 25
};
static int tdm_seq_1[40] = {
     2, 18, 26, 34, 10,  3, 27, 35,
    19, 11, 28, 36,  4, 20, 29, 37,
    12,  5, 26, 34, 21, 13, 27, 35,
     6, 22, 28, 36,  7, 23, 29, 37,
     8, 24,  0, 63,  9, 25, 63, 63
};
static int gp3_mode_1[3] = {
    0x1F, 0, 0xF
};

/* Option 2 */
static int p2l_mapping_2[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};
static int port_speed_max_2[] = {
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};
static int tdm_seq_2[34] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22, 34,  7, 27, 23, 35,
     8, 28, 24, 36,  9, 29, 25, 37,
     0, 63
};
static int gp3_mode_2[3] = {
    0x3F, 0xF, 0
};

/* Option 3 */
static int p2l_mapping_3[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};
static int port_speed_max_3[] = {
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};
static int tdm_seq_3[34] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22, 34,  7, 27, 23, 35,
     8, 28, 24, 36,  9, 29, 25, 37,
     0, 63
};
static int gp3_mode_3[3] = {
    0x3F, 0x3, 0xC
};

/* Option 4 */
static int p2l_mapping_4[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};
static int port_speed_max_4[] = {
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10
};
static int tdm_seq_4[32] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22,  0,  7, 27, 23, 63,
     8, 28, 24, 63,  9, 29, 25, 63
};
static int gp3_mode_4[3] = {
    0x3F, 0x3, 0xC
};

/* Option 5 */
static int p2l_mapping_5[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};
static int port_speed_max_5[] = {
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10
};
static int tdm_seq_5[32] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22,  0,  7, 27, 23, 63,
     8, 28, 24, 63,  9, 29, 25, 63
};
static int gp3_mode_5[3] = {
    0x1F, 0, 0xF
};

/* Option 6 */
static int p2l_mapping_6[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};
static int port_speed_max_6[] = {
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10
};
static int tdm_seq_6[32] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22,  0,  7, 27, 23, 63,
     8, 28, 24, 63,  9, 29, 25, 63
};
static int gp3_mode_6[3] = {
    0x3F, 0xF, 0
};

/* Option 7 */
static int p2l_mapping_7[] = {
     0, -1,  2,  3,  4,  5, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  6,  7,  8,  9, 10, 11,
    12, 13, 14, 15, 16, 17, -1, -1,
    -1, -1, 18, 19, 20, 21
};
static int port_speed_max_7[] = {
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 25, 25, 25, 25, -1, -1,
    -1, -1, 25, 25, 25, 25
};
static int tdm_seq_7[32] = {
     2, 18, 26, 34,  3, 19, 27, 35,
     4, 20, 28, 36,  5, 21, 29, 37,
     0, 22, 26, 34, 63, 23, 27, 35,
    63, 24, 28, 36, 63, 25, 29, 37
};
static int gp3_mode_7[3] = {
    0x19, 0, 0xF
};

/* Option 8 */
static int p2l_mapping_8[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};
static int port_speed_max_8[] = {
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};
static int tdm_seq_8[34] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22, 34,  7, 27, 23, 35,
     8, 28, 24, 36,  9, 29, 25, 37,
     0, 63
};
static int gp3_mode_8[3] = {
    0x39, 0xF, 0
};

/* Option 9 */
static int p2l_mapping_9[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};
static int port_speed_max_9[] = {
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};
static int tdm_seq_9[34] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22, 34,  7, 27, 23, 35,
     8, 28, 24, 36,  9, 29, 25, 37,
     0, 63
};
static int gp3_mode_9[3] = {
    0x39, 0x3, 0xC
};

/* Option 10 */
static int p2l_mapping_10[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};
static int port_speed_max_10[] = {
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10
};
static int tdm_seq_10[32] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22,  0,  7, 27, 23, 63,
     8, 28, 24, 63,  9, 29, 25, 63
};
static int gp3_mode_10[3] = {
    0x39, 0x3, 0xC
};

/* Option 11 */
static int p2l_mapping_11[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};
static int port_speed_max_11[] = {
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10
};
static int tdm_seq_11[32] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22,  0,  7, 27, 23, 63,
     8, 28, 24, 63,  9, 29, 25, 63
};
static int gp3_mode_11[3] = {
    0x19, 0, 0xF
};


/* Option 12 */
static int p2l_mapping_12[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};
static int port_speed_max_12[] = {
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10
};
static int tdm_seq_12[32] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22,  0,  7, 27, 23, 63,
     8, 28, 24, 63,  9, 29, 25, 63
};
static int gp3_mode_12[3] = {
    0x39, 0xF, 0
};

/* Option 13 */
static int p2l_mapping_13[] = {
     0, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  2,  3,  4,  5, -1, -1,
    -1, -1,  6,  7,  8,  9, -1, -1,
    -1, -1, 10, 11, 12, 13
};
static int port_speed_max_13[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25, -1, -1,
    -1, -1, 25, 25, 25, 25
};
static int tdm_seq_13[16] = {
    18, 26, 34,  0, 19, 27, 35, 63,
    20, 28, 36, 63, 21, 29, 37, 63
};
static int gp3_mode_13[3] = {
    0x8, 0, 0xF
};

/* Option 14 */
static int p2l_mapping_14[] = {
     0, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  2,  3,  4,  5, -1, -1,
    -1, -1,  6,  7,  8,  9, -1, -1,
    -1, -1, 10, 11, 12, 13
};
static int port_speed_max_14[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};
static int tdm_seq_14[16] = {
    18, 26, 34,  0, 19, 27, 35, 63,
    20, 28, 36, 63, 21, 29, 37, 63
};
static int gp3_mode_14[3] = {
    0x28, 0xF, 0
};

/* Option 15 */
static int p2l_mapping_15[] = {
     0, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  2,  3,  4,  5, -1, -1,
    -1, -1,  6,  7,  8,  9, -1, -1,
    -1, -1, 10, 11, 12, 13
};
static int port_speed_max_15[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};
static int tdm_seq_15[16] = {
    18, 26, 34,  0, 19, 27, 35, 63,
    20, 28, 36, 63, 21, 29, 37, 63
};
static int gp3_mode_15[3] = {
    0x28, 0x3, 0xC
};

/* Option 16 */
static int p2l_mapping_16[] = {
     0, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  2,  3,  4,  5, -1, -1,
    -1, -1,  6,  7,  8,  9, -1, -1,
    -1, -1, 10, 11, 12, 13
};
static int port_speed_max_16[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10
};
static int tdm_seq_16[16] = {
    18, 26, 34,  0, 19, 27, 35, 63,
    20, 28, 36, 63, 21, 29, 37, 63
};
static int gp3_mode_16[3] = {
    0x28, 0x3, 0xC
};

/* Option 17 */
static int p2l_mapping_17[] = {
     0, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  2,  3,  4,  5, -1, -1,
    -1, -1,  6,  7,  8,  9, -1, -1,
    -1, -1, 10, 11, 12, 13
};
static int port_speed_max_17[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10
};
static int tdm_seq_17[16] = {
    18, 26, 34,  0, 19, 27, 35, 63,
    20, 28, 36, 63, 21, 29, 37, 63
};
static int gp3_mode_17[3] = {
    0x8, 0, 0xF
};

/* Option 18 */
static int p2l_mapping_18[] = {
     0, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  2,  3,  4,  5, -1, -1,
    -1, -1,  6,  7,  8,  9, -1, -1,
    -1, -1, 10, 11, 12, 13
};
static int port_speed_max_18[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10
};
static int tdm_seq_18[16] = {
    18, 26, 34,  0, 19, 27, 35, 63,
    20, 28, 36, 63, 21, 29, 37, 63
};
static int gp3_mode_18[3] = {
    0x28, 0xF, 0
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
    _CFG_MAP(tdm_seq_##_id), \
    _CFG_MAP(gp3_mode_##_id) \
}

typedef struct _port_cfg_s {
    uint32_t id;
    _cfg_map_t p2l;
    _cfg_map_t speed_max;
    _cfg_map_t tdm;
    _cfg_map_t gp3_mode;
/*
    uint32_t   qgphy_core_map;
    uint32_t   qgphy5_lane;
    uint32_t   sgmii_4p0_lane;
*/
} _port_cfg_t;

static _port_cfg_t _port_cfgs[] = {
    _PORT_CFGS(0),
    _PORT_CFGS(1),
    _PORT_CFGS(2),
    _PORT_CFGS(3),
    _PORT_CFGS(4),
    _PORT_CFGS(5),
    _PORT_CFGS(6),
    _PORT_CFGS(7),
    _PORT_CFGS(8),
    _PORT_CFGS(9),
    _PORT_CFGS(10),
    _PORT_CFGS(11),
    _PORT_CFGS(12),
    _PORT_CFGS(13),
    _PORT_CFGS(14),
    _PORT_CFGS(15),
    _PORT_CFGS(16),
    _PORT_CFGS(17),
    _PORT_CFGS(18),
};


static int _pcfg_idx[BMD_CONFIG_MAX_UNITS];

static const int qgphy_port[QGPHY_MAX_BLK_COUNT] = {10, 26};

/*
 * Unused SerDes cores can be disabled through H/W pin strapping.
 * This table tracks which ports have been disabled this way.
 */
static int _serdes_disabled[NUM_PHYS_PORTS] = { 0 };

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static struct _port_map_s {
    cdk_port_map_port_t map[BMD_CONFIG_MAX_PORTS];
} _port_map[BMD_CONFIG_MAX_UNITS];


static void
_init_port_map(int unit)
{
    cdk_pbmp_t gpbmp;
    int port;
    int blk, idx, pidx = 0;

    CDK_MEMSET(&_port_map[unit], -1, sizeof(_port_map[unit]));

    _port_map[unit].map[0] = CMIC_PORT;

    bcm53540_a0_gport_pbmp_get(unit, &gpbmp);

    /* 20P_8_2.5 */
    for (idx = 2; idx < NUM_PHYS_PORTS; idx++) {
        blk = PMQ_BLKNUM(idx);
        if (blk == 0) {
            port = 13 + 2 - idx;
        } else {
            port = idx;
        }

        if (CDK_PBMP_MEMBER(gpbmp, port)) {
            pidx++;
            _port_map[unit].map[pidx] = port;
        }
    }

    CDK_PORT_MAP_SET(unit, _port_map[unit].map, pidx + 1);
}
#endif /* CDK_CONFIG_INCLUDE_PORT_MAP */

int
bcm53540_a0_gport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
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
bcm53540_a0_all_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, pbmp);
    CDK_PBMP_PORT_ADD(*pbmp, CMIC_PORT);

    return 0;
}

int
bcm53540_a0_block_index_get(int unit, int blk_type, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, blk_type)) {
        return -1 ;
    }
    return pblk.bindex;
}

int
bcm53540_a0_block_number_get(int unit, int blk_type, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, blk_type)) {
        return -1 ;
    }
    return pblk.block;
}

int
bcm53540_a0_block_subport_get(int unit, int blk_type, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, blk_type)) {
        return -1 ;
    }
    return pblk.bport;
}

int
bcm53540_a0_p2l(int unit, int port, int inverse)
{
    _cfg_map_t *pmap = &_port_cfgs[_pcfg_idx[unit]].p2l;
    int pp;

    /* Fixed mappings */
    if (port == CMIC_PORT) {
        return CMIC_LPORT;
    }

    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        if (inverse) {
            for (pp = 0; pp < NUM_PHYS_PORTS; pp++) {
                if (port == CDK_PORT_CONFIG_SYS_PORT(unit, pp)) {
                    return (_serdes_disabled[pp]) ? -1 : pp;
                }
            }
            return -1;
        } else {
            return (_serdes_disabled[port]) ? -1 :
                    CDK_PORT_CONFIG_SYS_PORT(unit, port);
        }
    }

    if (inverse) {
        for (pp = 0; pp < pmap->size; pp++) {
            if (port == pmap->val[pp]) {
                return (_serdes_disabled[pp]) ? -1 : pp;
            }
        }
        return -1;
    }
    if (port >= pmap->size) {
        return -1;
    }

    return (_serdes_disabled[port]) ? -1 : pmap->val[port];
}

int
bcm53540_a0_p2m(int unit, int port, int inverse)
{
    /* MMU port map is same as logical port map */
    return bcm53540_a0_p2l(unit, port, inverse);
}

int
bcm53540_a0_core_clock(int unit)
{
    int freq = 104;

    if (_pcfg_idx[unit] >= 16) {
        freq = 41;
    } else if ((_pcfg_idx[unit] >= 4)  && (_pcfg_idx[unit] <= 6)) {
        freq = 62;
    } else if ((_pcfg_idx[unit] >= 10)  && (_pcfg_idx[unit] <= 12)) {
        freq = 62;
    }

    return freq;
}

int
bcm53540_a0_tdm_default(int unit, const int **tdm_seq)
{
    _cfg_map_t *tdm = &_port_cfgs[_pcfg_idx[unit]].tdm;

    *tdm_seq = tdm->val;
    return tdm->size;
}

uint32_t
bcm53540_a0_port_speed_max(int unit, int port)
{
    _cfg_map_t *pmap = &_port_cfgs[_pcfg_idx[unit]].speed_max;

    if (_serdes_disabled[port]) {
        return 0;
    }

    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        return CDK_PORT_CONFIG_SPEED_MAX(unit, port);
    }
    if (port < 0 || port >= pmap->size) {
        return 0;
    }
    if (pmap->val[port] < 0) {
        return 0;
    }

    return (100 * pmap->val[port]);
}

uint32_t
bcm53540_a0_qgphy_core_map(int unit)
{
    _cfg_map_t *pmap = &_port_cfgs[_pcfg_idx[unit]].gp3_mode;

    return pmap->val[0];
}

uint32_t
bcm53540_a0_qgphy5_lane(int unit)
{
    _cfg_map_t *pmap = &_port_cfgs[_pcfg_idx[unit]].gp3_mode;

    return pmap->val[1];
}

uint32_t
bcm53540_a0_sgmii_4p0_lane(int unit)
{
    _cfg_map_t *pmap = &_port_cfgs[_pcfg_idx[unit]].gp3_mode;

    return pmap->val[2];
}

int
bcm53540_a0_gphy_get(int unit, int port)
{

    int is_gphy;
    int qgphy_id;
    uint32_t qgphy_core_map = QGPHY_CORE(unit);
    uint32_t qgphy5_lane = QGPHY5_LANE(unit);

    /* get the quad gphy id */
    /* 0:2-5, 1:6-9, 2:10-13, 3:18-21, 4:22-25, 5:26-29 */
    qgphy_id = (port - 2) / 4;
    if (port >= 34) {
        /* SGMII_4P1 => qgphy_id = -1 */
        qgphy_id = -1;
    } else if (port >= 18) {
        qgphy_id--;
    }

    if (qgphy_id != -1) {
        if (qgphy_core_map & (1 << qgphy_id)) {
            if (qgphy_id == 5) {
                if (qgphy5_lane & (1 << (port - 2) % 4)) {
                    is_gphy = 1;
                } else {
                    is_gphy = 0;
                }

            } else {
                is_gphy = 1;
            }
        } else {
            is_gphy = 0;
        }
    } else {
        is_gphy = 0;
    }
    return is_gphy;
}

int
bcm53540_a0_sku_option_get(int unit)
{
    int cfg_id;

    /*
     * Get the port configuration
     */
    cfg_id = CDK_CHIP_CONFIG(unit) & OPT_CONFIG_MASK;
    if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_CFG7) {
        if (cfg_id > 6) {
            CDK_WARN(("bcm53540_a0_bmd_attach[%d]: Invalid port config specified (%x). "
                      "Using default port config instead.\n",
                      unit, cfg_id));
        } 
        cfg_id = 1;
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_CFG8) {
        if ((cfg_id > 12) || (cfg_id < 7)) {
            CDK_WARN(("bcm53540_a0_bmd_attach[%d]: Invalid port config specified (%x). "
                      "Using default port config instead.\n",
                      unit, cfg_id));
        }
        cfg_id = 7;
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_CFG9) {
        if (cfg_id < 13) {
            CDK_WARN(("bcm53540_a0_bmd_attach[%d]: Invalid port config specified (%x). "
                    "Using default port config instead.\n",
                    unit, cfg_id));
        }
        cfg_id = 13;
    } else {
        cfg_id = 2;
    }

    return cfg_id;
}

int
bcm53540_a0_bmd_attach(int unit)
{
    int port;
    cdk_pbmp_t pbmp;
    uint32_t speed_max;

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    _pcfg_idx[unit] = bcm53540_a0_sku_option_get(unit);

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        BMD_PORT_PROPERTIES(unit, port) = 0;

        speed_max = bcm53540_a0_port_speed_max(unit, port);
        if (speed_max <= 0) {
            continue;
        }
        BMD_PORT_PROPERTIES(unit, port) |= BMD_PORT_GE;

#if BMD_CONFIG_INCLUDE_PHY == 1
        bmd_phy_bus_set(unit, port, bcm53540_phy_bus);
#endif /* BMD_CONFIG_INCLUDE_PHY */
    }

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    _init_port_map(unit);
#endif /* CDK_CONFIG_INCLUDE_PORT_MAP */

    /* Initialize debug functions */
    BMD_PORT_SPEED_MAX(unit) = bcm53540_a0_port_speed_max;
    BMD_PORT_P2L(unit) = bcm53540_a0_p2l;
    BMD_PORT_P2M(unit) = bcm53540_a0_p2m;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0;
}

#endif /* CDK_CONFIG_INCLUDE_BCM53540_A0 */
