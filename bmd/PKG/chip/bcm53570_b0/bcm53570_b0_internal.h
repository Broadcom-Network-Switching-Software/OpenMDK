/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM53570_B0_INTERNAL_H__
#define __BCM53570_B0_INTERNAL_H__

#define MAX_PHY_PORTS            90
#define MAX_LOG_PORTS            66
#define MAX_MMU_PORTS        MAX_LOG_PORTS
#define MIN_LOG_PORTS            2

#define MIN_TSCQ_PHY_PORT        26
#define MAX_TSCQ_PHY_PORT        57
#define MIN_TSCX_PHY_PORT        58
#define MIN_TSCF_PHY_PORT        86

/* MMU port index 58~65 is with 64 COSQ */
#define MMU_64Q_PORT_IDX_MIN 58
#define MMU_64Q_PORT_IDX_MAX 65

#define LEGACY_QUEUE_NUM 8

#define QGROUP_PER_PORT_NUM 8
#define QLAYER_COSQ_PER_QGROUP_NUM 8
#define QLAYER_COSQ_PER_PORT_NUM (QGROUP_PER_PORT_NUM * \
                                          QLAYER_COSQ_PER_QGROUP_NUM)
                                          
#define TSC_MAX_BLK_COUNT               7
#define QTC_MAX_BLK_COUNT               2
#define PORT_COUNT_PER_TSC              4
#define PORT_COUNT_PER_QTC              16

#define COMMAND_CONFIG_SPEED_10         0x0
#define COMMAND_CONFIG_SPEED_100        0x1
#define COMMAND_CONFIG_SPEED_1000       0x2
#define COMMAND_CONFIG_SPEED_2500       0x3
#define COMMAND_CONFIG_SPEED_10000      0x4

#define XPORT_MODE_QUAD                 0x0
#define XPORT_MODE_TRI_012              0x1
#define XPORT_MODE_TRI_023              0x2
#define XPORT_MODE_DUAL                 0x3
#define XPORT_MODE_SINGLE               0x4


#define XPORT_FLAG_GXPORT       (1 << BLKTYPE_GXPORT)
#define XPORT_FLAG_XLPORT       (1 << BLKTYPE_XLPORT)
#define XPORT_FLAG_ALL          (XPORT_FLAG_GXPORT | XPORT_FLAG_XLPORT)

#define NUM_PHYS_PORTS                  38
#define MAX_TSC_COUNT                   6

#define PORT_CONFIG_MASK        0xFFFF0000
#define OPT_START_BIT           4

/* TDM related */
#define NUM_EXT_PORTS                       136

extern int
bcm53570_b0_p2l(int unit, int port, int inverse);

extern int
bcm53570_b0_p2m(int unit, int port, int inverse);

extern int
bcm53570_b0_gport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm53570_b0_tscq_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm53570_b0_xlport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm53570_b0_clport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm53570_b0_all_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern uint32_t
bcm53570_b0_port_speed_max(int unit, int port);

extern int
bcm53570_b0_port_is_xlport(int unit, int port);

extern int
bcm53570_b0_port_is_clport(int unit, int port);

extern int
bcm53570_b0_subport_get(int unit, int port);

#define P2L(_u,_p) bcm53570_b0_p2l(_u,_p,0)
#define L2P(_u,_p) bcm53570_b0_p2l(_u,_p,1)
#define P2M(_u,_p) bcm53570_b0_p2m(_u,_p,0)
#define M2P(_u,_p) bcm53570_b0_p2m(_u,_p,1)

extern int
bcm53570_b0_block_disable(int unit, uint32_t disable_tsc, uint32_t disable_qtc);

extern int
bcm53570_b0_xlport_reset(int unit, int port);

extern int
bcm53570_b0_xlport_init(int unit, int port);

extern int
bcm53570_b0_clport_init(int unit, int port);

extern int
bcm53570_b0_xlblock_number_get(int unit, int port);

extern int
bcm53570_b0_xlblock_subport_get(int unit, int port);

extern int
bcm53570_b0_blk_port_get(int unit, int port);

extern int
bcm53570_b0_port_num_lanes(int unit, int port);

extern int
bcm53570_b0_port_mode_get(int unit, int port);

extern int
bcm53570_b0_port_is_tscq(int port);

extern int
bcm53570_b0_block_index_get(int unit, int blk_type, int port);

extern int
bcm53570_b0_xlport_init(int unit, int port);

extern int
bcm53570_b0_clport_init(int unit, int port);

extern int 
bcm53570_b0_cl_port_credit_reset(int unit, int port);

extern int
bcm53570_b0_freq_set(int unit);

extern void *
bcm53570_b0_tdm_malloc(int unit, int size);

extern void
bcm53570_b0_tdm_free(int unit, void *addr);

#define SUBPORT(_u,_p) bcm53570_b0_subport_get(_u,_p)
#define BLK_PORT(_u,_p) bcm53570_b0_blk_port_get(_u,_p)
#define BLKIDX(_u,_b,_p) bcm53570_b0_block_index_get(_u,_b,_p)
#define IS_TSCQ(_p) bcm53570_b0_port_is_tscq(_p)
#define IS_XL(_u,_p) bcm53570_b0_port_is_xlport(_u,_p)
#define IS_CL(_u,_p) bcm53570_b0_port_is_clport(_u,_p)
#define SPEED_MAX(_u,_p) bcm53570_b0_port_speed_max(_u,_p)
#define SOC_PORT_MODE(_u,_p) bcm53570_b0_port_mode_get(_u,_p)
#define FREQ(_u) bcm53570_b0_freq_set(_u)
#define LANES(_u,_p) bcm53570_b0_port_num_lanes(_u,_p)

/* For manipulating port bitmap memory fields */
#define PBM_PORT_WORDS          ((MAX_PHY_PORTS / 32) + 1)
#define PBM_LPORT_WORDS         ((MAX_LOG_PORTS / 32) + 1)
#define PBM_MEMBER(_pbm, _port) \
        ((_pbm)[(_port) >> 5] & LSHIFT32(1, (_port) & 0x1f))
#define PBM_PORT_ADD(_pbm, _port) \
        ((_pbm)[(_port) >> 5] |= LSHIFT32(1, (_port) & 0x1f))
#define PBM_PORT_REMOVE(_pbm, _port) \
        ((_pbm)[(_port) >> 5] &= ~(LSHIFT32(1, (_port) & 0x1f)))

#endif /* __BCM53570_B0_INTERNAL_H__ */
