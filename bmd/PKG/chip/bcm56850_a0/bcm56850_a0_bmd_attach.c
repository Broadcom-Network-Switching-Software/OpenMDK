#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56850_A0 == 1

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

#include <cdk/chip/bcm56850_a0_defs.h>
#include <cdk/arch/xgsm_chip.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_assert.h>

#include "bcm56850_a0_bmd.h"
#include "bcm56850_a0_internal.h"

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy_buslist.h>

static phy_bus_t *bcm56850_phy_bus[] = {
#ifdef PHY_BUS_BCM56850_MIIM_INT_INSTALLED
    &phy_bus_bcm56850_miim_int,
#endif
#ifdef PHY_BUS_BCM956850K_MIIM_EXT_INSTALLED
    &phy_bus_bcm956850k_miim_ext,
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

    bcm56850_a0_xlport_pbmp_get(unit, &pbmp);
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
    cdk_pbmp_t pbmp;
    int port_count[2];
    int idx, pipe, pgw, port, base_port, mmu_port;

    /* All configured physical ports */
    bcm56850_a0_xlport_pbmp_get(unit, &pbmp);

    /* Clear MMU port map */
    CDK_MEMSET(&_mmu_port_map[unit], -1, sizeof(_mmu_port_map[unit]));

    /* Count ports in each pipe */
    port_count[0] = port_count[1] = 0;
    CDK_PBMP_ITER(pbmp, port) {
        pipe = PORT_IN_Y_PIPE(port) ? 1 : 0;
        port_count[pipe]++;
    }
    /* Sanity check */
    for (pipe = 0; pipe < 2; pipe++) {
        if (port_count[pipe] > NUM_MMU_PORTS>>1) {
            CDK_WARN(("bcm56850_a0_bmd_attach[%d]: MMU map error (%d %d)\n",
                      unit, pipe, port_count[pipe]));
        }
    }

    /* Assign MMU port */
    for (pipe = 0; pipe < PIPES_PER_DEV; pipe++) {
        mmu_port = pipe * PORTS_PER_PIPE;
        base_port = mmu_port + 1;
        /* First assign the lowest MMU port number for 100+G ports */
        for (pgw = 0; pgw < PGWS_PER_PIPE; pgw++) {
            port = base_port + (pgw * PORTS_PER_PGW) + ((pgw & 1) ? 20 : 0);
            if (bcm56850_a0_port_speed_max(unit, port) > 42000) {
                _mmu_port_map[unit].map[port] = mmu_port;
                mmu_port++;
            }
        }
        /* Then assign the next lowest MMU port number for 40+G ports */
        for (idx = 0; idx < PORTS_PER_PIPE; idx += 4) {
            port = base_port + idx;
            if (_mmu_port_map[unit].map[port] == -1 &&
                bcm56850_a0_port_speed_max(unit, port) > 20000) {
                _mmu_port_map[unit].map[port] = mmu_port;
                mmu_port++;
            }
        }
        /* Then assign the next lowest MMU port number for 20+G ports */
        for (idx = 0; idx < PORTS_PER_PIPE; idx += 2) {
            port = base_port + idx;
            if (_mmu_port_map[unit].map[port] == -1 &&
                bcm56850_a0_port_speed_max(unit, port) > 10000) {
                _mmu_port_map[unit].map[port] = mmu_port;
                mmu_port++;
            }
        }
        /* Finally assign MMU port number for all other ports */
        for (idx = 0; idx < PORTS_PER_PIPE; idx++) {
            port = base_port + idx;
            if (_mmu_port_map[unit].map[port] == -1 &&
                bcm56850_a0_port_speed_max(unit, port) > 0) {
                _mmu_port_map[unit].map[port] = mmu_port;
                mmu_port++;
            }
        }
    }
}

static int
_q_num_config(int unit)
{
    cdk_pbmp_t pbmp;
    int port;
    int uc_base = 0, mc_base = 0;

    CDK_MEMSET(&_mc_q_base[unit], -1, BMD_CONFIG_MAX_PORTS * sizeof(int));
    CDK_MEMSET(&_mc_q_base[unit], -1, BMD_CONFIG_MAX_PORTS * sizeof(int));

    _mc_q_base[unit][CMIC_PORT] = 520;
    _mc_q_base[unit][LB_PORT] = 1088;
    _uc_q_base[unit][CMIC_PORT] = 0;
    _uc_q_base[unit][LB_PORT] = 0;

    bcm56850_a0_xlport_pbmp_get(unit, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        if (port == (NUM_PHYS_PORTS>>1)) {
            uc_base = 1480;
            mc_base = 568;
        }
        _mc_q_base[unit][port] = mc_base;
        mc_base += bcm56850_a0_mmu_port_mc_queues(unit, port); 
        _uc_q_base[unit][port] = uc_base;
        uc_base += bcm56850_a0_mmu_port_uc_queues(unit, port);
    }

    return 0;
}

int
bcm56850_a0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    int port;

    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, pbmp);
    CDK_PBMP_ITER(*pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            CDK_PBMP_PORT_REMOVE(*pbmp, port);
        }
    }
    return 0;
}

int
bcm56850_a0_all_pbmp_get(int unit, cdk_pbmp_t *pbmp)
{
    bcm56850_a0_xlport_pbmp_get(unit, pbmp);
    CDK_PBMP_PORT_ADD(*pbmp, CMIC_PORT);
    CDK_PBMP_PORT_ADD(*pbmp, LB_PORT);

    return 0;
}

int
bcm56850_a0_p2l(int unit, int port, int inverse)
{
    int pp;

    /* Fixed mappings */
    if (port == CMIC_PORT) {
        return CMIC_LPORT;
    }
    if (port == LB_PORT) {
        return LB_LPORT;
    }
    if (inverse && port == LB_LPORT) {
        return LB_PORT;
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
bcm56850_a0_p2m(int unit, int port, int inverse)
{
    int pp;

    if (inverse) {
        if (port == CMIC_MPORT) {
            return CMIC_PORT;
        } else if (port == LB_MPORT) {
            return LB_PORT;
        } else {
            for (pp = 0; pp < NUM_PHYS_PORTS; pp++) {
                if (port == _mmu_port_map[unit].map[pp]) {
                    return pp;
                }
            }
        }
        return -1;
    } else {
        if (port == CMIC_PORT) {
            return CMIC_MPORT;
        } else if (port == LB_PORT) {
            return LB_MPORT;
        } else {
            return _mmu_port_map[unit].map[port];
        }
    }
}

uint32_t
bcm56850_a0_port_speed_max(int unit, int port)
{
    /* Use per-port config if available */
    if (CDK_NUM_PORT_CONFIGS(unit) != 0) {
        return CDK_PORT_CONFIG_SPEED_MAX(unit, port);
    } 
    return 0;
}

int
bcm56850_a0_mmu_port_mc_queues(int unit, int port)
{
    int mport = P2M(unit, port);

    if (port == CMIC_PORT) {
        return 48;
    } else if (port == LB_PORT) {
        return 8;
    } else {
        return (mport < 0) ? 0 : 10;
    }
}

int
bcm56850_a0_mmu_port_uc_queues(int unit, int port)
{
    int mport = P2M(unit, port);
    
    if (port == CMIC_PORT || port == LB_PORT || mport < 0) {
        return 0 ;
    }
    
    return 12; /* (10 + 3) & ~0x3) */
}

int
bcm56850_a0_mc_queue_num(int unit, int port, int cosq)
{
    if (port >= 0 && port < BMD_CONFIG_MAX_PORTS) {
        return _mc_q_base[unit][port] + cosq;
    }
    return -1;
}

int
bcm56850_a0_uc_queue_num(int unit, int port, int cosq)
{
    if (port >= 0 && port < BMD_CONFIG_MAX_PORTS) {
        return _uc_q_base[unit][port] + cosq;
    }
    return -1;
}

int
bcm56850_a0_bmd_attach(int unit)
{
    int port;
    int port_mode;
    uint32_t speed_max;
    cdk_pbmp_t pbmp;
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_bus_t **phy_bus = bcm56850_phy_bus;
#endif

    if(!CDK_DEV_EXISTS(unit)) {
        return CDK_E_UNIT;
    }

    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
        speed_max = bcm56850_a0_port_speed_max(unit, port);
        if (speed_max == 0) {
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

    /* Initialize MMU port mappings */
    _init_mmu_port_map(unit);

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    /* Match default API port map to configured logical ports */
    _init_port_map(unit);
#endif

    port = CMIC_PORT;
    CDK_ASSERT(port < BMD_CONFIG_MAX_PORTS);
    BMD_PORT_PROPERTIES(unit, port) = BMD_PORT_CPU;

    /* Initialize debug functions */
    BMD_PORT_SPEED_MAX(unit) = bcm56850_a0_port_speed_max;
    BMD_PORT_P2L(unit) = bcm56850_a0_p2l;
    BMD_PORT_P2M(unit) = bcm56850_a0_p2m;

    BMD_DEV_FLAGS(unit) |= BMD_DEV_ATTACHED;

    /* Configure unicast/multicast queues for linked list scheduler (LLS) */
    _q_num_config(unit);

    return 0; 
}
#endif /* CDK_CONFIG_INCLUDE_BCM56850_A0 */
