/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BCM56640_A0_INTERNAL_H__
#define __BCM56640_A0_INTERNAL_H__

#define XMAC_MODE_SPEED_10      0x0
#define XMAC_MODE_SPEED_100     0x1
#define XMAC_MODE_SPEED_1000    0x2
#define XMAC_MODE_SPEED_2500    0x3
#define XMAC_MODE_SPEED_10000   0x4

#define XMAC_MODE_HDR_IEEE      0x0
#define XMAC_MODE_HDR_HIGIG     0x1
#define XMAC_MODE_HDR_HIGIG2    0x2
#define XMAC_MODE_HDR_CLH       0x3
#define XMAC_MODE_HDR_CSH       0x4

#define CORE_MODE_SINGLE        0
#define CORE_MODE_DUAL          1
#define CORE_MODE_QUAD          2
#define CORE_MODE_NOTDM         3

#define PHY_MODE_SINGLE         0
#define PHY_MODE_DUAL           1
#define PHY_MODE_QUAD           2

#define MAC_MODE_INDEP          0
#define MAC_MODE_AGGR           1

#define NUM_PHYS_PORTS          86
#define NUM_LOGIC_PORTS         63
#define NUM_MMU_PORTS           63

#define CMIC_LPORT              0
#define CMIC_MPORT              59

#define MMU_QBASE_UC            0
#define MMU_QBASE_MC            1024
#define MMU_QBASE_MC_HG         1344
#define MMU_QBASE_MC_GE         1520
#define MMU_QBASE_CPU           1536

extern int
bcm56640_a0_tdm_default(int unit, const int **tdm_seq);

extern uint32_t
bcm56640_a0_port_speed_max(int unit, int port);

extern int
bcm56640_a0_mmu_port_mc_queues(int unit, int port);

extern int
bcm56640_a0_mmu_port_uc_queues(int unit, int port);

extern int
bcm56640_a0_mc_queue_num(int unit, int port, int cosq);

extern int
bcm56640_a0_uc_queue_num(int unit, int port, int cosq);

extern int
bcm56640_a0_unicore_phy_init(int unit, int port);

extern int
bcm56640_a0_warpcore_phy_init(int unit, int port);

extern int
bcm56640_a0_xport_inst(int unit, int port);

extern int
bcm56640_a0_xmac_reset_set(int unit, int port, int reset);

extern int
bcm56640_a0_xport_reset(int unit, int port);

extern int
bcm56640_a0_xport_init(int unit, int port);

extern int
bcm56640_a0_xport_pbmp_get(int unit, cdk_pbmp_t *pbmp);

extern int
bcm56640_a0_p2l(int unit, int port, int inverse);

extern int
bcm56640_a0_p2m(int unit, int port, int inverse);

#define P2L(_u,_p) bcm56640_a0_p2l(_u,_p,0)
#define L2P(_u,_p) bcm56640_a0_p2l(_u,_p,1)

#define P2M(_u,_p) bcm56640_a0_p2m(_u,_p,0)
#define M2P(_u,_p) bcm56640_a0_p2m(_u,_p,1)

#define XPORT_BLKIDX(_p) ((_p - 1) >> 2)
#define XPORT_SUBPORT(_p) ((_p - 1) & 0x3)

#endif /* __BCM56640_A0_INTERNAL_H__ */
