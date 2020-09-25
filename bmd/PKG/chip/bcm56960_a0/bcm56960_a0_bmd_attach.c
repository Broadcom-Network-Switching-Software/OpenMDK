/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56960_A0 == 1

#include <bmd/bmd.h>
#include <bmd/bmd_phy_ctrl.h>

#include <cdk/chip/bcm56960_a0_defs.h>
#include <cdk/cdk_debug.h>

#include "bcm56960_a0_bmd.h"
#include "bcm56960_a0_internal.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm56960_phy_bus[] = {
#ifdef PHY_BUS_BCM56960_MIIM_INT_INSTALLED
    &phy_bus_bcm56960_miim_int,
#endif
    NULL
};
#define PHY_BUS_SET(_u,_p,_b) bmd_phy_bus_set(_u,_p,_b)
#else
#define PHY_BUS_SET(_u,_p,_b)
#endif

struct _port_info {
    int port;
    int phy_port;
    int pipe;
    int idb_port;
    int mmu_port;
};

static struct _port_info _cpu_port = { 0, 0, 0, 32, 32 };
static struct _port_info _lb_ports[] = {
    { 33,  132, 0, 33, 33 },    /* loopback port 0 */
    { 67,  133, 1, 33, 97 },    /* loopback port 1 */
    { 101, 134, 2, 33, 161 },   /* loopback port 2 */
    { 135, 135, 3, 33, 225 }    /* loopback port 3 */
};

uint8_t _lb_port_valid[BMD_CONFIG_MAX_UNITS];

#define LB_PORT_VALID_SET(_unit, _idx) (_lb_port_valid[_unit] |= (1 << (_idx)))
#define LB_PORT_VALID_GET(_unit, _idx) ((_lb_port_valid[_unit] >> (_idx)) & 0x1)

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
    cdk_pbmp_t pbmp, pbmp2;
    int lport, lport_max = 0;
    int port;

    CDK_MEMSET(&_port_map[unit], -1, sizeof(_port_map[unit]));

    _port_map[unit].map[0] = CMIC_PORT;

    bcm56960_a0_clport_pbmp_get(unit, &pbmp);
    bcm56960_a0_xlport_pbmp_get(unit, &pbmp2);
    CDK_PBMP_OR(pbmp, pbmp2);
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


/*
 * The MMU port mappings must be derived from the individual
 * port configurations such as maximum speed and queueing
 * capabilities.
 */
static void
_init_mmu_port_map(int unit)
{
    cdk_pbmp_t pbmp, pbmp2;
    int pp, pipe, port, idb_port;


    /* Clear MMU port map */
    CDK_MEMSET(&_mmu_port_map[unit], -1, sizeof(_mmu_port_map[unit]));

    /* Fixed mappings */
    _mmu_port_map[unit].map[_cpu_port.phy_port] = _cpu_port.mmu_port;
    for (pp = 0; pp < COUNTOF(_lb_ports); pp++) {
        if (LB_PORT_VALID_SET(unit, pp)) {
            _mmu_port_map[unit].map[_lb_ports[pp].phy_port] = _lb_ports[pp].mmu_port;
        }
    }

    /* All configured physical ports */
    bcm56960_a0_clport_pbmp_get(unit, &pbmp);
    bcm56960_a0_xlport_pbmp_get(unit, &pbmp2);
    CDK_PBMP_OR(pbmp, pbmp2);

    CDK_PBMP_ITER(pbmp, port) {
        if (port == TH_MGMT_PORT_0) { /* management port 0 */
            _mmu_port_map[unit].map[port] = 96;
        } else if (port == TH_MGMT_PORT_1) { /* management port 1 */
            _mmu_port_map[unit].map[port] = 160;
        } else {
            idb_port = (port - 1) % TH_PORTS_PER_PIPE;
            pipe = (port -1 ) / TH_PORTS_PER_PIPE;
            _mmu_port_map[unit].map[port] =
                (pipe << 6) | ((idb_port & 0x1) << 4) | (idb_port >> 1);
        }
    }
}

int 
bcm56960_a0_num_cosq(int unit, int port)
{
    if ((port < 0) || (port >= NUM_PHYS_PORTS)) {
        return 0;
    }

    if (port == _cpu_port.phy_port) {
        return 48;
    } else {
        return 10;
    }
}

int 
bcm56960_a0_num_cosq_base(int unit, int port)
{
    int mport;

    if ((port < 0) || (port >= NUM_PHYS_PORTS)) {
        return 0;
    }

    mport = P2M(unit, port);
    if (mport < 0) {
        return 0;
    }

    return ((mport & 0x3f) * 10);
}

int 
bcm56960_a0_num_uc_cosq(int unit, int port)
{
    int pp;
    
    if (port <= _cpu_port.phy_port || port >= _lb_ports[0].phy_port) {
        return 0;
    }

    if (port == _cpu_port.phy_port) {
        return 0;
    }
    for (pp = 0; pp < COUNTOF(_lb_ports); pp++) {
        if (port == _lb_ports[pp].phy_port) {
            return 0;
        }
    }

    return 10;
}

int 
bcm56960_a0_num_uc_cosq_base(int unit, int port)
{
    int pp, mport;

    if (port <= _cpu_port.phy_port || port >= _lb_ports[0].phy_port) {
        return 0;
    }

    mport = P2M(unit, port);
    if (mport < 0) {
        return 0;
    }

    if (port == _cpu_port.phy_port) {
        return 0;
    }
    for (pp = 0; pp < COUNTOF(_lb_ports); pp++) {
        if (port == _lb_ports[pp].phy_port) {
            return 0;
        }
    }

    return ((mport & 0x3f) * 10);
}

int
bcm56960_a0_port_ratio_get(int unit, int clport)
{
    int port, phy_port_base, lane;
    int num_lanes[TH_PORTS_PER_PBLK];
    int speed_max[TH_PORTS_PER_PBLK];

    phy_port_base = 1 + clport * TH_PORTS_PER_PBLK;
    for (lane = 0; lane < TH_PORTS_PER_PBLK; lane += 2) {
        port = P2L(unit, phy_port_base + lane);
        if (port == -1 || BMD_PORT_PROPERTIES(unit, port) == BMD_PORT_FLEX) {
            num_lanes[lane] = -1;
            speed_max[lane] = -1;
        } else {
            num_lanes[lane] = bcm56960_a0_port_lanes_get(unit, port);
            speed_max[lane] = bcm56960_a0_port_speed_max(unit, port);
        }
    }

    if (num_lanes[0] == 4) {
        return PORT_RATIO_SINGLE;
    } else if (num_lanes[0] == 2 && num_lanes[2] == 2) {
        if (speed_max[0] == speed_max[2]) {
            return PORT_RATIO_DUAL_1_1;
        } else if (speed_max[0] > speed_max[2]) {
            return PORT_RATIO_DUAL_2_1;
        } else {
            return PORT_RATIO_DUAL_1_2;
        }
    } else if (num_lanes[0] == 2) {
        if (num_lanes[2] == -1) {
            return PORT_RATIO_DUAL_1_1;
        } else {
            return ((speed_max[0] == 20000 || speed_max[0] == 21000 ) ?
                     PORT_RATIO_TRI_023_2_1_1 : PORT_RATIO_TRI_023_4_1_1);
        }
    } else if (num_lanes[2] == 2) {
        if (num_lanes[0] == -1) {
            return PORT_RATIO_DUAL_1_1;
        } else {
            return ((speed_max[2] == 20000 || speed_max[2] == 21000 ) ?
                     PORT_RATIO_TRI_012_1_1_2 : PORT_RATIO_TRI_012_1_1_4);
        }
    } else {
        return PORT_RATIO_QUAD;
    }
}

int
bcm56960_a0_port_lanes_get(int unit, int port)
{
    uint32_t speed_max;

    if ((port < 0) || (port >= NUM_PHYS_PORTS)) {
        return -1;
    }

    speed_max = bcm56960_a0_port_speed_max(unit, port);
    if (speed_max > 53000) {
        return 4;
    } else if ((speed_max == 40000) || (speed_max == 42000)) {
        /* Note: 40G, HG[42] can operate either in dual or quad lane mode 
                 Check if adjacent ports exist to set dual lane mode */
        if ((port % 4) & 1) {
            if (bcm56960_a0_port_speed_max(unit, port ^ 0x2) > 0) {
                return 2;
            }
        }
        /* Else set to quad lane mode by default if the user did not 
           specify anything and no sister ports existed */
        return 4;
    } else if ((speed_max >= 20000) && !(speed_max == 25000 || speed_max == 27000)) {
        /* 50G, HG[53], 20G MLD, HG[21] use 2 lanes */
        return 2;
    } else if (speed_max > 0) {
        /* 10G XFI, HG[11], 25G XFI and HG[27] use 1 lane */
        /* But RXAUI and XAUI on mgmt ports use 2 and 4 lanes, which 
               * can be provided via the portmap interface.
               */
        return 1;
    }

    return -1;
}

int
bcm56960_a0_port_serdes_get(int unit, int port)
{
    int serdes = -1;

    if (port == TH_MGMT_PORT_0 || port == TH_MGMT_PORT_1) {
        serdes = 32;
    } else {
        serdes = (port - 1) / TH_PORTS_PER_PBLK;
    }

    return serdes;    
}

int 
bcm56960_a0_idbport_get(int unit, int port)
{
    int pp, idb_port;

    if ((port < 0) || (port >= NUM_PHYS_PORTS)) {
        return -1;
    }

    /* Fixed mappings */
    if (port == _cpu_port.phy_port) {
        return _cpu_port.idb_port;
    }
    for (pp = 0; pp < COUNTOF(_lb_ports); pp++) {
        if (port == _lb_ports[pp].phy_port) {
            if (LB_PORT_VALID_GET(unit, pp)) {
                return _lb_ports[pp].idb_port;
            } else {
                return -1;
            }
        }
    }

    if (port == TH_MGMT_PORT_0 || port == TH_MGMT_PORT_1) {
        idb_port = 32;
    } else {
        idb_port = (port - 1) % TH_PORTS_PER_PIPE;
    }

    return idb_port;
}

int
bcm56960_a0_oversub_map_get(int unit, cdk_pbmp_t *oversub_map)
{
    int port;
    uint32_t total_bw = 0;
    cdk_pbmp_t pbmp;

    bcm56960_a0_clport_pbmp_get(unit, oversub_map);
    CDK_PBMP_ITER(*oversub_map, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        total_bw += bcm56960_a0_port_speed_max(unit, port);
    }
    if (total_bw > 532500 * 4) {
        bcm56960_a0_xlport_pbmp_get(unit, &pbmp);
        CDK_PBMP_OR(*oversub_map, pbmp);
    } else {
        CDK_PBMP_CLEAR(*oversub_map);
    }

    return 0;
}

int
bcm56960_a0_pipe_map_get(int unit, uint32_t *pipe_map)
{
    int pp;

    *pipe_map = 0;

    for (pp=0; pp < COUNTOF(_lb_ports); pp ++) {
        if (LB_PORT_VALID_GET(unit, pp)) {
            *pipe_map |= (1 << pp);
        }
    }

    return 0;
}

int
bcm56960_a0_port_pipe_get(int unit, int port)
{
    int pp, pipe;

    if ((port < 0) || (port >= NUM_PHYS_PORTS)) {
        return -1;
    }

    /* Fixed mappings */
    if (port == _cpu_port.phy_port) {
        return _cpu_port.pipe;
    }
    for (pp = 0; pp < COUNTOF(_lb_ports); pp++) {
        if (port == _lb_ports[pp].phy_port) {
            if (LB_PORT_VALID_GET(unit, pp)) {
                return _lb_ports[pp].pipe;
            } else {
                return -1;
            }
        }
    }

    if (port == TH_MGMT_PORT_0) {
        pipe = 1;
    } else if (port == TH_MGMT_PORT_1) {
        pipe = 2;
    } else {
        pipe = (port - 1) / TH_PORTS_PER_PIPE;
    }

    return pipe;
}

int
bcm56960_a0_block_port_get(int unit, int port, int blktype)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, blktype)) {
        return -1 ;
    } 
    
    return pblk.bport;
}

int
bcm56960_a0_block_index_get(int unit, int port, int blktype)
{
    cdk_xgsd_pblk_t pblk;

    if (cdk_xgsd_port_block(unit, port, &pblk, blktype)) {
        return -1 ;
    } 
    
    return pblk.bindex;
}


int
bcm56960_a0_eq_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;
    uint32_t speed_max;
    cdk_pbmp_t pbmp2;

    bcm56960_a0_clport_pbmp_get(unit, pbmp);
    bcm56960_a0_xlport_pbmp_get(unit, &pbmp2);
    CDK_PBMP_OR(*pbmp, pbmp2);
    CDK_PBMP_ITER(*pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        speed_max = bcm56960_a0_port_speed_max(unit, port);
        if (speed_max < 40000) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    return 0;
}

int
bcm56960_a0_lpbk_pbmp_get(int unit,cdk_pbmp_t * pbmp)
{
    int idx;

    CDK_PBMP_CLEAR(*pbmp);

    for (idx = 0; idx < TH_PIPES_PER_DEV; idx++) {
        if (LB_PORT_VALID_GET(unit, idx)) {
            CDK_PBMP_ADD(*pbmp, _lb_ports[idx].phy_port);
        }
    }

    return 0;
}

int
bcm56960_a0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
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
bcm56960_a0_clport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
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
bcm56960_a0_all_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    cdk_pbmp_t pbmp2;

    bcm56960_a0_clport_pbmp_get(unit, pbmp);
    bcm56960_a0_xlport_pbmp_get(unit, &pbmp2);
    CDK_PBMP_OR(*pbmp, pbmp2);
    CDK_PBMP_PORT_ADD(*pbmp, CMIC_PORT);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_LBPORT, &pbmp2);
    CDK_PBMP_OR(*pbmp, pbmp2);

    return 0;
}

int
bcm56960_a0_p2l(int unit, int port, int inverse)
{
    int pp;

    if ((port < 0) || (port >= NUM_PHYS_PORTS)) {
        return -1;
    }

    if (inverse) {
        /* Fixed mappings */
        if (port == _cpu_port.port) {
            return _cpu_port.phy_port;
        }
        for (pp = 0; pp < COUNTOF(_lb_ports); pp++) {
            if (LB_PORT_VALID_GET(unit, pp)) {
                if (port == _lb_ports[pp].port) {
                    return _lb_ports[pp].phy_port;
                }
            }
        }
        /* Use per-port config if available */
        if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
            for (pp = 0; pp < NUM_PHYS_PORTS; pp++) {
                if (port == CDK_PORT_CONFIG_SYS_PORT(unit, pp)) {
                    return pp;
                }
            }
        } 
    } else {
        /* Fixed mappings */
        if (port == _cpu_port.phy_port) {
            return _cpu_port.port;
        }
        for (pp = 0; pp < COUNTOF(_lb_ports); pp++) {
            if (LB_PORT_VALID_GET(unit, pp)) {
                if (port == _lb_ports[pp].phy_port) {
                    return _lb_ports[pp].port;
                }
            }
        }
        /* Use per-port config if available */
        if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
            return CDK_PORT_CONFIG_SYS_PORT(unit, port);
        } 
    }

    return -1;
}

int
bcm56960_a0_p2m(int unit, int port, int inverse)
{
    int pp;

    if (port < 0) {
        return -1;
    }

    if (inverse) {
        /* Fixed mappings */
        if (port == _cpu_port.mmu_port) {
            return _cpu_port.phy_port;
        }
        for (pp = 0; pp < COUNTOF(_lb_ports); pp++) {
            if (LB_PORT_VALID_GET(unit, pp)) {
                if (port == _lb_ports[pp].mmu_port) {
                    return _lb_ports[pp].phy_port;
                }
            }
        }

        for (pp =0; pp < NUM_PHYS_PORTS; pp++) {
            if (port == _mmu_port_map[unit].map[pp]) {
                return pp;
            }
        }
    } else {
        /* Fixed mappings */
        if (port == _cpu_port.phy_port) {
            return _cpu_port.mmu_port;
        }
        for (pp = 0; pp < COUNTOF(_lb_ports); pp++) {
            if (LB_PORT_VALID_GET(unit, pp)) {
                if (port == _lb_ports[pp].phy_port) {
                    return _lb_ports[pp].mmu_port;
                }
            }
        }

        return _mmu_port_map[unit].map[port]; 
    }

    return -1;
}

uint32_t
bcm56960_a0_port_speed_max(int unit, int port)
{
    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        return CDK_PORT_CONFIG_SPEED_MAX(unit, port);
    } 
    return 0;
}

int 
bcm56960_a0_bmd_attach(int unit)
{
    int idx, port, port_mode, pipe[TH_PIPES_PER_DEV];
    uint32_t speed_max;
    cdk_pbmp_t pbmp, pbmp2;
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_bus_t **phy_bus = bcm56960_phy_bus;
#endif

    if (!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    for (idx = 0; idx < TH_PIPES_PER_DEV; idx++) {
        pipe[idx] = 0;
    }

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_CLPORT, &pbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp2);
    CDK_PBMP_OR(pbmp, pbmp2);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        speed_max = bcm56960_a0_port_speed_max(unit, port);
        if (speed_max == 0) {
            continue;
        }
        BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_XE;
        port_mode = CDK_PORT_CONFIG_PORT_MODE(unit, port);
        if (port_mode == CDK_DCFG_PORT_MODE_HIGIG ||
            port_mode == CDK_DCFG_PORT_MODE_HIGIG2) {
            BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_HG;
        }
#if BMD_CONFIG_INCLUDE_PHY == 1
        PHY_BUS_SET(unit, port, phy_bus);
#endif        

        idx = bcm56960_a0_port_pipe_get(unit, port);
        if (idx >= 0) {
            pipe[idx] = 1;
        }
    }

    _lb_port_valid[unit] = 0;
    for (idx = 0; idx < TH_PIPES_PER_DEV; idx++) {
        if (pipe[idx] == 1) {
            LB_PORT_VALID_SET(unit, idx);
        }
    }

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    /* Initialize MMU port mappings */
    _init_mmu_port_map(unit);

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    /* Match default API port map to configured logical ports */
    _init_port_map(unit);
#endif

    /* Initialize debug functions */
    BMD_PORT_SPEED_MAX(unit) = bcm56960_a0_port_speed_max;
    BMD_PORT_P2L(unit) = bcm56960_a0_p2l;
    BMD_PORT_P2M(unit) = bcm56960_a0_p2m;
    
    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    return 0;
}
#endif /* CDK_CONFIG_INCLUDE_BCM56960_A0 */

