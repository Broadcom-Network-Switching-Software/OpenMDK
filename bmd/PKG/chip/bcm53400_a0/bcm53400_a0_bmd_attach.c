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
#include <bmd/bmd_device.h>
#include <bmd/bmd_phy_ctrl.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>
#include <cdk/chip/bcm53400_a0_defs.h>
#include <cdk/cdk_util.h>

#include "bcm53400_a0_bmd.h"
#include "bcm53400_a0_internal.h"

#define PORT_CONFIG_MASK        0xFF

#if BMD_CONFIG_INCLUDE_PHY == 1
#include <cdk/arch/xgsd_miim.h>
#include <phy/phy_buslist.h>

static phy_bus_t *bcm53400_phy_bus[] = {
#ifdef PHY_BUS_BCM53400_MIIM_INT_INSTALLED
    &phy_bus_bcm53400_miim_int,
#endif
#ifdef PHY_BUS_BCM953400K_MIIM_EXT_INSTALLED
    &phy_bus_bcm953400k_miim_ext,
#endif
    NULL
};

#define PHY_BUS_SET(_u,_p,_b) bmd_phy_bus_set(_u,_p,_b)

#else

#define PHY_BUS_SET(_u,_p,_b)

#endif /* BMD_CONFIG_INCLUDE_PHY */

/* Port configurations */
/* QSGMII SKU:
 * 1. 4xQSGMII + 8x1G + 4x10G, 
 * 2. 4xQSGMII + 8x1G + 2x10G + 2xHiGigDuo[13]
 * 3. 2xQSGMII + 16x1G + 4x10G
 * 4. 4xQSGMII + 4x10G (Low port count)
 * 5. 2xQSGMII + 16x5G + 4x10G
 * 6. 2xQSGMII + 8x5G + 2xXAUI + 4x10G
 * 7. 2xQSGMII + 16x2.5G + 4x10G
 * 7. 2xQSGMII + 16x1G + 4x10G
 */
static const int p2l_mapping_cascade[] = {
     0, -1,  2,  3,  4,  5,  6,  7, 
     8,  9, 10, 11, 12, 13, 14, 15, 
    16, 17, 18, 19, 20, 21, 22, 23, 
    24, 25, 26, -1, 27, -1, 28, -1, 
    29, -1, -1, -1, -1, -1
};

static const int p2l_mapping_cascade_1[] = {
     0, -1,  2,  3,  4,  5,  6,  7, 
     8,  9, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 11, 12, 13, 14, 15, 
    16, 17, 18, 19, 20, 21, 22, 23, 
    24, 25, 26, 27, 28, 29 
};

static const int p2l_mapping_lpc_cascade[] = {
     0, -1,  2,  3,  4,  5,  6,  7, 
     8,  9, 10, 11, 12, 13, 14, 15, 
    16, 17, -1, -1, -1, -1, -1, -1, 
    -1, -1, 18, -1, 19, -1, 20, -1, 
    21, -1, -1, -1, -1, -1
};

static const int p2l_mapping_cascade_16x5g[] = {
     0, -1,  2,  3,  4,  5,  6,  7, 
     8,  9, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 11, 12, 13, 14, 15, 
    16, 17, 18, 19, 20, 21, 22, 23, 
    24, 25, 26, 27, 28, 29 
};

static const int p2l_mapping_cascade_8x5g[] = {
     0, -1,  2,  3,  4,  5,  6,  7, 
     8,  9, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 11, 12, 13, 14, 15, 
    16, 17, 18, -1, -1, -1, 19, -1, 
    -1, -1, 20, 21, 22, 23
};

static const int port_speed_max_non_cascade[] = {
     -1,  -1,  10,  10,  10,  10,  10,  10,  
     10,  10,  10,  10,  10,  10,  10,  10,
     10,  10,  10,  10,  10,  10,  10,  10,  
     10,  10, 100,  -1, 100,  -1, 100,  -1, 
    100,  -1,  -1,  -1,  -1,  -1
};

static const int port_speed_max_cascade_1[] = {
    -1,  -1,  10,  10,  10,  10,  10,  10,  
    10,  10,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  10,  10,  10,  10,  10,  10,  
    10,  10, 100, 100, 100, 100,  10,  10,  
    10,  10,  10,  10,  10,  10
};

static const int port_speed_max_cascade[] = {
     -1,  -1,  10,  10,  10,  10,  10,  10,  
     10,  10,  10,  10,  10,  10,  10,  10,  
     10,  10,  10,  10,  10,  10,  10,  10,  
     10,  10, 100,  -1, 100,  -1, 130,  -1, 
    130,  -1,  -1,  -1,  -1,  -1
};

static const int port_speed_max_lpc_cascade[] = {
     -1,  -1,  10,  10,  10,  10,  10,  10,  
     10,  10,  10,  10,  10,  10,  10,  10,
     10,  10,  -1,  -1,  -1,  -1,  -1,  -1,  
     -1,  -1, 100,  -1, 100,  -1, 100,  -1, 
    100,  -1,  -1,  -1,  -1,  -1
};

static const int port_speed_max_cascade_16x5g[] = {
    -1,  -1,  10,  10,  10,  10,  10,  10,  
    10,  10,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  50,  50,  50,  50,  50,  50,  
    50,  50,  50,  50,  50,  50,  50,  50,  
    50,  50, 100, 100, 100, 100
};

static const int port_speed_max_cascade_8x5g[] = {
    -1,  -1,  10,  10,  10,  10,  10,  10,  
    10,  10,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  50,  50,  50,  50,  50,  50,  
    50,  50, 100,  -1,  -1,  -1,  100, -1,  
    -1,  -1, 100, 100, 100, 100
};

static const int port_speed_max_cascade_2p5g[] = {
    -1,  -1,  10,  10,  10,  10,  10,  10,  
    10,  10,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  25,  25,  25,  25,  25,  25,  
    25,  25, 100, 100, 100, 100,  25,  25,  
    25,  25,  25,  25,  25,  25
};

static const int port_speed_max_cascade_16x1g[] = {
      -1,  -1,  10,  10,  10,  10,  10,  10,  
      10,  10,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  10,  10,  10,  10,  10,  10,  
      10,  10, 100, 100, 100, 100,  10,  10,  
      10,  10,  10,  10,  10,  10
};

/* 1GE SKU:
 * 1. 8x1G + 2x10G, 
 * 2. 20x1G + 4x10G
 * 3. 16x1G + 4x2.5G
 */
static const int p2l_mapping_8x1g[] = {
     0, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, -1, 11, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1
};

static const int port_speed_max_8x1g[] = {
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  10,  10,  10,  10,  10,  10,  
      10,  10, 100,  -1, 100,  -1,  -1,  -1,  
      -1,  -1,  -1,  -1,  -1,  -1
};

static const int port_speed_max_20x1g[] = {
      -1,  -1,  10,  10,  10,  10,  -1,  -1,  
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  10,  10,  10,  10,  10,  10,  
      10,  10, 100, 100, 100, 100,  10,  10,  
      10,  10,  10,  10,  10,  10
};

static const int port_speed_max_56066[] = {
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  10,  10,  10,  10,  10,  10,
      10,  10,  25,  25,  25,  25,  10,  10,
      10,  10,  10,  10,  10,  10
};

/* 2.5GE SKU: 
 * 1. 24x2.5G
 * 2. 20x2.5G + 4x10G 
 */
static const int port_speed_max_2p5g[] = {
    -1,  -1,  25,  25,  25,  25,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
    -1,  -1,  25,  25,  25,  25,  25,  25,  
    25,  25,  25,  25,  25,  25,  25,  25,
    25,  25,  25,  25,  25,  25        
};

static const int port_speed_max_4x10g_2p5g[] = {
    -1,  -1,  25,  25,  25,  25,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
    -1,  -1,  25,  25,  25,  25,  25,  25,  
    25,  25, 100, 100, 100, 100,  25,  25,
    25,  25,  25,  25,  25,  25        
};

/* 10GE SKU: 
 * 1. 16x10G
 * 2. 2xXAUI + 14x10G
 */
static const int p2l_mapping_10g[] = {
     0, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1,  2,  3,
     4,  5,  6,  7,  8,  9, 10, 11, 
    12, 13, 14, 15, 16, 17
};

static const int p2l_mapping_mixed_10g[] = {
     0, -1,  2, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  3, -1, -1, -1,  4,  5,  
     6,  7,  8,  9, 10, 11, 12, 13, 
    14, 15, 16, 17, -1, -1
};

static const int port_speed_max_10g[] = {
    -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,   -1,  -1,  -1,  -1,  -1, 100, 100, 
    100, 100, 100, 100, 100, 100, 100, 100, 
    100, 100, 100, 100, 100, 100
};

static const int port_speed_max_10g_flex[] = {
    -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,   -1,  -1,  -1,  -1,  -1, 100,   0, 
     0,    0, 100,   0,   0,   0, 100,   0, 
     0,    0, 100,   0,   0,   0
};

static const int port_speed_max_mixed_10g[] = {
     -1,  -1, 100,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1, 100,  -1,  -1,  -1, 100, 100,       
    100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100,  -1,  -1
};        

/* Low port count 10GE SKU: 8x10G */
static const int p2l_mapping_lpc_10g[] = {
     0, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1,  2,  3,
     4,  5,  6,  7,  8,  9, -1, -1,
    -1, -1, -1, -1, -1, -1
};

static const int port_speed_max_lpc_10g[] = {
    -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,   -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,   -1,  -1,  -1,  -1,  -1, 100,   0, 
     0,    0, 100,   0,   0,   0,  -1,  -1,
    -1,   -1,  -1,  -1,  -1,  -1
};

/* Mixed SKUs:
 * 1. 12x2.5G + 12x10G
 * 2. 4xXAUI + 8x10G
 * 3. 4xXAUI + 4x10G
 */
static const int p2l_mapping_all[] = {
     0, -1,  2,  3,  4,  5, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  6,  7,  8,  9, 10, 11, 
    12, 13, 14, 15, 16, 17, 18, 19, 
    20, 21, 22, 23, 24, 25
};


static const int p2l_mapping_mixed_12x10g[] = {
     0, -1,  2, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  6, -1, -1, -1, 10, -1, 
    -1, -1, 14, -1, -1, -1, 18, 19, 
    20, 21, 22, 23, 24, 25
};

static const int p2l_mapping_mixed_8x10g[] = {
     0, -1,  2, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  6, -1, -1, -1, 10, -1, 
    -1, -1, 14, -1, -1, -1, 18, 19, 
    20, 21, -1, -1, -1, -1
};

/* 12x10G + 8x2.5G + 4x5G */   
static const int port_speed_max_mixed_embedded[] = {
    -1,  -1,  50,  50,  50,  50,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
    -1,  -1,  25,  25,  25,  25,  25,  25,
    25,  25, 100, 100, 100, 100, 100, 100,
   100, 100, 100, 100, 100, 100
};

static const int port_speed_max_mixed_h10g[] = {
     -1,  -1, 100,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1, 100,  -1,  -1,  -1, 100,  -1,
     -1,  -1, 100,  -1,  -1,  -1, 100, 100,
    100, 100, 100, 100, 100, 100 
};

static const int port_speed_max_mixed_l10g[] = {
     -1,  -1, 100,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1, 100,  -1,  -1,  -1, 100,  -1,
     -1,  -1, 100,  -1,  -1,  -1, 100, 100,
    100, 100,  -1,  -1,  -1,  -1 
};

static const int port_speed_max_mixed_op4[] = {
     -1,  -1,  10,  10,  10,  10,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  10,  10,  10,  10,  10, 100,
    100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100 
};

/* 13x1GE + 5x10G-KR + 1xXAUI*/
static const int p2l_mapping_hpc_mixed_io[] = {
     0, -1,  2, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  3,  4,  5,  6,  7,  8,  
     9, 10, 11, 12, 13, 14, 15, -1, 
    16, -1, 17, 18, 19, 20
};

static const int port_speed_max_hpc_mixed_io[] = {
     -1,  -1, 100,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  10,  10,  10,  10,  10,  10,       
     10,  10,  10,  10,  10,  10,  10,  -1,
    100,  -1, 100, 100, 100, 100 
};
static const int p2l_mapping_lpc_mixed_io[] = {
     0, -1,  2, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  3,  4,  5,  6,  7,  8,  
     9, 10, 11, 12, 13, -1, 14, -1, 
    -1, -1, -1, -1, -1, -1
};
static const int port_speed_max_lpc_mixed_io_op1[] = {
     -1,  -1, 100,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  10,  10,  10,  10,  10,  10,       
     10,  10, 100,  -1,  -1,  -1,  10,  -1,
     -1,  -1,  -1,  -1,  -1,  -1 
};
static const int port_speed_max_lpc_mixed_io_op2[] = {
     -1,  -1, 100,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  10,  10,  10,  10,  10,  10,
     10,  -1, 100, 100, 100,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1 
};


/* TDM configurations */
/* GE and low port count GE SKUs */
static int tdm_seq_cascade[96] = {
     2, 18, 26, 28, 30, 32,
     3, 19, 26, 28, 30, 32,
     4, 20, 26, 28, 30, 32,
     5, 21, 26, 28, 30, 32,
     6, 22, 26, 28, 30, 32,
     7, 23, 26, 28, 30, 32,
     8, 24, 26, 28, 30, 32,
     9, 25, 26, 28, 30, 32,
    10,  0, 26, 28, 30, 32,
    11, 63, 26, 28, 30, 32,
    12, 63, 26, 28, 30, 32,
    13, 63, 26, 28, 30, 32,
    14, 63, 26, 28, 30, 32,
    15, 63, 26, 28, 30, 32,
    16, 63, 26, 28, 30, 32,
    17, 63, 26, 28, 30, 32 
};            

static int tdm_seq_cascade_1[78] = {
    26, 27, 28, 29,  2, 18,
    26, 27, 28, 29,  3, 19,
    26, 27, 28, 29,  4, 20,
    26, 27, 28, 29,  5, 21,
    26, 27, 28, 29,  6, 22,
    26, 27, 28, 29,  7, 23,
    26, 27, 28, 29,  8, 24,
    26, 27, 28, 29,  9, 25,
    26, 27, 28, 29, 30, 34,
    26, 27, 28, 29, 31, 35,
    26, 27, 28, 29, 32, 36,
    26, 27, 28, 29, 33, 37,
    26, 27, 28, 29,  0, 63
};            

/* 1GE SKU */
static int tdm_seq_1g[78]={
    26, 27, 28, 29, 18,  2,
    26, 27, 28, 29, 19,  3,
    26, 27, 28, 29, 20,  4,
    26, 27, 28, 29, 21,  5,
    26, 27, 28, 29, 22,  6,
    26, 27, 28, 29, 23,  7,
    26, 27, 28, 29, 24,  8,
    26, 27, 28, 29, 25,  9,
    26, 27, 28, 29, 30, 34,
    26, 27, 28, 29, 31, 35,
    26, 27, 28, 29, 32, 36,
    26, 27, 28, 29, 33, 37,
    26, 27, 28, 29,  0, 63,
};

/* 2.5GE SKU */
static int tdm_seq_2p5g[28] = {
     2, 18, 22, 26, 30, 34,
     0,  3, 19, 23, 27, 31,
    35, 63,  4, 20, 24, 28,
    32, 36, 63,  5, 21, 25,
    29, 33, 37, 63
};

static int tdm_seq_10g_2p5g[48]={
    26, 27, 18, 22,  2,  0,
    28, 29, 30, 34,  3, 63,
    26, 27, 19, 23,  4, 63,
    28, 29, 31, 35,  5, 63,
    26, 27, 20, 24,  6, 63,
    28, 29, 32, 36,  7, 63,
    26, 27, 21, 25,  8, 63,
    28, 29, 33, 37,  9, 63,
};

/* 10GE SKU */
static int tdm_seq_10g[68] = {
    22, 23, 24, 25, 26, 27,
    28, 29, 30, 31, 32, 33,
    34, 35, 36, 37,  0, 22,
    23, 24, 25, 26, 27, 28,
    29, 30, 31, 32, 33, 34,
    35, 36, 37, 63, 22, 23,
    24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35,
    36, 37, 63, 22, 23, 24,
    25, 26, 27, 28, 29, 30,
    31, 32, 33, 34, 35, 36,
    37, 63
};    

static int tdm_seq_mixed_10g[68] = {
    22, 23, 24, 25, 26, 27,
    28, 29, 30, 31, 32, 33,
    34, 35,  2, 18,  0, 22,
    23, 24, 25, 26, 27, 28,
    29, 30, 31, 32, 33, 34,
    35,  2, 18, 63, 22, 23,
    24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35,
     2, 18, 63, 22, 23, 24,
    25, 26, 27, 28, 29, 30,
    31, 32, 33, 34, 35,  2,
    18, 63
};

/* Low port count 10GE SKU */
static int tdm_seq_lpc_10g[36] = {
    22, 23, 24, 25, 26, 27,
    28, 29,  0, 22, 23, 24,
    25, 26, 27, 28, 29, 63,
    22, 23, 24, 25, 26, 27,
    28, 29, 63, 22, 23, 24,
    25, 26, 27, 28, 29, 63
};

/* Mixed SKUs */
static int tdm_seq_mixed_flex[68] = {
    26, 27, 28, 29, 30, 31,
    32, 33,  2, 18, 22, 34,
    35, 36, 37,  0,  3, 26,
    27, 28, 29, 30, 31, 32,
    33,  4, 19, 23, 34, 35,
    36, 37, 63,  5, 26, 27,
    28, 29, 30, 31, 32, 33,
     2, 20, 24, 34, 35, 36,
    37, 63,  3, 26, 27, 28,
    29, 30, 31, 32, 33,  4,
    21, 25, 34, 35, 36, 37,
    63,  5
};

static int tdm_seq_mixed_xaui[68] = {
    26, 27, 28, 29, 30, 31,
    32, 33,  2, 18, 22, 34,
    35, 36, 37,  0,  4, 26,
    27, 28, 29, 30, 31, 32,
    33,  2, 18, 22, 34, 35,
    36, 37, 63,  4, 26, 27,
    28, 29, 30, 31, 32, 33,
     2, 18, 22, 34, 35, 36,
    37, 63,  4, 26, 27, 28,
    29, 30, 31, 32, 33,  2,
    18, 22, 34, 35, 36, 37,
    63,  4
};

static int tdm_seq_mixed_embeded_op4[101]={
        26,  27,  28,  29,  34,  35,
        36,  37,  30,  31,  32,  33,
        23,  24,  25,  26,  27,  28,
        29,  34,  35,  36,  37,  30,
        31,  32,  33,  23,  24,  25,
        26,  27,  28,  29,  34,  35,
        36,  37,  30,  31,  32,  33,
        23,  24,  25,  26,  27,  28,
        29,  34,  35,  36,  37,  30,
        31,  32,  33,  23,  24,  25,
        26,  27,  28,  29,  34,   2,
        35,  36,  37,  30,  31,   3,
        32,  33,  23,  24,  25,   4,
        26,  27,  28,  29,  34,   5,
        35,  36,  37,  30,  31,  18,
        32,  33,  23,  24,  25,  22,
        19,  20,  21,   0,  63
};

static int tdm_seq_cascade_16x5g[60] = {
    34, 35, 18, 22, 26,  2,
    36, 37, 19, 23, 27,  3,
    34, 35, 20, 24, 28,  4,
    36, 37, 21, 25, 29,  5,
     0, 63, 30, 31, 32, 33,
    34, 35, 18, 22, 26,  6,
    36, 37, 19, 23, 27,  7,
    34, 35, 20, 24, 28,  8,
    36, 37, 21, 25, 29,  9,
    63, 63, 30, 31, 32, 33,
};

static int tdm_seq_cascade_8x5g[58] = {
    34, 35, 36, 37, 30,  2,
    26, 18, 20, 22, 24,  3,
    34, 35, 36, 37, 30,  4,
    26, 18, 20, 22, 24,  5,
    19, 21, 23, 25,  0, 63,
    34, 35, 36, 37, 30,  6,
    26, 18, 20, 22, 24,  7,
    34, 35, 36, 37, 30,  8,
    26, 18, 20, 22, 24,  9,
    19, 21, 23, 25,
};
static int tdm_seq_hpc_mixed_io[80] = {
        18, 22,  2, 32, 34, 35,
        36, 37, 19, 23,  2, 32,
        34, 35, 36, 37, 20, 24,
         2, 32, 34, 35, 36, 37,
        21, 25,  2, 32, 34, 35,
        36, 37, 26, 63,  2, 32,
        34, 35, 36, 37, 27, 63,
         2, 32, 34, 35, 36, 37,
        28, 63,  2, 32, 34, 35,
        36, 37, 29, 63,  2, 32,
        34, 35, 36, 37, 30, 63,
         2, 32, 34, 35, 36, 37,
         0, 63,  2, 32, 34, 35,
        36, 37};
static int tdm_seq_lpc_mixed_io[36] = {
        18, 22,  2, 26, 27, 28,
        19, 23,  2, 26, 27, 28,
        20, 24,  2, 26, 27, 28,
        21, 25,  2, 26, 27, 28,
        30, 63,  2, 26, 27, 28,
         0, 63,  2, 26, 27, 28,
};

#define _SET_CFG_MAP(p, cfg) { p.size = COUNTOF(cfg); p.val = cfg; }
#define _SET_PORT_CFGS(p, s, t) \
    _SET_CFG_MAP(_port_cfg.p2l, p); \
    _SET_CFG_MAP(_port_cfg.speed_max, s); \
    _SET_CFG_MAP(_port_cfg.tdm, t); 
    
typedef struct _cfg_map_s {
    int size;
    const int *val;
} _cfg_map_t;

static struct _port_cfg_s {
    _cfg_map_t p2l;
    _cfg_map_t speed_max;
    _cfg_map_t tdm;
} _port_cfg;

static int 
port_mapping_override[NUM_PHYS_PORTS] = { 0 };

/***********************************************************************
 *
 * HELPER FUNCTIONS
 */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1

static struct _port_map_s {
    cdk_port_map_port_t map[BMD_CONFIG_MAX_PORTS];
} _port_map[BMD_CONFIG_MAX_UNITS];

static void
_init_port_map(int unit)
{
    cdk_pbmp_t pbmp;
    int port, pidx = 0;

    CDK_MEMSET(&_port_map[unit], -1, sizeof(_port_map[unit]));

    _port_map[unit].map[pidx] = CMIC_PORT;

    bcm53400_a0_xport_pbmp_get(unit, XPORT_FLAG_ALL, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        pidx++;
        _port_map[unit].map[pidx] = port;
    }
    CDK_PORT_MAP_SET(unit, _port_map[unit].map, pidx + 1);
}
#endif /* CDK_CONFIG_INCLUDE_PORT_MAP */

static int
_default_port_speed(int unit, int port)
{
    _cfg_map_t *pmap = &_port_cfg.speed_max;

    if (port >= pmap->size) {
        return -1;
    }
    if (pmap->val[port] < 0) {
        return 0;
    }
    return 100 * pmap->val[port];
}

static int
_default_p2l(int unit, int port)
{
    _cfg_map_t *pmap = &_port_cfg.p2l;

    if (port == CMIC_PORT) {
        return CMIC_LPORT;
    }
    if (port >= pmap->size) {
        return -1;
    }
    return pmap->val[port];
}

static int
_is_flex_port(int unit, int port)
{
    uint32_t speed;

    speed = _default_port_speed(unit, port);
    if ((speed == 0) && (_default_p2l(unit, port) >= 0)) {
        return 1;
    }

    speed = _default_port_speed(unit, port + 1);
    if ((speed == 0) && (_default_p2l(unit, port + 1) >= 0)) {
        return 1;
    }

    return 0;
}

/***********************************************************************
 *
 * BMD DRIVER FUNCTIONS
 */
int
bcm53400_a0_tsc_block_disable(int unit, uint32_t disable_tsc)
{
    int tidx, pidx;
    int start_port, end_port;

    for (tidx = 0; tidx < MAX_TSC_COUNT; tidx++) {
        if ((disable_tsc >> tidx) & 0x1) {
            if (tidx == 5) {
                start_port = 2;
                end_port = 17;
            } else {
                start_port = 18 + (tidx * 4);
                end_port = 18 + (tidx * 4) + 3;
            }

            for (pidx = start_port; pidx <= end_port; pidx++) {
                port_mapping_override[pidx] = 1;
                BMD_PORT_PROPERTIES(unit, pidx) = 0;
            }
        }
    }

    return 0;
}

int
bcm53400_a0_xlblock_number_get(int unit, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_XLPORT)) {
        return -1 ;
    } 
    
    return pblk.block;
}

int
bcm53400_a0_xlblock_subport_get(int unit, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_XLPORT)) {
        return -1 ;
    } 
    
    return pblk.bport;
}

int
bcm53400_a0_xport_pbmp_get(int unit, int flag, cdk_pbmp_t *pbmp)
{
    int port;
    cdk_pbmp_t gpbmp, xpbmp;

    CDK_PBMP_CLEAR(*pbmp) ;
    CDK_PBMP_CLEAR(gpbmp) ;
    CDK_PBMP_CLEAR(xpbmp) ;
    
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &xpbmp);
    if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_GE) {
        CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GXPORT, &gpbmp);
        CDK_PBMP_REMOVE(xpbmp, gpbmp);
    }
    
    if (flag & XPORT_FLAG_GXPORT) {
        CDK_PBMP_OR(*pbmp, gpbmp);
    }
    if (flag & XPORT_FLAG_XLPORT) {
        CDK_PBMP_OR(*pbmp, xpbmp);
    }
    
    CDK_PBMP_ITER(*pbmp, port) {
        if ((BMD_PORT_PROPERTIES(unit, port) == 0) || 
            (BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_FLEX)) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    
    return 0;
}

int
bcm53400_a0_p2l(int unit, int port, int inverse)
{
    _cfg_map_t *pmap = &_port_cfg.p2l;
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
                    return (port_mapping_override[pp]) ? -1 : pp;
                }
            }
            return -1;
        } else {
            return (port_mapping_override[port]) ? -1 : 
                    CDK_PORT_CONFIG_SYS_PORT(unit, port);
        }
    }

    if (inverse) {
        for (pp = 0; pp < pmap->size; pp++) {
            if (port == pmap->val[pp]) {
                return (port_mapping_override[pp]) ? -1 : pp;
            }
        }
        return -1;
    }
    if (port >= pmap->size) {
        return -1;
    }

    return (port_mapping_override[port]) ? -1 : pmap->val[port];
}

int
bcm53400_a0_p2m(int unit, int port, int inverse)
{
    /* MMU port map is same as logical port map */
    return bcm53400_a0_p2l(unit, port, inverse);
}

uint32_t
bcm53400_a0_port_speed_max(int unit, int port)
{
    if (port_mapping_override[port]) {
        return 0;
    }

    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        return CDK_PORT_CONFIG_SPEED_MAX(unit, port);
    }

    return _default_port_speed(unit, port);
}

int
bcm53400_a0_tdm_default(int unit, const int **tdm_seq)
{
    _cfg_map_t *tdm = &_port_cfg.tdm;

    *tdm_seq = tdm->val;
    
    return tdm->size;
}


int
bcm53400_a0_port_num_lanes(int unit, int port)
{
    uint32_t lanes = 0;
    uint32_t blkport;
    cdk_pbmp_t gpbmp;
    
    bcm53400_a0_xport_pbmp_get(unit, XPORT_FLAG_GXPORT, &gpbmp);
    if (CDK_PBMP_MEMBER(gpbmp, port)) {
        return 1;
    }
    
    blkport = port - XLPORT_SUBPORT(unit, port);
    lanes = 4;
    if (P2L(unit, blkport + 1) >= 0) {
        lanes = 1;
    } else if (P2L(unit, blkport + 2) >= 0) {
        lanes = 1;
        if ((BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_FLEX) ||
            (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG)) {
            lanes = 2;
        }
    }

    return lanes;
}

int 
bcm53400_a0_bmd_attach(int unit)
{
    int port;
    cdk_pbmp_t pbmp, pbmp_all;
    uint32_t speed_max;
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_bus_t **phy_bus = bcm53400_phy_bus;
#endif /* BMD_CONFIG_INCLUDE_PHY */

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    /* Get the port configurations */
    if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_GE) {
        if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_LPC) {
            _SET_PORT_CFGS(p2l_mapping_lpc_cascade, 
                           port_speed_max_lpc_cascade, 
                           tdm_seq_cascade);
        } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_X1G) {
            _SET_PORT_CFGS(p2l_mapping_cascade_1, 
                           port_speed_max_cascade_16x1g, 
                           tdm_seq_1g);
        } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_X2P5G) {
            _SET_PORT_CFGS(p2l_mapping_cascade_1, 
                           port_speed_max_cascade_2p5g, 
                           tdm_seq_10g_2p5g);
        } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_X5G) {
            if (CDK_CHIP_CONFIG(unit) & DCFG_XAUI) {
                _SET_PORT_CFGS(p2l_mapping_cascade_8x5g, 
                               port_speed_max_cascade_8x5g, 
                               tdm_seq_cascade_8x5g);
            } else {
                _SET_PORT_CFGS(p2l_mapping_cascade_16x5g, 
                               port_speed_max_cascade_16x5g, 
                               tdm_seq_cascade_16x5g);
            }
        } else {
            if (CDK_CHIP_CONFIG(unit) & DCFG_HIGIG) {
                _SET_PORT_CFGS(p2l_mapping_cascade, 
                               port_speed_max_cascade, 
                               tdm_seq_cascade);
            } else if (CDK_CHIP_CONFIG(unit) & DCFG_2QSGMII) {
                _SET_PORT_CFGS(p2l_mapping_cascade_1, 
                               port_speed_max_cascade_1, 
                               tdm_seq_cascade_1);
            } else {
                _SET_PORT_CFGS(p2l_mapping_cascade, 
                               port_speed_max_non_cascade, 
                               tdm_seq_cascade);
            }
        }
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_X1G) {
        if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_LPC) {
            _SET_PORT_CFGS(p2l_mapping_8x1g, 
                           port_speed_max_8x1g, 
                           tdm_seq_1g);
        } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_HPC) {
            _SET_PORT_CFGS(p2l_mapping_all, 
                           port_speed_max_20x1g, 
                           tdm_seq_1g);
        } else {
            _SET_PORT_CFGS(p2l_mapping_all, 
                           port_speed_max_56066, 
                           tdm_seq_1g);
        }
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_X2P5G) {
        if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_X10G) {
            _SET_PORT_CFGS(p2l_mapping_all, 
                           port_speed_max_4x10g_2p5g, 
                           tdm_seq_10g_2p5g);
        } else {
            _SET_PORT_CFGS(p2l_mapping_all, 
                           port_speed_max_2p5g, 
                           tdm_seq_2p5g);
        }
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_X10G) {
        if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_LPC) {
            _SET_PORT_CFGS(p2l_mapping_lpc_10g, 
                           port_speed_max_lpc_10g, 
                           tdm_seq_lpc_10g);
        } else {
            if (CDK_CHIP_CONFIG(unit) & DCFG_XAUI) {
                _SET_PORT_CFGS(p2l_mapping_mixed_10g, 
                               port_speed_max_mixed_10g, 
                               tdm_seq_mixed_10g);
            } else if (CDK_CHIP_CONFIG(unit) & DCFG_FLEX) {
                _SET_PORT_CFGS(p2l_mapping_10g, 
                               port_speed_max_10g_flex, 
                               tdm_seq_10g);
            } else {
                _SET_PORT_CFGS(p2l_mapping_10g, 
                               port_speed_max_10g, 
                               tdm_seq_10g);
            }
        }
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_MIXED) {
        if (CDK_CHIP_CONFIG(unit) & DCFG_TSC1X10G) {
            _SET_PORT_CFGS(p2l_mapping_all, 
                           port_speed_max_mixed_op4, 
                           tdm_seq_mixed_embeded_op4);
        } else if (CDK_CHIP_CONFIG(unit) & DCFG_H10G) {
            _SET_PORT_CFGS(p2l_mapping_mixed_12x10g, 
                           port_speed_max_mixed_h10g, 
                           tdm_seq_mixed_xaui);
        } else if (CDK_CHIP_CONFIG(unit) & DCFG_L10G) {
            _SET_PORT_CFGS(p2l_mapping_mixed_8x10g, 
                           port_speed_max_mixed_l10g, 
                           tdm_seq_mixed_xaui);
        } else {
            _SET_PORT_CFGS(p2l_mapping_all, 
                           port_speed_max_mixed_embedded, 
                           tdm_seq_mixed_flex);
        }
    } else if ((CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ176) 
            && (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ_CHG )) {
        if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_HPC) {
            _SET_PORT_CFGS(p2l_mapping_hpc_mixed_io, 
                           port_speed_max_hpc_mixed_io, 
                           tdm_seq_hpc_mixed_io);
        } else {
            if ((CDK_CHIP_CONFIG(unit) & PORT_CONFIG_MASK) == 0x2) {
                _SET_PORT_CFGS(p2l_mapping_lpc_mixed_io, 
                            port_speed_max_lpc_mixed_io_op2, 
                            tdm_seq_lpc_mixed_io);
            } else {
                _SET_PORT_CFGS(p2l_mapping_lpc_mixed_io, 
                            port_speed_max_lpc_mixed_io_op1, 
                            tdm_seq_lpc_mixed_io);
            }
        }
    } else {
        return CDK_E_CONFIG;
    }

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GXPORT, &pbmp_all);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_OR(pbmp_all, pbmp);
    CDK_PBMP_ITER(pbmp_all, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        if (_is_flex_port(unit, port)) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_FLEX;
        } else {
            BMD_PORT_PROPERTIES(unit, port) = 0;
        }
        
        speed_max = bcm53400_a0_port_speed_max(unit, port);
        if (speed_max == 0) {
            if (BMD_PORT_PROPERTIES(unit, port) == 0) {
                continue;
            }
        } else if (speed_max > 10000) {
            BMD_PORT_PROPERTIES(unit, port) |= BMD_PORT_HG;
        } else if (speed_max > 2500) {
            BMD_PORT_PROPERTIES(unit, port) |= BMD_PORT_XE;
        } else {
            BMD_PORT_PROPERTIES(unit, port) |= BMD_PORT_GE;
        }

#if BMD_CONFIG_INCLUDE_PHY == 1
        PHY_BUS_SET(unit, port, phy_bus);
#endif /* BMD_CONFIG_INCLUDE_PHY */
    }
    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    _init_port_map(unit);
#endif /* CDK_CONFIG_INCLUDE_PORT_MAP */

    /* Initialize debug functions */
    BMD_PORT_SPEED_MAX(unit) = bcm53400_a0_port_speed_max;
    BMD_PORT_P2L(unit) = bcm53400_a0_p2l;
    BMD_PORT_P2M(unit) = bcm53400_a0_p2m;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 
}

#endif /* CDK_CONFIG_INCLUDE_BCM53400_A0 */
