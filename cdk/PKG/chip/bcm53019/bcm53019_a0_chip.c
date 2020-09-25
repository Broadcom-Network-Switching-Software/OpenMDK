/*******************************************************************************
 *
 * DO NOT EDIT THIS FILE!
 * This file is auto-generated from the registers file.
 * Edits to this file will be lost when it is regenerated.
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/chip/bcm53010_a0_defs.h>

/* Block types */
extern const char *bcm53010_a0_blktype_names[];

/* Block structures */
extern cdk_robo_block_t bcm53010_a0_blocks[];

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm53010_a0_dsymbols;
#else
extern cdk_symbols_t bcm53010_a0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    8, 0, 1, 2, 3, 4, 5, 7
};
#endif

/* Declare function first to prevent compiler warnings */
extern uint32_t
bcm53010_a0_blockport_addr(int port, int size, uint32_t offset);

/* Chip information structure */
static cdk_robo_chip_info_t bcm53019_a0_chip_info = {

    /* Block types */
    5,
    bcm53010_a0_blktype_names,

    /* Address calculation */
    bcm53010_a0_blockport_addr,

    /* Chip blocks */
    5,
    bcm53010_a0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_1(0x000001bf),

    /* Chip flags */
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    &bcm53010_a0_dsymbols,
#else
    &bcm53010_a0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

};

/* Declare function first to prevent compiler warnings */
extern int
bcm53019_a0_setup(cdk_dev_t *dev);

int
bcm53019_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm53019_a0_chip_info;

    return cdk_robo_setup(dev);
}

