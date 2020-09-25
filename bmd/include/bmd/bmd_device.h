/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BMD_DEVICE_H__
#define __BMD_DEVICE_H__

#include <bmd_config.h>
#include <bmd/bmd_phy.h>

#include <cdk/cdk_chip.h>

/* Port properties */
#define BMD_PORT_FE             0x1
#define BMD_PORT_GE             0x2
#define BMD_PORT_XE             0x4
#define BMD_PORT_HG             0x8
#define BMD_PORT_CPU            0x10
#define BMD_PORT_ST             0x20
#define BMD_PORT_SCH            0x40
#define BMD_PORT_ASSP           0x80
#define BMD_PORT_FLEX           0x100

#define BMD_PORT_ENET           (BMD_PORT_FE | BMD_PORT_GE | BMD_PORT_XE)
#define BMD_PORT_ALL            (BMD_PORT_ENET | BMD_PORT_HG | BMD_PORT_FLEX)

/* Port status */
#define BMD_PST_DISABLED        0x1
#define BMD_PST_AN_DONE         0x2
#define BMD_PST_LINK_UP         0x4
#define BMD_PST_FORCE_LINK      0x8
#define BMD_PST_FORCE_UPDATE    0x80000000

typedef struct bmd_port_info_s {
    uint32_t properties;
    uint32_t status;
} bmd_port_info_t;

/* Device flags */
#define BMD_DEV_ATTACHED        0x1

/* Maps a port from one space to another (or back if inverse=1) */
typedef int (*bmd_port_map_func_t)(int unit, int port, int inverse);

/* Get a port configuration parameter */
typedef uint32_t (*bmd_port_param_get_func_t)(int unit, int port);

typedef struct bmd_dev_s {

    /* BMD device flags */
    uint32_t flags;

    /* BMD port information */
    bmd_port_info_t port_info[BMD_CONFIG_MAX_PORTS];

    /* Get maximum speed for port (for debug only) */
    bmd_port_param_get_func_t port_speed_max;

    /* Maps physical port to logical port (for debug only) */
    bmd_port_map_func_t port_p2l;

    /* Maps physical port to MMU port (for debug only) */
    bmd_port_map_func_t port_p2m;

} bmd_dev_t;

#define BMD_DEV(_u) bmd_dev[_u]

#define BMD_DEV_FLAGS(_u) BMD_DEV(_u).flags
#define BMD_PORT_SPEED_MAX(_u) BMD_DEV(_u).port_speed_max
#define BMD_PORT_P2L(_u) BMD_DEV(_u).port_p2l
#define BMD_PORT_P2M(_u) BMD_DEV(_u).port_p2m
#define BMD_PORT_INFO(_u,_p) BMD_DEV(_u).port_info[_p]
#define BMD_PORT_PROPERTIES(_u,_p) BMD_PORT_INFO(_u,_p).properties
#define BMD_PORT_STATUS(_u,_p) BMD_PORT_INFO(_u,_p).status
#define BMD_PORT_STATUS_SET(_u,_p,_bits) BMD_PORT_STATUS(_u,_p) |= (_bits)
#define BMD_PORT_STATUS_CLR(_u,_p,_bits) BMD_PORT_STATUS(_u,_p) &= ~(_bits)

#define BMD_DEV_VALID(_u) (BMD_DEV(_u).flags & BMD_DEV_ATTACHED)
#define BMD_PORT_VALID(_u,_p) \
    (_p >= 0 && _p < BMD_CONFIG_MAX_PORTS && BMD_PORT_PROPERTIES(_u,_p) != 0)

#define BMD_PORT_ITER(_u,type,iter) \
    for ((iter) = 0; (iter) < BMD_CONFIG_MAX_PORTS; (iter++)) \
        if ((BMD_PORT_INFO(_u,iter).properties) & (type)) 

#define BMD_CHECK_UNIT(_u) \
    if (!BMD_DEV_VALID(_u)) return CDK_E_UNIT;

#define BMD_CHECK_PORT(_u,_p) \
    if (!BMD_PORT_VALID(_u,_p)) return CDK_E_PORT;

#define BMD_CHECK_VLAN(_u,_v) \
    if (_v <= 0 || _v > 4095) return CDK_E_PARAM;


extern bmd_dev_t bmd_dev[BMD_CONFIG_MAX_UNITS];

extern int
bmd_port_type_pbmp(int unit, uint32_t port_type, cdk_pbmp_t *pbmp);

#define BMD_MODID(_u) ((_u) + 1)
#endif /* __BMD_DEVICE_H__ */
