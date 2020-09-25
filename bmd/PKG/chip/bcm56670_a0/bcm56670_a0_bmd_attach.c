/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56670_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_phy_ctrl.h>

#include <cdk/chip/bcm56670_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm56670_a0_bmd.h"
#include "bcm56670_a0_internal.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm56670_phy_bus[] = {
#ifdef PHY_BUS_BCM56670_MIIM_INT_INSTALLED
    &phy_bus_bcm56670_miim_int,
#endif
    NULL
};
#define PHY_BUS_SET(_u,_p,_b) bmd_phy_bus_set(_u,_p,_b)
#else
#define PHY_BUS_SET(_u,_p,_b)
#endif

/* Unicast queue base per unit/port */
static int _uc_q_base[BMD_CONFIG_MAX_UNITS][BMD_CONFIG_MAX_PORTS];
static int _mc_q_base[BMD_CONFIG_MAX_UNITS][BMD_CONFIG_MAX_PORTS];

static struct _mmu_port_map_s {
    cdk_port_map_port_t map[BMD_CONFIG_MAX_PORTS];
} _mmu_port_map[BMD_CONFIG_MAX_UNITS];

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

    bcm56670_a0_all_pbmp_get(unit, &pbmp);
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
bcm56670_a0_port_in_eq_bmp(int unit, int port)
{
    return TRUE;
}

/*
 * The MMU port mappings must be derived from the individual
 * port configurations such as maximum speed and queueing
 * capabilities.
 * The function assign the front-port mmu mapping.
 */
static void
_mmu_ports_assign(int unit)
{
    cdk_pbmp_t pbmp;
    int phy_port, mmu_port;
    cdk_pbmp_t pbmp_add;
    int num_phy_port = 63;
    /* Direct assign CMIC_MPORT(65) LB_MPORT(66) RDB_MPORT(64) */

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_CLPORT, &pbmp_add);
    CDK_PBMP_OR(pbmp, pbmp_add);

    /* Clear MMU port map */
    CDK_MEMSET(&_mmu_port_map[unit], -1, sizeof(_mmu_port_map[unit]));

    mmu_port = 0;
    /* Assign MMU port for all OS ports with speed >= 100G */
    for (phy_port = 1; phy_port <= num_phy_port; phy_port++) {
        if (_mmu_port_map[unit].map[phy_port] != -1) {
            continue;
        }

        if (P2L(unit, phy_port) == -1) {
            continue;
        }

        if ((CDK_PORT_CONFIG_PORT_FLAGS(unit, phy_port) & CDK_DCFG_PORT_F_OVERSUB) &&
            (SPEED_MAX(unit, phy_port) >= 100000)) {
            _mmu_port_map[unit].map[phy_port] = mmu_port;
            mmu_port++;
        }
    }

    /* Assign MMU port for all OS ports with speed >= 40G */
    for (phy_port = 1; phy_port <= num_phy_port; phy_port++) {
        if (_mmu_port_map[unit].map[phy_port]!= -1) {
            continue;
        }

        if (P2L(unit, phy_port) == -1) {
            continue;
        }

        if ((CDK_PORT_CONFIG_PORT_FLAGS(unit, phy_port) & CDK_DCFG_PORT_F_OVERSUB) &&
            (SPEED_MAX(unit, phy_port) >= 40000)) {
            _mmu_port_map[unit].map[phy_port] = mmu_port;
            mmu_port++;
        }
    }

    /* Assign MMU port for all ports with speed >= 100G */
    for (phy_port = 1; phy_port <= num_phy_port; phy_port++) {
        if (_mmu_port_map[unit].map[phy_port] != -1) {
            continue;
        }

        if (P2L(unit, phy_port) == -1) {
            continue;
        }

        if (SPEED_MAX(unit, phy_port) >= 100000) {
            _mmu_port_map[unit].map[phy_port] = mmu_port;
            mmu_port++;
        }
    }

    /* Assign MMU port for all ports with speed >= 40G */
    for (phy_port = 1; phy_port <= num_phy_port; phy_port++) {
        if (_mmu_port_map[unit].map[phy_port] != -1) {
            continue;
        }

        if (P2L(unit, phy_port) == -1) {
            continue;
        }

        if (SPEED_MAX(unit, phy_port) >= 40000) {
            _mmu_port_map[unit].map[phy_port] = mmu_port;
            mmu_port++;
        }
    }
    
    /* Assign MMU port for 1st phy port in PM4x25|PM4x10 */
    for (phy_port = 1; phy_port <= num_phy_port; phy_port+= PORTS_PER_TSC) {
        if (_mmu_port_map[unit].map[phy_port]!= -1) {
            continue;
        }

        if (P2L(unit, phy_port) == -1) {
            continue;
        }

        _mmu_port_map[unit].map[phy_port] = mmu_port;
        mmu_port++;
    }

    /* Assign MMU port for all ports with speed >= 25G */
    for (phy_port = 1; phy_port <= num_phy_port; phy_port++) {
        if (_mmu_port_map[unit].map[phy_port] != -1) {
            continue;
        }

        if (P2L(unit, phy_port) == -1) {
            continue;
        }

        if (SPEED_MAX(unit, phy_port) >= 25000) {
            _mmu_port_map[unit].map[phy_port] = mmu_port;
            mmu_port++;
        }
    }

    /* Assign MMU port for all remaining ports with speeds >= 10G */
    for (phy_port = 1; phy_port <= num_phy_port; phy_port++) {
        if (_mmu_port_map[unit].map[phy_port] != -1) {
            continue;
        }

        if (P2L(unit, phy_port) == -1) {
            continue;
        }

        if (SPEED_MAX(unit, phy_port) >= 10000) {
            _mmu_port_map[unit].map[phy_port] = mmu_port;
            mmu_port++;
        }
    }

    /* Assign MMU port for all remaining ports */
    for (phy_port = 1; phy_port <= num_phy_port; phy_port++) {
        if (_mmu_port_map[unit].map[phy_port] != -1) {
            continue;
        }
        _mmu_port_map[unit].map[phy_port] = mmu_port;
        mmu_port++;
    }
}

#if 1
static int
_q_num_config(int unit)
{
    int phy_port, lport, mport_offset;
    int uc_base, mc_base;

    CDK_MEMSET(&_mc_q_base[unit], -1, BMD_CONFIG_MAX_PORTS * sizeof(int));
    CDK_MEMSET(&_uc_q_base[unit], -1, BMD_CONFIG_MAX_PORTS * sizeof(int));

    _mc_q_base[unit][CMIC_PORT] = MC_Q_BASE_CMIC_PORT;
    _mc_q_base[unit][LB_LPORT] = MC_Q_BASE_LB_PORT;
    _mc_q_base[unit][RDB_LPORT] = 0;
    _uc_q_base[unit][CMIC_PORT] = UC_Q_BASE_CMIC_PORT;
    _uc_q_base[unit][LB_LPORT] = UC_Q_BASE_LB_PORT;

    uc_base = 0;
    mc_base = MC_Q_BASE;
    for (mport_offset = 0; mport_offset < PORTS_PER_PIPE; mport_offset++) {
        phy_port = M2P(unit, mport_offset);
        lport = P2L(unit, phy_port);
        if (lport == -1 || lport == CMIC_PORT || lport == LB_PORT
                                              || lport == RDB_LPORT) {
            continue;
        }

        _mc_q_base[unit][phy_port] = mc_base;
        mc_base += bcm56670_a0_mmu_port_mc_queues(unit, phy_port);

        _uc_q_base[unit][phy_port] = uc_base;
        uc_base += bcm56670_a0_mmu_port_uc_queues(unit, phy_port);
    }

    return 0;
}
#endif

int
bcm56670_a0_get_core_frequency(int unit)
{
    uint32_t op_cfg;
    int freq;
    
    /* default freq set to 815 */
    op_cfg = CDK_CHIP_CONFIG(unit) & CHIP_FREQ_MASK;
    switch (op_cfg) {
        case DCFG_FREQ510:
            freq = 510;
            break;
        case DCFG_FREQ705:
            freq = 705;
            break;
        case DCFG_FREQ816:
            freq = 816;
            break;
        case DCFG_FREQ817:
            freq = 817;
            break;
        case DCFG_FREQ818:
            freq = 818;
            break;
        case DCFG_FREQ819:
            freq = 819;
            break;
        case DCFG_FREQ820:
            freq = 820;
            break;
        case DCFG_FREQ861:
            freq = 861;
            break;
        case DCFG_FREQ862:
            freq = 862;
            break;
        default:
            freq = 815;
        break;
    }
    return freq;
}

int
bcm56670_a0_port_lanes_get(int unit, int port)
{
    uint32_t speed_max;
    int cports[] = { 13, 17, 45, 49 };
    int cx;
    int cx_valid = 0;
    int lanes = 1;

    if (port < 0 || port >= NUM_PHYS_PORTS) {
        return -1;
    }

    /* Get CPORT instance */
    for (cx = 0; cx < COUNTOF(cports); cx++) {
        if (port == cports[cx]) {
            cx_valid = 1;
            break;
        }
    }

    speed_max = SPEED_MAX(unit, port);
    if (IS_FALCON(unit, port) && !IS_CPRI(unit, port)) {
        if (speed_max > 53000) {
            lanes = (cx_valid && IS_100G_ENABLED(unit, cx)) ? 4 : -1;
        } else if (speed_max > 42000) {
            lanes = 2;
        } else if (speed_max > 27000) {
            lanes = (cx_valid && IS_100G_ENABLED(unit, cx)) ? 2 : 4;
        } else {
            lanes = 1;
        }
    } else {
        /* XLPORT blocks */
        if (speed_max >= 100000) {
                if ((port == 17) || (port == 53)) {
                    lanes = (cx_valid && IS_100G_ENABLED(unit, cx)) ? 10 : -1;
                } else {
                    lanes = 10;
                }
        } else if (speed_max > 21000) {
            lanes = 4;
        } else if (speed_max > 11000) {
            lanes = 2;
        } else {
            lanes = 1;
        }
    }
    return lanes;
}

int
bcm56670_a0_cpri_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_CPRI, pbmp);
    CDK_PBMP_ITER(*pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    return 0;
}

int
bcm56670_a0_port_is_cpri(int unit, int port)
{
    cdk_pbmp_t cpri_pbmp;

    bcm56670_a0_cpri_pbmp_get(unit, &cpri_pbmp);

    if (CDK_PBMP_MEMBER(cpri_pbmp, port)) {
        return 1;
    }
    return 0;
}

int
bcm56670_a0_port_is_falcon(int unit, int port)
{

    if ((port >= 1 && port <= 20) || (port >= 45 && port <= 64)) {
        return TRUE;
    }

    return FALSE;
}

int
bcm56670_a0_clport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
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

int
bcm56670_a0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
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
bcm56670_a0_all_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    cdk_pbmp_t pbmp_add;

    bcm56670_a0_xlport_pbmp_get(unit, pbmp);
    bcm56670_a0_clport_pbmp_get(unit, &pbmp_add);
    CDK_PBMP_OR(*pbmp, pbmp_add);
    CDK_PBMP_PORT_ADD(*pbmp, CMIC_PORT);
    CDK_PBMP_PORT_ADD(*pbmp, LB_PORT);

    return 0;
}

int
bcm56670_a0_all_front_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    cdk_pbmp_t pbmp_add;

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, pbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_CLPORT, &pbmp_add);
    CDK_PBMP_OR(*pbmp, pbmp_add);

    return 0;
}

void
bcm56670_a0_mmu_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;

    CDK_PBMP_CLEAR(*pbmp);
    for (port = 1; port <= 67; port++) {
        if (_mmu_port_map[unit].map[port] != -1) {
            CDK_PBMP_PORT_ADD(*pbmp, port);
        }
    }

}

void
bcm56670_a0_ovrs_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;

    bcm56670_a0_all_pbmp_get(unit, pbmp);

    CDK_PBMP_ITER(*pbmp, port) {
        if (!(CDK_PORT_CONFIG_PORT_FLAGS(unit, port) & CDK_DCFG_PORT_F_OVERSUB)) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
}

int
bcm56670_a0_is_ovrs_port(int unit, int port)
{
    cdk_pbmp_t pbmp;

    bcm56670_a0_ovrs_pbmp_get(unit, &pbmp);

    if (CDK_PBMP_MEMBER(pbmp, port)) {
        return 1;
    }

    return 0;
}

/* Apply to CLPORT block
 * return 1: this block is disabeld
 */
int
bcm56670_a0_is_cblk_disabled(int unit, int clp_blk)
{
    cdk_pbmp_t clpbmp;
    int port;
    int idx;

    bcm56670_a0_clport_pbmp_get(unit, &clpbmp);

    switch (clp_blk) {
    case 0:
        port = 17;
        break;
    case 1:
        port = 29;
        break;
    case 2:
        port = 33;
        break;
    case 3:
        port = 53;
        break;
    case 4:
        port = 65;
        break;
    default:
        port = 69;
        break;
    }

    for (idx = 1; idx < 4; idx++) {
        if (CDK_PBMP_MEMBER(clpbmp, port + idx)) {
            return 1;
        }
    }

    return 0;
}

int
bcm56670_a0_block_index_get(int unit, int port, int *obm, int *sub_port)
{
    if (port <= 32) {
        *obm = (port - 1) / 4;
        *sub_port = (port - 1) - (*obm) * 4;
        return *obm;
    } else {
        port -= 32;
        *obm = (port - 1) / 4;
        *sub_port = (port - 1) - (*obm) * 4;
        return (8 + *obm);
    }
}

int
bcm56670_a0_cpri_block_index_get(int unit, int port, int *obm, int *sub_port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_CPRI)) {
        return -1 ;
    }
    return pblk.bindex;
}

int
bcm56670_a0_xlport_block_index_get(int unit, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_XLPORT)) {
        return -1 ;
    }
    return pblk.bindex;
}

int
bcm56670_a0_pgw_clport_block_index_get(int unit, int port, int *obm, int *sub_port)
{
    if (port <= 32) {
        *obm = (port - 1) / 4;
        *sub_port = (port - 1) - (*obm) * 4;
        return 0;
    } else {
        port -= 32;
        *obm = (port - 1) / 4;
        *sub_port = (port - 1) - (*obm) * 4;
        return 1;
    }
}

int
bcm56670_a0_pgw_clport_block_number_get(int unit, int port)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_PGW_CL)) {
        return -1 ;
    }
    return pblk.block;
}

int
bcm56670_a0_p2l(int unit, int port, int inverse)
{
    int pp;

    if (inverse) {
        /* L to P */
        if (port == LB_LPORT) {
            return LB_PPORT;
        } else if (port == RDB_LPORT) {
            return RDB_PPORT;
        } else if (port == CMIC_LPORT) {
            return CMIC_PPORT;
        }
    } else {
        /* P to L */
        if (port == LB_PPORT) {
            return LB_LPORT;
        } else if (port == RDB_PPORT) {
            return RDB_LPORT;
        } else if (port == CMIC_PPORT) {
            return CMIC_LPORT;
        }
    }

    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        if (inverse) {
            for (pp = 0; pp < NUM_PHYS_PORTS; pp++) {
                if (port == CDK_PORT_CONFIG_SYS_PORT(unit, pp)) {
                    return pp;
                }
            }
            return -1;
        } else {
            return CDK_PORT_CONFIG_SYS_PORT(unit, port);
        }
    }

    return -1;
}

int
bcm56670_a0_p2m(int unit, int port, int inverse)
{
    int pp;

    if (inverse) {
        if (port == CMIC_MPORT) {
            return CMIC_PPORT;
        } else if (port == LB_MPORT) {
            return LB_PPORT;
        } else if (port == RDB_MPORT) {
            return RDB_PPORT;
        } else {
            for (pp = 0; pp < NUM_PHYS_PORTS; pp++) {
                if (port == _mmu_port_map[unit].map[pp]) {
                    return pp;
                }
            }
        }
        return -1;
    } else {
        if (port == CMIC_PPORT) {
            return CMIC_MPORT;
        } else if (port == LB_PPORT) {
            return LB_MPORT;
        } else if (port == RDB_PPORT) {
            return RDB_MPORT;
        } else {
            return _mmu_port_map[unit].map[port];
        }
    }
}

uint32_t
bcm56670_a0_port_speed_max(int unit, int port)
{
    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        return CDK_PORT_CONFIG_SPEED_MAX(unit, port);
    }

    return 0;
}


int
bcm56670_a0_mmu_port_mc_queues(int unit, int port)
{
    if (port == CMIC_PORT) {
        return 48;
    }
    
    return 10;
}

int
bcm56670_a0_mmu_port_uc_queues(int unit, int port)
{
    int mport = P2M(unit, port);

    if (port == CMIC_PORT || port == LB_PORT || port == RDB_LPORT || mport < 0) {
        return 0 ;
    }

    if (bcm56670_a0_port_in_eq_bmp(unit, port)) {
        return 10;
    }

    return 16;
}

int
bcm56670_a0_mmu_port_uc_queue_index(int unit, int port)
{
    int mport = P2M(unit, port);
    int idx = 0;
    int mport_idx = mport;

    if (bcm56670_a0_port_in_eq_bmp(unit, port)) {
        idx = bcm56670_a0_mmu_port_uc_queues(unit, port) * mport_idx;
    } else {
        idx = bcm56670_a0_mmu_port_uc_queues(unit, port) * mport_idx;
    }

    return idx;

}

int
bcm56670_a0_mc_queue_num(int unit, int port, int cosq)
{
    if (port >= 0 && port < BMD_CONFIG_MAX_PORTS) {
        return _mc_q_base[unit][port] + cosq;
    }
    return -1;
}

int
bcm56670_a0_bmd_attach(int unit)
{
    int port;
    int port_mode;
    uint32_t speed_max;
    cdk_pbmp_t pbmp, pbmp_add;
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_bus_t **phy_bus = bcm56670_phy_bus;
#endif

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_CLPORT, &pbmp_add);
    CDK_PBMP_OR(pbmp, pbmp_add);

    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        speed_max = SPEED_MAX(unit, port);
        if (speed_max == 0) {
            continue;
        }

        if (speed_max >= 100000 && P2L(unit, port) == -1) {
            /* Single port with multi-core PHYs */
            PHY_BUS_SET(unit, port, phy_bus);
            continue;
        }
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;

        port_mode = CDK_PORT_CONFIG_PORT_MODE(unit, port);
        if (port_mode == CDK_DCFG_PORT_MODE_HIGIG ||
            port_mode == CDK_DCFG_PORT_MODE_HIGIG2) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_HG;
        }

        PHY_BUS_SET(unit, port, phy_bus);
    }

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    /* Match default API port map to configured logical ports */
    _init_port_map(unit);
#endif
    /* Initialize MMU port mappings */
    _mmu_ports_assign(unit);

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;
    /* Initialize debug functions */
    BMD_PORT_SPEED_MAX(unit) = bcm56670_a0_port_speed_max;
    BMD_PORT_P2L(unit) = bcm56670_a0_p2l;
    BMD_PORT_P2M(unit) = bcm56670_a0_p2m;
    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    _q_num_config(unit);

    return 0;

}
#endif /* CDK_CONFIG_INCLUDE_BCM56670_A0 */

