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

#include <cdk/chip/bcm56504_a0_defs.h>

/* Block types */
extern const char *bcm56504_a0_blktype_names[];

/* Block structures */
extern cdk_xgs_block_t bcm56504_a0_blocks[];

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56504_a0_dsymbols;
#else
extern cdk_symbols_t bcm56504_a0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    28, 24, 25, 26, 27
};
#endif

/* Chip information structure */
static cdk_xgs_chip_info_t bcm56306_a0_chip_info = {

    /* CMIC block */
    BCM56504_A0_CMIC_BLOCK,

    /* Other (non-CMIC) block types */
    9,
    bcm56504_a0_blktype_names,

    /* Address calculation */
    NULL,

    /* Other (non-CMIC) blocks */
    12,
    bcm56504_a0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_1(0x1f000000),

    /* Chip flags */
    CDK_XGS_CHIP_FLAG_CLAUSE45 |
    CDK_XGS_CHIP_FLAG_SCHAN_EXT |
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56504_a0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56504_a0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    NULL,

    /* Configuration dependent memory max index */
    NULL,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56306_a0_setup(cdk_dev_t *dev);

int
bcm56306_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56306_a0_chip_info;

    return cdk_xgs_setup(dev);
}

