#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56260_A0 == 1

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

#include <cdk/chip/bcm56260_a0_defs.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include "bcm56260_a0_bmd.h"
#include "bcm56260_a0_internal.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm56260_phy_bus[] = {
#ifdef PHY_BUS_BCM56260_MIIM_INT_INSTALLED
    &phy_bus_bcm56260_miim_int,
#endif
    NULL
};
#endif /* BMD_CONFIG_INCLUDE_PHY */

/* port speed arrays */
/* 56460_1 Cfg=1:IntCfg=1 */
/* 24xGE + 4xHGs11  4xGE 4xGE 4xGE + 4xGE 4xGE 4xGE 4xGE N4 2.5G(LPBK) */
static int port_speed_max_1[] = {
       1000, 1000, 1000, 1000,      /* 1  - 4  */
       1000, 1000, 1000, 1000,      /* 5  - 8  */
       1000, 1000, 1000, 1000,      /* 9  - 12 */
       1000, 1000, 1000, 1000,      /* 13 - 16 */
       1000, 1000, 1000, 1000,      /* 17 - 20 */
       1000, 1000, 1000, 1000,      /* 21 - 24 */
       10000, 10000, 10000, 10000   /* 25 - 28 */
};

/* 56460_2 Cfg=2:IntCfg=2 */
/* F.XAUI F.XAUI F.XAUI	F.XAUI F.XAUI F.XAUI 4xGE 2.5G(LPBK) */
static int port_speed_max_2[] = {
       10000, 0000, 0000, 0000,      /* 1  - 4  */
       10000, 0000, 0000, 0000,      /* 5  - 8  */
       10000, 0000, 0000, 0000,      /* 9  - 12 */
       10000, 0000, 0000, 0000,      /* 13 - 16 */
       10000, 0000, 0000, 0000,      /* 17 - 20 */
       10000, 0000, 0000, 0000,      /* 21 - 24 */
       1000,  1000, 1000, 1000       /* 25 - 28 */
};

/* 56460_3 Cfg=3:IntCfg=3 */
/* 2xF.XAUI + 12xGE + 1xF.XAUI + [2xXFI + 2xGE] + LPBK-2.5G */
static int port_speed_max_3[] = {
       10000, 0000, 0000, 0000,      /* 1  - 4  */
       10000, 0000, 0000, 0000,      /* 5  - 8  */
       1000,  1000, 1000, 1000,      /* 9  - 12 */
       1000,  1000, 1000, 1000,      /* 13 - 16 */
       1000,  1000, 1000, 1000,      /* 17 - 20 */
       10000, 0000, 0000, 0000,      /* 21 - 24 */
       10000, 1000,10000, 1000       /* 25 - 28 */
};

/* 56460_4 Cfg=4:IntCfg=4 */
/* 3x F.XAUI + 2x1G + .. 1xF.XAUI  + [2xXFI + 2x1G] + LPBK-2.5G */
static int port_speed_max_4[] = {
       10000, 0000, 0000, 0000,      /* 1  - 4  */
       10000, 0000, 0000, 0000,      /* 5  - 8  */
       10000, 0000, 0000, 0000,      /* 9  - 12 */
       1000,  0000, 1000, 0000,      /* 13 - 16 */
       0000,  0000, 0000, 0000,      /* 17 - 20 */
       10000, 0000, 0000, 0000,      /* 21 - 24 */
       10000, 1000,10000, 1000       /* 25 - 28 */
};

/* 56460_5 Cfg=5:IntCfg=5 */
/* F.XAUI F.XAUI F.XAUI	- 4xGE	F.XAUI	N2 */
static int port_speed_max_5[] = {
       10000, 0000, 0000, 0000,      /* 1  - 4  */
       10000, 0000, 0000, 0000,      /* 5  - 8  */
       10000, 0000, 0000, 0000,      /* 9  - 12 */
       0000,  0000, 0000, 0000,      /* 13 - 16 */
       1000,  1000, 1000, 1000,      /* 17 - 20 */
       10000, 0000, 0000, 0000,      /* 21 - 24 */
       10000, 0000,10000, 0000       /* 25 - 28 */
};

/* 56460_6 Cfg=6:IntCfg=6 */
/* F.XAUI	4xGE	-	-	4xGE	F.XAUI	N4s	12.5G */
static int port_speed_max_6[] = {
       10000, 0000, 0000, 0000,      /* 1  - 4  */
       1000,  1000, 1000, 1000,      /* 5  - 8  */
       0000,  0000, 0000, 0000,      /* 9  - 12 */
       0000,  0000, 0000, 0000,      /* 13 - 16 */
       1000,  1000, 1000, 1000,      /* 17 - 20 */
       10000, 0000, 0000, 0000,      /* 21 - 24 */
       10000, 1000,10000, 1000       /* 25 - 28 */
};

/* 56462 Cfg=1:IntCfg=7 */
/* F.XAUI	F.XAUI	F.XAUI	4xGE	4xGE	4xGE	F.XAUI	2.5G	B1A */
static int port_speed_max_7[] = {
       10000, 0000, 0000, 0000,      /* 1  - 4  */
       10000, 0000, 0000, 0000,      /* 5  - 8  */
       10000, 0000, 0000, 0000,      /* 9  - 12 */
       1000,  1000, 1000, 1000,      /* 13 - 16 */
       1000,  1000, 1000, 1000,      /* 17 - 20 */
       1000,  1000, 1000, 1000,      /* 21 - 24 */
       10000, 0000, 0000, 0000       /* 25 - 28 */
};

/* 56462 Cfg=2:IntCfg=8 */
/* F.XAUI	F.XAUI	-	4xGE	4xGE	4xGE	N4s	2.5G	B1A */
static int port_speed_max_8[] = {
       10000, 0000, 0000,  0000,      /* 1  - 4  */
       10000, 0000, 0000,  0000,      /* 5  - 8  */
       0000,  0000, 0000,  0000,      /* 9  - 12 */
       1000,  1000, 1000,  1000,      /* 13 - 16 */
       1000,  1000, 1000,  1000,      /* 17 - 20 */
       1000,  1000, 1000,  1000,      /* 21 - 24 */
       10000, 1000, 10000, 1000       /* 25 - 28 */
};

/* 56463 Cfg=1:IntCfg=9 */
/* 4xGE	4xGE	-	-	-	4xGE	N4s	2.5G */
static int port_speed_max_9[] = {
       1000,  1000, 1000,  1000,      /* 1  - 4  */
       1000,  1000, 1000,  1000,      /* 5  - 8  */
       0000,  0000, 0000,  0000,      /* 9  - 12 */
       0000,  0000, 0000,  0000,      /* 13 - 16 */
       0000,  0000, 0000,  0000,      /* 17 - 20 */
       1000,  1000, 1000,  1000,      /* 21 - 24 */
       10000, 1000, 10000, 1000       /* 25 - 28 */
};

/* 56463 Cfg=2:IntCfg=10 */
/* 4xGE	4xGE	4xGE	4xGE	4xGE	4xGE	4x2.5G	2.5G	E1 */
static int port_speed_max_10[] = {
       1000, 1000, 1000,  1000,      /* 1  - 4  */
       1000, 1000, 1000,  1000,      /* 5  - 8  */
       1000, 1000, 1000,  1000,      /* 9  - 12 */
       1000, 1000, 1000,  1000,      /* 13 - 16 */
       1000, 1000, 1000,  1000,      /* 17 - 20 */
       1000, 1000, 1000,  1000,      /* 21 - 24 */
       2500, 2500, 2500,  2500       /* 25 - 28 */
};

/* 56463 Cfg=3:IntCfg=11 */
/* 4xGE	F.XAUI	-	-	-	F.XAUI	F.XAUI	2.5G	E3 */
static int port_speed_max_11[] = {
       1000,  1000, 1000,  1000,      /* 1  - 4  */
       10000, 0000, 0000,  0000,      /* 5  - 8  */
       0000,  0000, 0000,  0000,      /* 9  - 12 */
       0000,  0000, 0000,  0000,      /* 13 - 16 */
       0000,  0000, 0000,  0000,      /* 17 - 20 */
       10000, 0000, 0000,  0000,      /* 21 - 24 */
       10000, 0000, 0000,  0000       /* 25 - 28 */
};

/* 56260 Cfg=1:IntCfg=12 */
/* 12x 2.5G + 2xXFI + 12.5G LPBK */
static int port_speed_max_12[] = {
       2500, 2500, 2500, 2500,      /* 1  - 4  */
       2500, 2500, 2500, 2500,      /* 5  - 8  */
       0,    0,    0,    0,         /* 9  - 12 */
       0,    0,    0,    0,         /* 13 - 16 */
       0,    0,    0,    0,         /* 17 - 20 */
       2500, 2500, 2500, 2500,      /* 21 - 24 */
       10000, 0, 10000,  0          /* 25 - 28 */
};

/* 56260 Cfg=2:IntCfg=13 */
/* F.XAUI	F.XAUI				F.XAUI	N4s	2.5G	B1 */
static int port_speed_max_13[] = {
       10000, 0000, 0000,  0000,      /* 1  - 4  */
       10000, 0000, 0000,  0000,      /* 5  - 8  */
       0000,  0000, 0000,  0000,      /* 9  - 12 */
       0000,  0000, 0000,  0000,      /* 13 - 16 */
       0000,  0000, 0000,  0000,      /* 17 - 20 */
       10000, 0000, 0000,  0000,      /* 21 - 24 */
       10000, 1000, 10000, 1000          /* 25 - 28 */
};

/* 56262 Cfg=1:IntCfg=14 */
/* 4x2.5G 4x2.5G - - - - - 2.5G	EA 75 MHz */
static int port_speed_max_14[] = {
       2500, 2500, 2500, 2500,      /* 1  - 4  */
       2500, 2500, 2500, 2500,      /* 5  - 8  */
       0000, 0000, 0000, 0000,      /* 9  - 12 */
       0000, 0000, 0000, 0000,      /* 13 - 16 */
       0000, 0000, 0000, 0000,      /* 17 - 20 */
       0000, 0000, 0000, 0000,      /* 21 - 24 */
       0000, 0000, 0000, 0000          /* 25 - 28 */
};

/* 56263 Cfg=1:IntCfg=15 */
/* 4xGE	4xGE - - - 4xGE 4xGE 2.5G A1A 37 MHz */
static int port_speed_max_15[] = {
       1000, 1000, 1000, 1000,      /* 1  - 4  */
       1000, 1000, 1000, 1000,      /* 5  - 8  */
       0000, 0000, 0000, 0000,      /* 9  - 12 */
       0000, 0000, 0000, 0000,      /* 13 - 16 */
       0000, 0000, 0000, 0000,      /* 17 - 20 */
       1000, 1000, 1000, 1000,      /* 21 - 24 */
       1000, 1000, 1000, 1000          /* 25 - 28 */
};

/* 56263 Cfg=2:IntCfg=16 */
/* 3xGE	3xGE	- - - -	- 2.5G	A1 24 MHz */
static int port_speed_max_16[] = {
       1000, 1000, 1000, 0000,      /* 1  - 4  */
       1000, 1000, 1000, 0000,      /* 5  - 8  */
       0000, 0000, 0000, 0000,      /* 9  - 12 */
       0000, 0000, 0000, 0000,      /* 13 - 16 */
       0000, 0000, 0000, 0000,      /* 17 - 20 */
       0000, 0000, 0000, 0000,      /* 21 - 24 */
       0000, 0000, 0000, 0000          /* 25 - 28 */
};

/* 56233: Cfg=3:IntCfg=17 */
/* 7xGE + 1x2.5GE	- - - -	- 2.5G	F 75 MHz */
static int port_speed_max_17[] = {
       1000, 1000, 1000, 2500,      /* 1  - 4  */
       2500, 0000, 0000, 0000,      /* 5  - 8  */
       0000, 0000, 0000, 0000,      /* 9  - 12 */
       0000, 0000, 0000, 0000,      /* 13 - 16 */
       0000, 0000, 0000, 0000,      /* 17 - 20 */
       1000, 1000, 1000, 1000,      /* 21 - 24 */
       0000, 0000, 0000, 0000       /* 25 - 28 */
};

/* 56460_1 Cfg=1:IntCfg=1 */
static int tdm_seq_1[] = {
    	/*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/ /*Col:5*/ /*Col:6*/ /*Col:7*/
/*1*/   30,        9,       25,       26,       27,       28,        7,
/*2*/    4,       10,       25,       26,       27,       28,        8,
/*3*/    1,       11,       25,       26,       27,       28,        0,
/*4*/   29,       12,       25,       26,       27,       28,       21,
/*5*/    2,       13,       25,       26,       27,       28,       22,
/*6*/    3,       14,       25,       26,       27,       28,       17,
/*7*/    5,       15,       25,       26,       27,       28,       18,
/*8*/   29,       16,       25,       26,       27,       28,        0,
/*9*/    6,       23,       25,       26,       27,       28,       19,
/*10*/   4,       24,       25,       26,       27,       28,       20,
/*11*/   3,        9,       25,       26,       27,       28,        7,
/*12*/  29,       10,       25,       26,       27,       28,        8,
/*13*/   1,       11,       25,       26,       27,       28,        0,
/*14*/   4,       12,       25,       26,       27,       28,       21,
/*15*/   2,       13,       25,       26,       27,       28,       22,
/*16*/  29,       14,       25,       26,       27,       28,       17,
/*17*/   5,       15,       25,       26,       27,       28,       18,
/*18*/   3,       16,       25,       26,       27,       28,        0,
/*19*/   6,       23,       25,       26,       27,       28,       19,
/*20*/  29,       24,       25,       26,       27,       28,       20,
/*21*/  30        /* ================================= EMPTY ============ */
};

/* 56460_2 Cfg=2:IntCfg=2 */
static int tdm_seq_2[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/ /*Col:5*/ /*Col:6*/ /*Col:7*/
/*1*/   30,       1,        5,        9,        13,       17,       21,	
/*2*/   29,       1,        5,        9,        13,       17,       21,	
/*3*/   25,       1,        5,        9,        13,       17,       21,	
/*4*/    0,       1,        5,        9,        13,       17,       21,	
/*5*/   26,       1,        5,        9,        13,       17,       21,	
/*6*/   29,       1,        5,        9,        13,       17,       21,	
/*7*/   27,       1,        5,        9,        13,       17,       21,	
/*8*/    0,       1,        5,        9,        13,       17,       21,	
/*9*/   28,       1,        5,        9,        13,       17,       21,	
/*10*/  29,       1,        5,        9,        13,       17,       21,	
/*11*/   0,       1,        5,        9,        13,       17,       21,	
/*12*/  29,       1,        5,        9,        13,       17,       21,	
/*13*/  25,       1,        5,        9,        13,       17,       21,	
/*14*/   0,       1,        5,        9,        13,       17,       21,	
/*15*/  26,       1,        5,        9,        13,       17,       21,	
/*16*/  29,       1,        5,        9,        13,       17,       21,	
/*17*/  27,       1,        5,        9,        13,       17,       21,	
/*18*/   0,       1,        5,        9,        13,       17,       21,	
/*19*/  28,       1,        5,        9,        13,       17,       21,	
/*20*/  29,       1,        5,        9,        13,       17,       21,	
/*21*/  30
};

/* 56460_3 Cfg=3:IntCfg=3 */
static int tdm_seq_3[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/ /*Col:5*/ /*Col:6*/ /*Col:7*/
/*1*/   30,       1,        5,         9,       21,       25,       27,
/*2*/   29,       1,        5,        10,       21,       25,       27,
/*3*/   17,       1,        5,        11,       21,       25,       27,
/*4*/    0,       1,        5,        12,       21,       25,       27,
/*5*/   18,       1,        5,        13,       21,       25,       27,
/*6*/   29,       1,        5,        14,       21,       25,       27,
/*7*/   19,       1,        5,        15,       21,       25,       27,
/*8*/    0,       1,        5,        16,       21,       25,       27,
/*9*/   20,       1,        5,        26,       21,       25,       27,
/*10*/  29,       1,        5,        28,       21,       25,       27,
/*11*/   0,       1,        5,         9,       21,       25,       27,
/*12*/  29,       1,        5,        10,       21,       25,       27,
/*13*/  17,       1,        5,        11,       21,       25,       27,
/*14*/   0,       1,        5,        12,       21,       25,       27,
/*15*/  18,       1,        5,        13,       21,       25,       27,
/*16*/  29,       1,        5,        14,       21,       25,       27,
/*17*/  19,       1,        5,        15,       21,       25,       27,
/*18*/   0,       1,        5,        16,       21,       25,       27,
/*19*/  20,       1,        5,        26,       21,       25,       27,
/*20*/  29,       1,        5,        28,       21,       25,       27,
/*21*/  30        /* -------------------- EMPTY ------------------------- */
};

/* 56460_4 Cfg=4:IntCfg=4 */
static int tdm_seq_4[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/ /*Col:5*/ /*Col:6*/ /*Col:7*/
/*1*/   30,	      1,        5,        9,        21,       25,       27,	
/*2*/   29,	      1,        5,        9,        21,       25,       27,	
/*3*/   13,	      1,        5,        9,        21,       25,       27,	
/*4*/   0,	      1,        5,        9,        21,       25,       27,	
/*5*/   15,	      1,        5,        9,        21,       25,       27,	
/*6*/   29,	      1,        5,        9,        21,       25,       27,	
/*7*/   26,	      1,        5,        9,        21,       25,       27,	
/*8*/   0,	      1,        5,        9,        21,       25,       27,	
/*9*/   28,	      1,        5,        9,        21,       25,       27,	
/*10*/  29,	      1,        5,        9,        21,       25,       27,	
/*11*/  0,	      1,        5,        9,        21,       25,       27,	
/*12*/  29,	      1,        5,        9,        21,       25,       27,	
/*13*/  13,	      1,        5,        9,        21,       25,       27,	
/*14*/  0,	      1,        5,        9,        21,       25,       27,	
/*15*/  15,	      1,        5,        9,        21,       25,       27,	
/*16*/  29,	      1,        5,        9,        21,       25,       27,	
/*17*/  26,	      1,        5,        9,        21,       25,       27,	
/*18*/  0,	      1,        5,        9,        21,       25,       27,	
/*19*/  28,	      1,        5,        9,        21,       25,       27,	
/*20*/  29,	      1,        5,        9,        21,       25,       27,	
/*21*/  30
};

/* 56460_5 Cfg=5:IntCfg=5 */
static int tdm_seq_5[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/ /*Col:5*/ /*Col:6*/ /*Col:7*/
/*1*/   30,	      1,		5,	      9,        21,       25,       27,	
/*2*/   29,	      1,		5,	      9,        21,       25,       27,	
/*3*/   17,	      1,		5,	      9,        21,       25,       27,	
/*4*/   0,	      1,		5,	      9,        21,       25,       27,	
/*5*/   18,	      1,		5,	      9,        21,       25,       27,	
/*6*/   29,	      1,		5,	      9,        21,       25,       27,	
/*7*/   19,	      1,		5,	      9,        21,       25,       27,	
/*8*/   0,	      1,		5,	      9,        21,       25,       27,	
/*9*/   20,	      1,		5,	      9,        21,       25,       27,	
/*10*/  29,	      1,		5,	      9,        21,       25,       27,	
/*11*/  0,	      1,		5,	      9,        21,       25,       27,	
/*12*/  29,	      1,		5,	      9,        21,       25,       27,	
/*13*/  17,	      1,		5,	      9,        21,       25,       27,	
/*14*/  0,	      1,		5,	      9,        21,       25,       27,	
/*15*/  18,	      1,		5,	      9,        21,       25,       27,	
/*16*/  29,	      1,		5,	      9,        21,       25,       27,	
/*17*/  19,	      1,		5,	      9,        21,       25,       27,	
/*18*/  0,	      1,		5,	      9,        21,       25,       27,	
/*19*/  20,	      1,		5,	      9,        21,       25,       27,	
/*20*/  29,	      1,		5,	      9,        21,       25,       27,	
/*21*/  30
};

/* 56460_6 Cfg=6:IntCfg=6 */
static int tdm_seq_6[] = {
        /*Col:1*/ /*Col:2*/	/*Col:3*/ /*Col:4*/ /*Col:5*/ /*Col:6*/
/*1*/   30,       27,     	29,	      25,	    21,	       1,
/*2*/   17,        5,     	29,	      27,	    25,	      21,
/*3*/    1,        0,     	29,	      20,	    27,	      25,
/*4*/   21,        1,     	29,	      28,	    27,	      25,
/*5*/   21,        1,     	29,	      18,	     7,	      27,
/*6*/   25,       21,     	29,	       1,	    19,	       6,
/*7*/   27,       25,     	29,	      21,	     1,	       8,
/*8*/   26,       27,     	29,	      25,	    21,	       1,
/*9*/    0,       31,     	29,	      27,	    25,	      21,
/*10*/   1,       31,     	29,	       0,	    27,	      25,
/*11*/  21,        1,     	29,	       5,	    27,	      25,
/*12*/  21,        1,     	29,	      20,	     0,	      27,
/*13*/  25,       21,     	29,	       1,	    17,	      28,
/*14*/  27,       25,     	29,	      21,	     1,	       7,
/*15*/  18,       27,     	29,	      25,	    21,	       1,
/*16*/  19,        6,     	29,	      27,	    25,	      21,
/*17*/   1,       26,     	29,	       8,	    27,	      25,
/*18*/  21,        1,     	29,	       0,	    27,	      25,
/*19*/  21,        1,     	29,	      31,	     0,	      27,
/*20*/  25,       21,     	29,	       1,	    31,	       5,
/*21*/  27,       25,     	29,	      21,	     1,	       0,
/*22*/  20,       27,     	29,	      25,	    21,	       1,
/*23*/  17,       28,     	29,	      27,	    25,	      21,
/*24*/   1,        7,     	29,	      18,	    27,	      25,
/*25*/  21,        1,     	29,	       6,	    27,	      25,
/*26*/  21,        1,     	29,	       0,	    19,	      27,
/*27*/  25,       21,     	29,	       1,	     8,	      26,
/*28*/  27,       25,     	29,	      21,	     1,	      30
};

/* 56462 Cfg=1:IntCfg=7 */
static int tdm_seq_7[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/ /*Col:5*/ /*Col:6*/
/*1*/   30,	      1,        5,        21,       25,       9,	
/*2*/   29,	      1,        5,        22,       25,       9,	
/*3*/   17,	      1,        5,        23,       25,       9,	
/*4*/    0,	      1,        5,        24,       25,       9,	
/*5*/   18,	      1,        5,        13,       25,       9,	
/*6*/   29,	      1,        5,        14,       25,       9,	
/*7*/   19,	      1,        5,        15,       25,       9,	
/*8*/    0,	      1,        5,        16,       25,       9,	
/*9*/   20,	      1,        5,        31,       25,       9,	
/*10*/  29,	      1,        5,        31,       25,       9,	
/*11*/   0,	      1,        5,        21,       25,       9,	
/*12*/  29,	      1,        5,        22,       25,       9,	
/*13*/  17,	      1,        5,        23,       25,       9,	
/*14*/   0,	      1,        5,        24,       25,       9,	
/*15*/  18,	      1,        5,        13,       25,       9,	
/*16*/  29,	      1,        5,        14,       25,       9,	
/*17*/  19,	      1,        5,        15,       25,       9,	
/*18*/   0,	      1,        5,        16,       25,       9,	
/*19*/  20,	      1,        5,        31,       25,       9,	
/*20*/  29,	      1,        5,        31,       25,       9,	
/*21*/  30
};

/* 56462 Cfg=2:IntCfg=8 */
static int tdm_seq_8[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/ /*Col:5*/ /*Col:6*/
/*1*/   30,	      1,        5,        21,       25,       27,	
/*2*/   29,	      1,        5,        22,       25,       27,	
/*3*/   17,	      1,        5,        23,       25,       27,	
/*4*/    0,	      1,        5,        24,       25,       27,	
/*5*/   18,	      1,        5,        13,       25,       27,	
/*6*/   29,	      1,        5,        14,       25,       27,	
/*7*/   19,	      1,        5,        15,       25,       27,	
/*8*/    0,	      1,        5,        16,       25,       27,	
/*9*/   20,	      1,        5,        26,       25,       27,	
/*10*/  29,	      1,        5,        28,       25,       27,	
/*11*/   0,	      1,        5,        21,       25,       27,	
/*12*/  29,	      1,        5,        22,       25,       27,	
/*13*/  17,	      1,        5,        23,       25,       27,	
/*14*/   0,	      1,        5,        24,       25,       27,	
/*15*/  18,	      1,        5,        13,       25,       27,	
/*16*/  29,	      1,        5,        14,       25,       27,	
/*17*/  19,	      1,        5,        15,       25,       27,	
/*18*/   0,	      1,        5,        16,       25,       27,	
/*19*/  20,	      1,        5,        26,       25,       27,	
/*20*/  29,	      1,        5,        28,       25,       27,	
/*21*/  30
};

/* 56463 Cfg=1:IntCfg=9 */
static int tdm_seq_9[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/
/*1*/   30,	       1,       25,       27,	
/*2*/   29,	       2,       25,       27,	
/*3*/   21,	       3,       25,       27,	
/*4*/    0,	       4,       25,       27,	
/*5*/   22,	       5,       25,       27,	
/*6*/   29,	       6,       25,       27,	
/*7*/   23,	       7,       25,       27,	
/*8*/    0,	       8,       25,       27,	
/*9*/   24,	      26,       25,       27,	
/*10*/  29,	      28,       25,       27,	
/*11*/   0,	       1,       25,       27,	
/*12*/  29,	       2,       25,       27,	
/*13*/  21,	       3,       25,       27,	
/*14*/   0,	       4,       25,       27,	
/*15*/  22,	       5,       25,       27,	
/*16*/  29,	       6,       25,       27,	
/*17*/  23,	       7,       25,       27,	
/*18*/   0,	       8,       25,       27,	
/*19*/  24,	      26,       25,       27,	
/*20*/  29,	      28,       25,       27,	
/*21*/  30
};

/* 56463 Cfg=2:IntCfg=10 */
static int tdm_seq_10[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/
/*1*/   30,	       1,       11,       25,	
/*2*/   29,	       2,       12,       26,	
/*3*/   21,	       3,       13,       27,	
/*4*/    0,	       4,       14,       28,	
/*5*/   22,	       5,       15,       25,	
/*6*/   29,	       6,       16,       26,	
/*7*/   23,	       7,       17,       27,	
/*8*/    0,	       8,       18,       28,	
/*9*/   24,	       9,       19,       25,	
/*10*/  29,	      10,       20,       26,	
/*11*/   0,	       1,       11,       27,	
/*12*/  29,	       2,       12,       28,	
/*13*/  21,	       3,       13,       25,	
/*14*/   0,	       4,       14,       26,	
/*15*/  22,	       5,       15,       27,	
/*16*/  29,	       6,       16,       28,	
/*17*/  23,	       7,       17,       25,	
/*18*/   0,	       8,       18,       26,	
/*19*/  24,	       9,       19,       27,	
/*20*/  29,	      10,       20,       28,	
/*21*/  30
};

/* 56463 Cfg=3:IntCfg=11 */
static int tdm_seq_11[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/
/*1*/   30,       21,       5,        25,	
/*2*/   29,       21,       5,        25,	
/*3*/    1,       21,       5,        25,	
/*4*/    0,       21,       5,        25,	
/*5*/    2,       21,       5,        25,	
/*6*/   29,       21,       5,        25,	
/*7*/    3,       21,       5,        25,	
/*8*/    0,       21,       5,        25,	
/*9*/    4,       21,       5,        25,	
/*10*/  29,       21,       5,        25,	
/*11*/   0,       21,       5,        25,	
/*12*/  29,       21,       5,        25,	
/*13*/   1,       21,       5,        25,	
/*14*/   0,       21,       5,        25,	
/*15*/   2,       21,       5,        25,	
/*16*/  29,       21,       5,        25,	
/*17*/   3,       21,       5,        25,	
/*18*/   0,       21,       5,        25,	
/*19*/   4,       21,       5,        25,	
/*20*/  29,       21,       5,        25,	
/*21*/  30
};

/* 56260 Cfg=1:IntCfg=12 */
static int tdm_seq_12[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/ /*Col:5*/ /*Col:6*/
/*1*/   29,        1,       27,	       5,       25,	      21,
/*2*/   29,        2,        6,	      27,       29,	      25,
/*3*/   22,        3,       29,	      27,        7,	      25,
/*4*/   23,        4,       29,	      27,        8,	      25,
/*5*/   24,        0,        1,	      29,       27,        5,
/*6*/   25,       29,       21,	       2,       27,	      29,
/*7*/   25,        6,       22,	      29,       27,	      3,
/*8*/   25,        7,       23,	      29,       27,	      4,
/*9*/   25,        8,       24,	      29,        0,	      27,
/*10*/   1,       25,       29,        5,       21,	      27,
/*11*/  29,       25,        2,	       6,       29,	      27,
/*12*/  22,       25,       29,	       3,        7,	      27,
/*13*/  23,       25,        4,	      29,        8,	      24,
/*14*/  27,        0,       25,	      29,        1,	       5,
/*15*/  27,       29,       25,	      21,        2,	       6,
/*16*/  27,       29,       25,	      22,        3,	       7,
/*17*/  27,       29,       25,	      23,        4,	       8,
/*18*/  29,       27,       24,	      25,       30,	      30,
/*19*/  29,       27,        1,	      25,        5,	      29,
/*20*/  21,       27,        2,	      25,        6,	      29,
/*21*/  22,       27,        3,	      25,        7,	      23,
/*22*/  29,        4,       27,	       8,       25,	      24,
/*23*/   0        /* ------------------------- EMPTY ------------ */
};

/* 56260 Cfg=2:IntCfg=13 */
static int tdm_seq_13[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/ /*Col:5*/ /*Col:6*/
/*1*/   30,	       1,        5,       25,       27,       21,
/*2*/   29,	       1,        5,       25,       27,       21,
/*3*/    0,	       1,        5,       25,       27,       21,
/*4*/   28,	       1,        5,       25,       27,       21,
/*5*/   26,	       1,        5,       25,       27,       21,
/*6*/   29,	       1,        5,       25,       27,       21,
/*7*/   31,	       1,        5,       25,       27,       21,
/*8*/    0,	       1,        5,       25,       27,       21,
/*9*/   31,	       1,        5,       25,       27,       21,
/*10*/  29,	       1,        5,       25,       27,       21,
/*11*/  28,	       1,        5,       25,       27,       21,
/*12*/  31,	       1,        5,       25,       27,       21,
/*13*/   0,	       1,        5,       25,       27,       21,
/*14*/  29,	       1,        5,       25,       27,       21,
/*15*/  26,	       1,        5,       25,       27,       21,
/*16*/   0,	       1,        5,       25,       27,       21,
/*17*/   1,	       5,       25,       27,       21,       29,
/*18*/   1,	       5,       25,       27,       21,        0,
/*19*/   1,	       5,       25,       27,       21,       28,
/*20*/   1,	       5,       25,       27,       21,       30
};

/* 56262 Cfg=1:IntCfg=14 */
static int tdm_seq_14[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/
/*1*/   30,        1,        5,       29,
/*2*/   31,        2,        6,       31,
/*3*/   31,        3,        7,       31,
/*4*/    0,        4,        8,       30
};

/* 56263 Cfg=1:IntCfg=15 */
static int tdm_seq_15[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/   
/*1*/    1,        5,       21,       29,
/*2*/   25,        2,        6,       0,
/*3*/   22,       26,        3,       29,
/*4*/    7,       23,       27,       0,
/*5*/    4,        8,       24,       29,
/*6*/   28,        1,        5,       0,
/*7*/   21,       25,        2,       29,
/*8*/    6,       22,       26,       0,
/*9*/    3,        7,       23,       29,
/*10*/  27,        4,        8,       24,
/*11*/  28,       30,       30,       1,
/*12*/   5,       21,       29,       25,
/*13*/   2,        6,        0,       22,
/*14*/  26,        3,       29,       7,
/*15*/  23,       27,        0,       4,
/*16*/   8,       24,       29,       28,
/*17*/   1,        5,        0,       21,
/*18*/  25,        2,       29,       6,
/*19*/  22,       26,        0,       3,
/*20*/   7,       23,       29,       27,
/*21*/   4,        8,       24,       28,
};

/* 56263 Cfg=2:IntCfg=16 */
static int tdm_seq_16[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/   
/*1*/    1,        5,       29,        0,
/*2*/    2,        6,       29,        0,
/*3*/    3,        7,       29,       30,
/*4*/   30        /* -------- EMPTY ------------ */
};

/* 56233 Cfg=3:IntCfg=17 */
static int tdm_seq_17[] = {
        /*Col:1*/ /*Col:2*/ /*Col:3*/ /*Col:4*/
/*1*/   29,        1,       21,         5,
/*2*/    0,        4,       22,         5,
/*3*/   29,        2,       23,         5,
/*4*/    0,        3,       24,         5,
/*5*/   29,        4,       30,        30,
};

static struct _support_cfgs_s {
    int def_cfg;
    int count;
    int cfgs[6];
} _support_cfgs[] = {
    /* 0, Default the same as CHIP_FLAG_FREQ130 */
    { 1, 6, {1, 2, 3, 4, 5, 6}},
    /* 1, CHIP_FLAG_FREQ130 */
    { 1, 6, {1, 2, 3, 4, 5, 6}},
    /* 2, CHIP_FLAG_FREQ118 without CHIP_FLAG_ACCESS */
    { 7, 2, {7, 8}},
    /* 3, CHIP_FLAG_FREQ75 without CHIP_FLAG_ACCESS */
    { 9, 3, {9, 10, 11}},
    /* 4, CHIP_FLAG_FREQ118 with CHIP_FLAG_ACCESS */
    /* Configs for BCM56260,261,265,266 */
    {12, 2, {12, 13}},
    /* 5, CHIP_FLAG_FREQ75 with CHIP_FLAG_ACCESS */
    /* Configs for BCM56262,267 */
    {14, 1, {14}},
    /* 6, CHIP_FLAG_FREQ37 with CHIP_FLAG_ACCESS */
    /* Configs for BCM56263,268 */
    {15, 2, {15, 16}},
    /* 7, CHIP_FLAG_FREQ37 without CHIP_FLAG_ACCESS */
    /* Configs for BCM56233 */
    {17, 1, {17}}
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
    _PORT_CFG(17)
};

/* Port configuration index for this unit */
static int _cfg_idx[BMD_CONFIG_MAX_UNITS];
static struct _block_map_s {
    struct {
        int port[PORTS_PER_BLOCK];
    } blk[NUM_MXQBLOCKS+NUM_XLBLOCKS];
} _block_map[BMD_CONFIG_MAX_UNITS];

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1

static struct _port_map_s {
    cdk_port_map_port_t map[BMD_CONFIG_MAX_PORTS];
} _port_map[BMD_CONFIG_MAX_UNITS];

static void
_init_port_map(int unit)
{
    cdk_pbmp_t pbmp, pbmp_all;
    int port, pidx = 1;

    CDK_MEMSET(&_port_map[unit], -1, sizeof(_port_map[unit]));

    _port_map[unit].map[0] = CMIC_PORT;

    bcm56260_a0_mxqport_pbmp_get(unit, &pbmp_all);
    bcm56260_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_OR(pbmp_all, pbmp);
    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        CDK_PBMP_ITER(pbmp_all, port) {
            if (P2L(unit, port) > 0) {
                pidx = CDK_PORT_CONFIG_SYS_PORT(unit, port);
                if (pidx >= 0) {
                    _port_map[unit].map[pidx] = port;
                }
            }
        }
    } else {
        CDK_PBMP_ITER(pbmp_all, port) {
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
bcm56260_a0_mxqport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, pbmp);
    CDK_PBMP_ITER(*pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    return 0;
}

int
bcm56260_a0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
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
bcm56260_a0_xlblock_subport_get(int unit, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_XLPORT)) {
        return -1 ;
    } 
    
    return pblk.bport;
}

int
bcm56260_a0_xlport_block_index_get(int unit, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_XLPORT)) {
        return -1 ;
    } 
    return pblk.bindex;
}

int
bcm56260_a0_xlport_from_index(int unit, int blkidx, int blkport)
{
    if (blkidx >= NUM_XLBLOCKS || blkport >= PORTS_PER_BLOCK) {
        return -1;
    }
    
    return _block_map[unit].blk[blkidx + NUM_MXQBLOCKS].port[blkport];
}

int
bcm56260_a0_mxqport_block_port_get(int unit, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_MXQPORT)) {
        return -1 ;
    } 
    
    return pblk.bport;
}

int
bcm56260_a0_mxqport_block_index_get(int unit, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_MXQPORT)) {
        return -1 ;
    } 
    return pblk.bindex;
}

int
bcm56260_a0_mxqport_from_index(int unit, int blkidx, int blkport)
{
    if (blkidx >= NUM_MXQBLOCKS || blkport >= PORTS_PER_BLOCK) {
        return -1;
    }
    
    return _block_map[unit].blk[blkidx].port[blkport];
}

uint32_t
bcm56260_a0_port_speed_max(int unit, int port)
{
    _cfg_map_t *pmap = &_port_cfgs[_cfg_idx[unit]].speed_max;

    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        /* Double check if the port default config is valid */
        if (pmap->val[port-1] == 0) {
            return 0;
        }
        return CDK_PORT_CONFIG_SPEED_MAX(unit, port);
    }

    if (port <= 0 || port > pmap->size) {
        return 0;
    }

    if (pmap->val[port-1] < 0) {
        return 0;
    }
    return pmap->val[port-1];
}

/* Check mxq_phy_mode_get, to get lanes */
int
bcm56260_a0_port_num_lanes(int unit, int port)
{

    int blkidx, subidx;
    int num = 0, lanes;
    cdk_pbmp_t mxqport_pbmp, xlport_pbmp;

    bcm56260_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
    bcm56260_a0_xlport_pbmp_get(unit, &xlport_pbmp);

    if (CDK_PBMP_MEMBER(mxqport_pbmp, port)) {
        blkidx = MXQPORT_BLKIDX(unit, port);
    
        for (subidx = 0; subidx < PORTS_PER_BLOCK; subidx++) {
            if (bcm56260_a0_mxqport_from_index(unit, blkidx, subidx) > 0) {
                num++;
            }
        }
    } else {
        blkidx = XLPORT_BLKIDX(unit, port);
    
        for (subidx = 0; subidx < PORTS_PER_BLOCK; subidx++) {
            if (bcm56260_a0_xlport_from_index(unit, blkidx, subidx) > 0) {
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


int
bcm56260_a0_p2l(int unit, int port, int inverse)
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
bcm56260_a0_tdm_default(int unit, const int **tdm_seq)
{
    _cfg_map_t *tdm = &_port_cfgs[_cfg_idx[unit]].tdm;

    *tdm_seq = tdm->val;

    return tdm->size;
}


int
bcm56260_a0_mxq_phy_mode_get(int unit, int port, int speed, 
                                    int *phy_mode, int *port_num_lanes)
{
    cdk_pbmp_t pbmp;
    
    bcm56260_a0_mxqport_pbmp_get(unit, &pbmp);
    if (CDK_PBMP_MEMBER(pbmp, port)) {
        /* Get PHY Port Mode according to speed */
        /*
        0x0 = SINGLE - Single Port Mode
        0x1 = DUAL - Dual Port Mode
        0x2 = QUAD - Quad Port Mode
        */
        *phy_mode = 0;
        *port_num_lanes = 4;
        if (speed <= 2500) {
            *phy_mode = 2;
            *port_num_lanes = 1;
        } else if (speed >= 10000) {
            *phy_mode = 0;
            *port_num_lanes = 4;
        }
    }
    
    return CDK_E_NONE;
}

int
bcm56260_a0_xl_phy_core_port_mode(int unit, int port, 
                                    int *phy_mode_xl, int *core_mode_xl)
{
    int loop = 0;
    int port_used[4] = {0, 0, 0, 0};
    int num_ports = 0;
    int bport;

    bport = port - XLPORT_SUBPORT(unit, port);
    for(loop = 0 ; loop < 4 ; loop++) {
        if (bcm56260_a0_port_speed_max(unit, bport + loop) > 0) {
            port_used[loop] = 1;
            num_ports++;
        }
    }

    /*
    0x0 = QUAD - Quad Port Mode. Lane 0 only on XLGMII.
    0x1 = TRI_012 - 
          Tri Port Mode. lane 2 is dual, and lanes 0 and 1 are single on XLGMII.
    0x2 = TRI_023 - 
          Tri Port Mode. lane 0 is dual, and lanes 2 and 3 are single on XLGMII.
    0x3 = DUAL - Dual Port Mode. Each of lanes 0 and 2 are dual on XLGMII.
    0x4 = SINGLE - Single Port Mode. Lanes 0 through 4 are single XLGMII.
    0x5 = TDM_DISABLE - Deprecated. TDM Optimize for this core block.
    */
    switch(num_ports) {
    case 0: 
        return CDK_E_PARAM;
    case 1: 
        *phy_mode_xl = 0x4;
        *core_mode_xl = 0x4;
        break;
    case 2:
        *phy_mode_xl = 0x3;
        *core_mode_xl = 0x3;
        break;
    case 3: 
        if(port_used[0] && port_used[2] && port_used[3]) {
            *phy_mode_xl = 0x2;
            *core_mode_xl = 0x2;
        } else if (port_used[0] && port_used[1] && port_used[2]) {
            *phy_mode_xl = 0x1;
            *core_mode_xl = 0x1;
        } else {
             return CDK_E_PARAM; 
        } 
        break;
    case 4: 
        *phy_mode_xl = 0;   
        *core_mode_xl = 0;
    }
    return CDK_E_NONE;
}

int
bcm56260_a0_bmd_attach(int unit)
{
    int port, blkidx, blkport;
    cdk_pbmp_t pbmp, pbmp_all;
    int port_cfg, list_id;

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    CDK_MEMSET(_block_map, 0, sizeof(_block_map));
    
    /* Get support config list ID */
    list_id = 0;
    if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ118) {
        list_id = 2;
        if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_ACCESS) {
            list_id = 4;
        } 
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ75) {
        list_id = 3;
        if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_ACCESS) {
            list_id = 5;
        } 
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ130) {
        list_id = 1;
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ37) {
        list_id = 7;
        if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_ACCESS) {
            list_id = 6;
        }
    }

    /* Select port configuration */
    _cfg_idx[unit] = -1;
    port_cfg = CDK_CHIP_CONFIG(unit);

    if (port_cfg > 0 && port_cfg <= _support_cfgs[list_id].count) {
         _cfg_idx[unit] = 
                    _port_cfg_index(_support_cfgs[list_id].cfgs[port_cfg - 1]);
    }
    if (_cfg_idx[unit] < 0) {
        if (port_cfg > 0) {
            /* Warn if unsupported configuration was requested */
            CDK_WARN(("bcm56260_a0_bmd_attach[%d]: Invalid port config (%d)\n",
                      unit, port_cfg));
        }
        /* Fall back to default port configuration */
        _cfg_idx[unit] = _port_cfg_index(_support_cfgs[list_id].def_cfg);
    }
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, &pbmp_all);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_OR(pbmp_all, pbmp);
    CDK_PBMP_ITER(pbmp_all, port) {
        int speed_max;

        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        
        speed_max = bcm56260_a0_port_speed_max(unit, port);
        if (speed_max <= 0) {
            continue;
        }
        
        if (speed_max > 2500) {
            switch (speed_max) {
#if BMD_CONFIG_INCLUDE_HIGIG == 1
            case 11000:
            case 13000:
            case 21000:
            case 42000:
                BMD_PORT_PROPERTIES(unit, port) |= BMD_PORT_HG;
                break;
#endif /* BMD_CONFIG_INCLUDE_HIGIG == 1 */
            default:
                BMD_PORT_PROPERTIES(unit, port) |= BMD_PORT_XE;
                break;
            }
        } else {
            BMD_PORT_PROPERTIES(unit, port) |= BMD_PORT_GE;
        }
    }

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    /* Attach the PHY bus driver */
#if BMD_CONFIG_INCLUDE_PHY == 1
    bcm56260_a0_mxqport_pbmp_get(unit, &pbmp_all);
    bcm56260_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_OR(pbmp_all, pbmp);
    CDK_PBMP_ITER(pbmp_all, port) {
        bmd_phy_bus_set(unit, port, bcm56260_phy_bus);
    }
#endif /* BMD_CONFIG_INCLUDE_PHY */

    /* 
     * _mxqblock_ports is used to store the MXQ block information 
     * That is required to process the ports in serdes order instead of in 
     * physical order. 
     */
    bcm56260_a0_mxqport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        blkidx = MXQPORT_BLKIDX(unit, port);
        blkport = MXQPORT_SUBPORT(unit, port);
        if ((blkidx >= 0 && blkidx < NUM_MXQBLOCKS) && 
            (blkport >= 0 && blkport < PORTS_PER_BLOCK)) {
            _block_map[unit].blk[blkidx].port[blkport] = port;
        }
    }
    bcm56260_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        blkidx = XLPORT_BLKIDX(unit, port);
        blkport = XLPORT_SUBPORT(unit, port);
        if ((blkidx >= 0 && blkidx < NUM_XLBLOCKS) &&
            (blkport >= 0 && blkport < PORTS_PER_BLOCK)) {
            _block_map[unit].blk[NUM_MXQBLOCKS + blkidx].port[blkport] = port;
        }
    }

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    /* Match default API port map to configured logical ports */
    _init_port_map(unit);
#endif /* CDK_CONFIG_INCLUDE_PORT_MAP */

    /* Initialize debug functions */
    BMD_PORT_SPEED_MAX(unit) = bcm56260_a0_port_speed_max;
    BMD_PORT_P2L(unit) = bcm56260_a0_p2l;
    BMD_PORT_P2M(unit) = bcm56260_a0_p2l;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56260_A0 */
