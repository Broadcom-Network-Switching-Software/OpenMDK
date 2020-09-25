/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

#include <board/board_config_maps.h>

extern board_config_t board_bcm56041_10g;
extern board_config_t board_bcm56045_10g;
extern board_config_t board_bcm56142_svk;
extern board_config_t board_bcm56150_10g;
extern board_config_t board_bcm56150_13g;
extern board_config_t board_bcm56218_skip_18;
extern board_config_t board_bcm56445_nohg;
extern board_config_t board_bcm56504_skip_13;
extern board_config_t board_bcm56649_skip_37;
extern board_config_t board_bcm56820_k24xg_r00;
extern board_config_t board_bcm56820_k24xg_r01;
extern board_config_t board_bcm56820_k24c;
extern board_config_t board_bcm56844_ext;
extern board_config_t board_bcm56845_svk;
extern board_config_t board_bcm56845_ext;
extern board_config_t board_bcm56845_ext2;
extern board_config_t board_bcm56846_svk;
extern board_config_t board_bcm56644_a0;
extern board_config_t board_bcm56854_10g;
extern board_config_t board_bcm56852_10g;
extern board_config_t board_bcm56851_10g;
extern board_config_t board_bcm56850_10g;
extern board_config_t board_bcm56850_40g;
extern board_config_t board_bcm56850_10g_over;
extern board_config_t board_bcm56850_40g_over;
extern board_config_t board_bcm56850_ext;
extern board_config_t board_bcm56450_svk;
extern board_config_t board_bcm56450_svk2;
extern board_config_t board_bcm53344_svk;
extern board_config_t board_bcm53393_svk;
extern board_config_t board_bcm53411_svk;
extern board_config_t board_bcm53415_10g;
extern board_config_t board_bcm53416_svk;
extern board_config_t board_bcm53456_svk;
extern board_config_t board_bcm56960_32x100g;
extern board_config_t board_bcm56960_32x40g;
extern board_config_t board_bcm56960_64x40g;
extern board_config_t board_bcm56960_64x20g;
extern board_config_t board_bcm56960_128x25g;
extern board_config_t board_bcm56960_128x10g;
extern board_config_t board_bcm56860_32x40g_K;
extern board_config_t board_bcm56860_96x10g_8x40g_K;
extern board_config_t board_bcm56860_4x100g_20x40g_KC;
extern board_config_t board_bcm56560_72x10g;
extern board_config_t board_bcm56560_72x10g_m2p1;
extern board_config_t board_bcm56560_56565;
extern board_config_t board_bcm56760_m3;
extern board_config_t board_bcm56766_m2p1;
extern board_config_t board_bcm56768_m8p8;
extern board_config_t board_bcm53570_op5;
extern board_config_t board_bcm56670_a10;

board_config_map_t board_config_map_sjlab[] = {
    /* Board configurations required for regression testing */
    { "rack01_12",      &board_bcm56445_nohg },
    { "rack16_08",      &board_bcm56218_skip_18 },
    { "rack25_13",      &board_bcm56820_k24c },
    { "rack21_06",      &board_bcm53416_svk },
    { "rack27_03",      &board_bcm56854_10g },
    { "rack31_06",      &board_bcm56142_svk },
    { "rack32_04",      &board_bcm56845_svk },
    { "rack32_15",      &board_bcm56845_ext2 },
    { "rack33_08",      &board_bcm56845_ext },
    { "rack37_10",      &board_bcm56649_skip_37 },
    { "rack37_13",      &board_bcm56504_skip_13 },
    { "rack40_08",      &board_bcm56846_svk },
    { "rack41_04",      &board_bcm56644_a0 },
    { "rack41_12",      &board_bcm56844_ext },
    { "rack45_10",      &board_bcm56850_ext },
    { "rack49_09",      &board_bcm56852_10g },
    { "rack50_05",      &board_bcm56851_10g },
    { "rack50_06",      &board_bcm56150_10g },
    { "rack53_09",      &board_bcm56450_svk },
    { "rack54_07",      &board_bcm56450_svk2 },
    { "rack57_07",      &board_bcm53344_svk },
    { "rack57_13",      &board_bcm53393_svk },
    { "rack71_01",      &board_bcm53416_svk },
    { "rack84_11",      &board_bcm53416_svk },    
    /* Test configurations for dev boards */
    { "953411svk",      &board_bcm53411_svk },
    { "gh10g",          &board_bcm53415_10g },
    { "hr213g",         &board_bcm56150_13g },
    { "r10g",           &board_bcm56041_10g },
    { "rp10g",          &board_bcm56045_10g },
    { "sc00",           &board_bcm56820_k24xg_r00 },
    { "sc01",           &board_bcm56820_k24xg_r01 },
    { "sc1g",           &board_bcm56820_k24c },
    { "td210g",         &board_bcm56850_10g },
    { "td240g",         &board_bcm56850_40g },
    { "td210o",         &board_bcm56850_10g_over },
    { "td240o",         &board_bcm56850_40g_over },
    { "th32x100g",      &board_bcm56960_32x100g },
    { "th32x40g",       &board_bcm56960_32x40g },
    { "th64x40g",       &board_bcm56960_64x40g },
    { "th64x20g",       &board_bcm56960_64x20g },
    { "th128x25g",      &board_bcm56960_128x25g },
    { "th128x10g",      &board_bcm56960_128x10g },
    { "td2p32x40g",     &board_bcm56860_32x40g_K },
    { "td2p96x10g_8x40g", &board_bcm56860_96x10g_8x40g_K },
    { "td2p4x100g_20x40g", &board_bcm56860_4x100g_20x40g_KC },
    { "ap72x10g",      &board_bcm56560_72x10g },
    { "ap72x10g_m2p1",      &board_bcm56560_72x10g_m2p1 },    
    { "ap_56565",      &board_bcm56560_56565 },
    { "ap_56760_m3",      &board_bcm56760_m3 },
    { "ap_56766_m2p1",      &board_bcm56766_m2p1 },
    { "ap_56768_m8p8",      &board_bcm56768_m8p8 },    
    { "gh2b_53570_op5",      &board_bcm53570_op5 },    
    { "mn_a10",      &board_bcm56670_a10 },
    /* Entries for disabling pre-assigned board configuration */
    { "none",           NULL },
    { "default",        NULL },
    { NULL, NULL }
};
