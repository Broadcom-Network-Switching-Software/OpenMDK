#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56270_A0 == 1

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

#include <cdk/chip/bcm56270_a0_defs.h>
#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include "bcm56270_a0_bmd.h"
#include "bcm56270_a0_internal.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm56270_phy_bus[] = {
#ifdef PHY_BUS_BCM56270_MIIM_INT_INSTALLED
    &phy_bus_bcm56270_miim_int,
#endif
    NULL
};
#endif /* BMD_CONFIG_INCLUDE_PHY */

/* Unicast queue base per unit/port */
static int _uc_q_base[BMD_CONFIG_MAX_UNITS][BMD_CONFIG_MAX_PORTS];

/* port speed arrays */
/* 56270_1: bcm5627x_config = 1 */
/* 4x2.5G + 4x10G + 4x11G + 2.5G LPBK   */
static int port_speed_max_1[] = {
       2500, 2500, 2500, 2500,          /* 1 - 4  */
       10000, 10000, 10000, 10000,      /* 5 - 8  */
       11000, 11000, 11000, 11000,      /* 9 - 12 */
};

/* 56271_1: Cfg=1:IntCfg=2 */
/* 4x1G/2.5G + 4xXFI + 4x1G/2.5G + 2.5G LPBK */
static int port_speed_max_2[] = {
       2500, 2500, 2500, 2500,          /* 1 - 4  */
       10000, 10000, 10000, 10000,      /* 5 - 8  */
       2500, 2500, 2500, 2500,          /* 9 - 12 */
};

/* 56271_2 Cfg=2:IntCfg=3 */
/* 4x1G/2.5G G.INT+ 2x1G/2.5G/10G */
static int port_speed_max_3[] = {
       2500, 2500, 2500, 2500,          /* 1 - 4  */
       10000, 0, 0, 0,                  /* 5 - 8  */
       10000, 0, 10000, 0,              /* 9 - 12 */
};

/* 56271_3 Cfg=3:IntCfg=4 */
/* 4x1G/2.5G G.INT + 4x1G/2.5G/10G/11G */
static int port_speed_max_4[] = {
       2500, 2500, 2500, 2500,          /* 1 - 4  */
       10000, 0, 0, 0,                  /* 5 - 8  */
       11000, 11000, 11000, 11000,      /* 9 - 12 */
};

/* 56272_1 Cfg=1:IntCfg=5 */
/* 8xGE + 1G LPBK */
static int port_speed_max_5[] = {
       1000, 1000, 1000, 1000,     /* 1 - 4  */
       2500, 0, 0, 0,              /* 5 - 8  */
       1000, 1000, 1000, 1000,     /* 9 - 12 */
};

/* 56272_2 Cfg=2:IntCfg=6 */
/* 4x1G G.INT + 2x1G + 1G LPBK */
static int port_speed_max_6[] = {
       1000, 1000, 1000, 1000,     /* 1 - 4  */
       2500, 0, 0, 0,              /* 5 - 8  */
       1000, 0, 1000, 0,           /* 9 - 12 */
};

/* 56270_1: bcm5627x_config = 1 */
static int tdm_seq_1[] = {
       /*1*/ /*2*/ /*3*/ /*4*/ /*5*/ /*6*/ /*7*/ /*8*/ /*9*/ /*10*/ /*11*/
/*1*/  5,    6,	   7,    8,    1,    9,    10,   11,   12,   5,     2,
/*2*/  6,    7,	   8,    9,    3,    10,   11,   12,   5,    6,     4,
/*3*/  7,    8,	   9,    10,   14,   14,   11,   12,   5,    6,     7,
/*4*/  8,    9,	   10,   11,   1,    12,   5,    6,    7,    8,     2,
/*5*/  9,    10,   11,   12,   3,    5,	   6,    7,    8,    9,     4,
/*6*/  10,   11,   12,   5,    13,   6,	   7,    8,    9,    10,    0,
/*7*/  11,   12,   5,    6,    13,   7,	   8,    9,    10,   11,    0,
/*8*/  12,   5,	   6,    7,    1,    8,	   9,    10,   11,   12,    2,
/*9*/  5,    6,	   7,    8,    3,    9,	   10,   11,   12,   5,     4,
/*10*/ 6,    7,	   8,    9,	   14,   14,   10,   11,   12,   5,     6,
/*11*/ 7,    8,	   9,    10,   1,    11,   12,   5,    6,    7,     2,
/*12*/ 8,    9,	   10,   11,   3,    12,   5,    6,    7,    8,     4,
/*13*/ 9,    10,   11,   12,   13,   5,    6,    7,    8,    9,     0,
/*14*/ 10,   11,   12,   5,	   13,   6,    7,    8,    9,    10,    0,
/*15*/ 11,   12,   5,    6,    1,    7,    8,    9,    10,   11,    2,
/*16*/ 12,   5,	   6,    7,    3,    8,    9,    10,   11,   12,    4,
/*17*/ 5,    6,	   7,    8,	   14,   14,   9,    10,   11,   12,    5,
/*18*/ 6,    7,	   8,    9,	   13,   10,   11,   12,   5,    6,     14,
/*19*/ 7,    8,	   9,    10,   11,   12	 /* --------- EMPTY ---------*/
};

/* 56271_1: Cfg=1:IntCfg=2 */
static int tdm_seq_2[] = {
       /*1*/ /*2*/ /*3*/ /*4*/ /*5*/ /*6*/ /*7*/
/*1*/  5,    6,    7,    8,    1,    9,	   0,
/*2*/  5,    6,    7,    8,    2,    10,   13,
/*3*/  5,    6,    7,    8,    3,    11,   14,
/*4*/  5,    6,    7,    8,    4,    12,   /* EMPTY */
/*5*/  5,    6,    7,    8,    1,    9,	   /* EMPTY */
/*6*/  5,    6,    7,    8,    2,    10,   13,
/*7*/  5,    6,    7,    8,    3,    11,   0,
/*8*/  5,    6,    7,    8,    4,    12,   14,
/*9*/  5,    6,    7,    8,    1,    9,    /* EMPTY */
/*10*/ 5,    6,    7,    8,    2,    10,   13,
/*11*/ 5,    6,    7,    8,    3,    11,   14,
/*12*/ 5,    6,    7,    8,    4,    12,   /* EMPTY */
/*13*/ 5,    6,    7,    8,    1,    9,	   0,
/*14*/ 5,    6,    7,    8,    2,    10,   13,
/*15*/ 5,    6,    7,    8,    3,    11,   14,
/*16*/ 5,    6,    7,    8,    4,    12,   /* EMPTY */
/*17*/ 5,    6,    7,    8,    1,    9,	   /* EMPTY */
/*18*/ 5,    6,    7,    8,    2,    10,   13,
/*19*/ 5,    6,    7,    8,    3,    11,   0,
/*20*/ 5,    6,    7,    8,    4,    12,   14,
/*21*/ 5,    6,    7,    8,    14,   14	   /* EMPTY */
};

/* 56271_2: Cfg=2:IntCfg=3 */
static int tdm_seq_3[] = {
       /*1*/ /*2*/ /*3*/ /*4*/ /*5*/ /*6*/ /*7*/
/*1*/  9,    14,   11,   14,   1,    14,   0,
/*2*/  9,    14,   11,   14,   2,    14,   13,
/*3*/  9,    14,   11,   14,   3,    14,   14,
/*4*/  9,    14,   11,   14,   4,    14,   /* EMPTY */
/*5*/  9,    14,   11,   14,   1,    14,   /* EMPTY */
/*6*/  9,    14,   11,   14,   2,    14,   13,
/*7*/  9,    14,   11,   14,   3,    14,   0,
/*8*/  9,    14,   11,   14,   4,    14,   14,
/*9*/  9,    14,   11,   14,   1,    14,   /* EMPTY */
/*10*/ 9,    14,   11,   14,   2,    14,   13,
/*11*/ 9,    14,   11,   14,   3,    14,   14,
/*12*/ 9,    14,   11,   14,   4,    14,   /* EMPTY */
/*13*/ 9,    14,   11,   14,   1,    14,   0,
/*14*/ 9,    14,   11,   14,   2,    14,   13,
/*15*/ 9,    14,   11,   14,   3,    14,   14,
/*16*/ 9,    14,   11,   14,   4,    14,   /* EMPTY */
/*17*/ 9,    14,   11,   14,   1,    14,   /* EMPTY */
/*18*/ 9,    14,   11,   14,   2,    14,   13,
/*19*/ 9,    14,   11,   14,   3,    14,   0,
/*20*/ 9,    14,   11,   14,   4,    14,   14,
/*21*/ 9,    14,   11,   14,   14,   14	   /* EMPTY */
};

/* 56271_3: Cfg=3:IntCfg=4 */
static int tdm_seq_4[] = {
       /*1*/ /*2*/ /*3*/ /*4*/ /*5*/ /*6*/ /*7*/
/*1*/  9,    10,   11,   12,   1,    14,   0,
/*2*/  9,    10,   11,   12,   2,    14,   13,
/*3*/  9,    10,   11,   12,   3,    14,   14,
/*4*/  9,    10,   11,   12,   4,    14,   /* EMPTY */
/*5*/  9,    10,   11,   12,   1,    14,   /* EMPTY */
/*6*/  9,    10,   11,   12,   2,    14,   13,
/*7*/  9,    10,   11,   12,   3,    14,   0,
/*8*/  9,    10,   11,   12,   4,    14,   14,
/*9*/  9,    10,   11,   12,   1,    14,   /* EMPTY */
/*10*/ 9,    10,   11,   12,   2,    14,   13,
/*11*/ 9,    10,   11,   12,   3,    14,   14,
/*12*/ 9,    10,   11,   12,   4,    14,   /* EMPTY */
/*13*/ 9,    10,   11,   12,   1,    14,   0,
/*14*/ 9,    10,   11,   12,   2,    14,   13,
/*15*/ 9,    10,   11,   12,   3,    14,   14,
/*16*/ 9,    10,   11,   12,   4,    14,   /* EMPTY */
/*17*/ 9,    10,   11,   12,   1,    14,   /* EMPTY */
/*18*/ 9,    10,   11,   12,   2,    14,   13,
/*19*/ 9,    10,   11,   12,   3,    14,   0,
/*20*/ 9,    10,   11,   12,   4,    14,   14,
/*21*/ 9,    10,   11,   12,   14,   14	   /* EMPTY */
};

/* 56272_1 Cfg=1:IntCfg=5 */
static int tdm_seq_5[] = {
      /*1*/ /*2*/ /*3*/ /*4*/
/*1*/ 14,	1,	  9,    5,
/*2*/ 0,	2,	  10,   5,
/*3*/ 13,	3,	  11,   5,
/*4*/ 0,	4,	  12,   14
};

/* 56272_2 Cfg=2:IntCfg=6 */
static int tdm_seq_6[] = {
      /*1*/ /*2*/ /*3*/ /*4*/
/*1*/ 14,	1,	  9,    5,
/*2*/ 0,	2,	  14,   5,
/*3*/ 13,	3,	  11,   5,
/*4*/ 0,	4,	  14,   14
};

static struct _config_info_s {
    int def_cfg;
    int count;
    int cfgs[3];
} _config_info[] = {
    /* 0, Default the same as CHIP_FLAG_FREQ180 */
    { 1, 1, {1}},
    /* 1, CHIP_FLAG_FREQ180 */
    { 1, 1, {1}},
    /* 2, CHIP_FLAG_FREQ125 */
    { 2, 3, {2, 3, 4}},
    /* 3, CHIP_FLAG_FREQ30 */
    { 5, 2, {5, 6}}
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
    _PORT_CFG(6)
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

    bcm56270_a0_mxqport_pbmp_get(unit, &pbmp_all);
    bcm56270_a0_xlport_pbmp_get(unit, &pbmp);
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
bcm56270_a0_mxqport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
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
bcm56270_a0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
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
bcm56270_a0_xlblock_subport_get(int unit, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_XLPORT)) {
        return -1 ;
    }
    return pblk.bport;
}

int
bcm56270_a0_xlport_block_index_get(int unit, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_XLPORT)) {
        return -1 ;
    }
    return pblk.bindex;
}

int
bcm56270_a0_xlport_from_index(int unit, int blkidx, int blkport)
{
    if (blkidx >= NUM_XLBLOCKS || blkport >= PORTS_PER_BLOCK) {
        return -1;
    }
    return _block_map[unit].blk[blkidx + NUM_MXQBLOCKS].port[blkport];
}

int
bcm56270_a0_mxqblock_subport_get(int unit, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_MXQPORT)) {
        return -1 ;
    }
    return pblk.bport;
}

int
bcm56270_a0_mxqport_block_index_get(int unit, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_MXQPORT)) {
        return -1 ;
    }
    return pblk.bindex;
}

int
bcm56270_a0_mxqport_from_index(int unit, int blkidx, int blkport)
{
    if (blkidx >= NUM_MXQBLOCKS || blkport >= PORTS_PER_BLOCK) {
        return -1;
    }
    return _block_map[unit].blk[blkidx].port[blkport];
}

uint32_t
bcm56270_a0_port_speed_max(int unit, int port)
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
bcm56270_a0_port_num_lanes(int unit, int port)
{

    int blkidx, subidx;
    int num = 0, lanes;
    cdk_pbmp_t mxqport_pbmp, xlport_pbmp;

    bcm56270_a0_mxqport_pbmp_get(unit, &mxqport_pbmp);
    bcm56270_a0_xlport_pbmp_get(unit, &xlport_pbmp);

    if (CDK_PBMP_MEMBER(mxqport_pbmp, port)) {
        blkidx = MXQPORT_BLKIDX(unit, port);

        for (subidx = 0; subidx < PORTS_PER_BLOCK; subidx++) {
            if (bcm56270_a0_mxqport_from_index(unit, blkidx, subidx) > 0) {
                num++;
            }
        }
    } else {
        blkidx = XLPORT_BLKIDX(unit, port);

        for (subidx = 0; subidx < PORTS_PER_BLOCK; subidx++) {
            if (bcm56270_a0_xlport_from_index(unit, blkidx, subidx) > 0) {
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
bcm56270_a0_p2l(int unit, int port, int inverse)
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
bcm56270_a0_tdm_default(int unit, const int **tdm_seq)
{
    _cfg_map_t *tdm = &_port_cfgs[_cfg_idx[unit]].tdm;

    *tdm_seq = tdm->val;

    return tdm->size;
}


int
bcm56270_a0_mxq_phy_mode_get(int unit, int port, int speed,
                                    int *phy_mode, int *port_num_lanes)
{
    cdk_pbmp_t pbmp;

    bcm56270_a0_mxqport_pbmp_get(unit, &pbmp);
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
bcm56270_a0_xl_phy_core_port_mode(int unit, int port,
                                    int *phy_mode_xl, int *core_mode_xl)
{
    int loop = 0;
    int port_used[4] = {0, 0, 0, 0};
    int num_ports = 0;
    int bport;

    bport = port - XLPORT_SUBPORT(unit, port);
    for(loop = 0 ; loop < 4 ; loop++) {
        if (bcm56270_a0_port_speed_max(unit, bport + loop) > 0) {
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

static int
bcm56270_a0_mmu_port_uc_queues(int unit, int port)
{
    int mport = P2M(unit, port);

    if (port == CMIC_PORT || port == LB_PORT || mport < 0) {
        return 0 ;
    }

    return 8;
}

static int
_q_num_config(int unit)
{
    int phy_port, lport;
    int uc_base, uc_queues;
    CDK_MEMSET(&_uc_q_base[unit], -1, BMD_CONFIG_MAX_PORTS * sizeof(int));
    _uc_q_base[unit][CMIC_PORT] = UC_Q_BASE_CMIC_PORT;
    _uc_q_base[unit][LB_LPORT] = UC_Q_BASE_LB_PORT;

    uc_base = UC_Q_BASE_CMIC_PORT + UC_Q_BASE_LB_PORT;
    for (phy_port = 0; phy_port < MAX_PORT; phy_port++) {
        lport = P2L(unit, phy_port);
        if (lport == -1 || lport == CMIC_PORT || lport == LB_PORT) {
            continue;
        }
        uc_queues = bcm56270_a0_mmu_port_uc_queues(unit, phy_port);
        _uc_q_base[unit][phy_port] = uc_base + (phy_port - 1) * uc_queues;
    }

    return 0;
}

int
bcm56270_a0_uc_queue_num(int unit, int port, int cosq)
{
    if (port >= 0 && port < BMD_CONFIG_MAX_PORTS) {
        return _uc_q_base[unit][port] + cosq;
    }

    return -1;
}

int
bcm56270_a0_uc_cosq_base(int unit, int port)
{
    /* CMIC : num=48, 0:47
       LB   : num=24, 48: 71
       port1: num=8,  72:80
       ...
    */
    if (port == CMIC_PORT) {
        return 0;
    } else if (port == LB_PORT) {
        return UC_Q_BASE_CMIC_PORT;
    } else if (port < 9) {
        return (UC_Q_BASE_CMIC_PORT + UC_Q_BASE_LB_PORT + (port - 1) * 8);
    } else {
        return (UC_Q_BASE_CMIC_PORT + UC_Q_BASE_LB_PORT + 8 * 8 + (port - 9) * 24);
    }
    return 0;
}
int
bcm56270_a0_bmd_attach(int unit)
{
    int port, blkidx, blkport;
    cdk_pbmp_t pbmp, pbmp_all;
    int port_cfg, list_id;
    struct _config_info_s *ci;
    
    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    CDK_MEMSET(_block_map, 0, sizeof(_block_map));

    /* Get support config list ID */
    list_id = 0;
    if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ180) {
        list_id = 1;
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ125) {
        list_id = 2;
    } else if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_FREQ30) {
        list_id = 3;
    }
    ci = &_config_info[list_id];
    /* Select port configuration */
    _cfg_idx[unit] = -1;
    port_cfg = CDK_CHIP_CONFIG(unit);

    if (port_cfg > 0 && port_cfg <= ci->count) {
         _cfg_idx[unit] = _port_cfg_index(ci->cfgs[port_cfg - 1]);
    }
    if (_cfg_idx[unit] < 0) {
        if (port_cfg > 0) {
            /* Warn if unsupported configuration was requested */
            CDK_WARN(("bcm56270_a0_bmd_attach[%d]: Invalid port config (%d)\n",
                      unit, port_cfg));
        }
        /* Fall back to default port configuration */
        _cfg_idx[unit] = _port_cfg_index(ci->def_cfg);
    }

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_MXQPORT, &pbmp_all);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_OR(pbmp_all, pbmp);
    CDK_PBMP_ITER(pbmp_all, port) {
        int speed_max;

        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);

        speed_max = bcm56270_a0_port_speed_max(unit, port);
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
    bcm56270_a0_mxqport_pbmp_get(unit, &pbmp_all);
    bcm56270_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_OR(pbmp_all, pbmp);
    CDK_PBMP_ITER(pbmp_all, port) {
        bmd_phy_bus_set(unit, port, bcm56270_phy_bus);
    }
#endif /* BMD_CONFIG_INCLUDE_PHY */

    /*
     * _mxqblock_ports is used to store the MXQ block information
     * That is required to process the ports in serdes order instead of in
     * physical order.
     */
    bcm56270_a0_mxqport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        blkidx = MXQPORT_BLKIDX(unit, port);
        blkport = MXQPORT_SUBPORT(unit, port);
        if ((blkidx >= 0 && blkidx < NUM_MXQBLOCKS) &&
            (blkport >= 0 && blkport < PORTS_PER_BLOCK)) {
            _block_map[unit].blk[blkidx].port[blkport] = port;
        }
    }
    bcm56270_a0_xlport_pbmp_get(unit, &pbmp);
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
    BMD_PORT_SPEED_MAX(unit) = bcm56270_a0_port_speed_max;
    BMD_PORT_P2L(unit) = bcm56270_a0_p2l;
    BMD_PORT_P2M(unit) = bcm56270_a0_p2l;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    _q_num_config(unit);

    return 0;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56270_A0 */
