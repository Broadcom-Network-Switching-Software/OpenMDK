/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __ROBO_CHIP_H__
#define __ROBO_CHIP_H__

#include <cdk/cdk_types.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_chip.h>
#include <cdk/cdk_symbols.h>

/*
 * Block/port information structure
 */
typedef struct cdk_robo_block_s {
    /* Block Type */
    int type; 

    /* Port Bitmaps */
    cdk_pbmp_t pbmps;

} cdk_robo_block_t; 

/*
 * Chip information
 */
typedef struct cdk_robo_chip_info_s {    
    
    /* Block types */
    int nblktypes; 
    const char **blktype_names; 

    /* Offset/Address Vectors */
    uint32_t (*port_addr)(int port, int size, uint32_t offset); 

    /* Block structures */
    int nblocks; 
    const cdk_robo_block_t *blocks; 

    /* Valid ports for this chip */
    cdk_pbmp_t valid_pbmps;

    /* Chip Flags */
    uint32_t flags; 
    
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
    /* Chip Symbol Table Pointer */
    const cdk_symbols_t *symbols; 
#endif

#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    /* Map of physical portnumbers */
    int nports;
    cdk_port_map_port_t *ports;
#endif

} cdk_robo_chip_info_t; 

/*
 * Get the port bitmap for a given block in the device
 */
extern int
cdk_robo_block_pbmp(int unit, int blktype, cdk_pbmp_t *pbmp);

extern uint32_t
cdk_robo_port_addr(int unit, int port, int size, uint32_t offset); 

/*
 * Useful Macros
 * Mostly unused withing the CDK, but provided as a convenience for driver development
 */

#define CDK_ROBO_INFO(unit) ((cdk_robo_chip_info_t *)cdk_device[unit].chip_info)

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#define CDK_ROBO_SYMBOLS(unit) CDK_ROBO_INFO(unit)->symbols
#else
#define CDK_ROBO_SYMBOLS(unit) NULL
#endif

/*
 * Union of bitmaps for all physical blocks of a specific block type
 */
#define CDK_ROBO_BLKTYPE_PBMP_GET(_u, _bt, _pbmp) \
    (cdk_robo_block_pbmp(_u, _bt, _pbmp))

/*
 * Architecture specific initialization functions
 */
extern int
cdk_robo_setup(cdk_dev_t *dev);

/*
 * Architecture specific probe function that extracts chip ID
 * information from the Robo PHY ID registers and optionally
 * retrieves model information from chip-specific register
 *
 * The reg_read functions has same prototype as the read
 * function in the cdk_dev_vectors_t type.
 */
extern int
cdk_robo_probe(void *dvc, cdk_dev_id_t *id,
               int (*reg_read)(void *, uint32_t, uint8_t *, uint32_t));

#endif /* __ROBO_CHIP_H__ */
