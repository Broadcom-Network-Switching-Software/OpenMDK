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

#include <cdk/chip/bcm56820_a0_defs.h>

/* Block types */
extern const char *bcm56820_a0_blktype_names[];

/* Block structures */
extern cdk_xgs_block_t bcm56820_a0_blocks[];

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56820_a0_dsymbols;
#else
extern cdk_symbols_t bcm56820_a0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 25, 26, 27, 28
};
#endif

/* Declare function first to prevent compiler warnings */
extern uint32_t
bcm56820_a0_blockport_addr(int block, int port, uint32_t offset);

/* Variable register array info */
extern cdk_xgs_numel_info_t bcm56820_a0_numel_info;

/* Chip information structure */
static cdk_xgs_chip_info_t bcm56823_a0_chip_info = {

    /* CMIC block */
    BCM56820_A0_CMIC_BLOCK,

    /* Other (non-CMIC) block types */
    6,
    bcm56820_a0_blktype_names,

    /* Address calculation */
    bcm56820_a0_blockport_addr,

    /* Other (non-CMIC) blocks */
    5,
    bcm56820_a0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_1(0x1e07ff81),

    /* Chip flags */
    CDK_XGS_CHIP_FLAG_CLAUSE45 |
    CDK_XGS_CHIP_FLAG_SCHAN_EXT |
    BCM56820_A0_CHIP_FLAG_TDM3_X |
    BCM56820_A0_CHIP_FLAG_TDM3_Y |
    BCM56820_A0_CHIP_FLAG_CLK_245 |
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56820_a0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56820_a0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm56820_a0_numel_info,

    /* Configuration dependent memory max index */
    NULL,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56823_a0_setup(cdk_dev_t *dev);

int
bcm56823_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56823_a0_chip_info;

    return cdk_xgs_setup(dev);
}

