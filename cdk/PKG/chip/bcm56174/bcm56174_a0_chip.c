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

#include <cdk/chip/bcm53570_a0_defs.h>

/* Block types */
extern const char *bcm53570_a0_blktype_names[];

/* Block structures */
extern cdk_xgsd_block_t bcm53570_a0_blocks[];

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm53570_a0_dsymbols;
#else
extern cdk_symbols_t bcm53570_a0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85
};
#endif

/* Variable register array info */
extern cdk_xgsd_numel_info_t bcm53570_a0_numel_info;

/* Chip information structure */
static cdk_xgsd_chip_info_t bcm56174_a0_chip_info = {

    /* CMIC block */
    BCM53570_A0_CMIC_BLOCK,

    /* CMC instance */
    0,

    /* Other (non-CMIC) block types */
    15,
    bcm53570_a0_blktype_names,

    /* Address calculation */
    NULL,

    /* Other (non-CMIC) blocks */
    33,
    bcm53570_a0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_3(0xfc03fffd, 0x03ffffff, 0x03fffc00),

    /* Chip flags */
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm53570_a0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm53570_a0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm53570_a0_numel_info,

    /* Configuration dependent memory max index */
    NULL,

    /* Pipe info */
    NULL,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56174_a0_setup(cdk_dev_t *dev);

int
bcm56174_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56174_a0_chip_info;

    return cdk_xgsd_setup(dev);
}

