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

#include <cdk/chip/bcm56624_b0_defs.h>

/* Block types */
extern const char *bcm56624_b0_blktype_names[];

/* Block structures */
extern cdk_xgs_block_t bcm56624_b0_blocks[];

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56624_b0_dsymbols;
#else
extern cdk_symbols_t bcm56624_b0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 8, 9, 10, 11, 12, 13, 20, 21, 22, 23, 24, 25, 37, 38, 39, 40, 41, 42, 48, 49, 50, 51, 52, 53, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 14, 26, 27, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 28, 29, 30, 31
};
#endif

/* Variable register array info */
extern cdk_xgs_numel_info_t bcm56624_b0_numel_info;

/* Chip information structure */
static cdk_xgs_chip_info_t bcm56629_b0_chip_info = {

    /* CMIC block */
    BCM56624_B0_CMIC_BLOCK,

    /* Other (non-CMIC) block types */
    8,
    bcm56624_b0_blktype_names,

    /* Address calculation */
    NULL,

    /* Other (non-CMIC) blocks */
    13,
    bcm56624_b0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_2(0xfff07f07, 0x003f07e0),

    /* Chip flags */
    CDK_XGS_CHIP_FLAG_CLAUSE45 |
    CDK_XGS_CHIP_FLAG_SCHAN_EXT |
    BCM56624_B0_CHIP_FLAG_XG01_16G |
    BCM56624_B0_CHIP_FLAG_XG23_16G |
    BCM56624_B0_CHIP_FLAG_XG_MIXED |
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56624_b0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56624_b0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm56624_b0_numel_info,

    /* Configuration dependent memory max index */
    NULL,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56629_b0_setup(cdk_dev_t *dev);

int
bcm56629_b0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56629_b0_chip_info;

    return cdk_xgs_setup(dev);
}

