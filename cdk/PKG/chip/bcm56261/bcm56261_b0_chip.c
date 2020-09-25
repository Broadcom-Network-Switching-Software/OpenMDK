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

#include <cdk/chip/bcm56260_b0_defs.h>

/* Block types */
extern const char *bcm56260_b0_blktype_names[];

/* Block structures */
extern cdk_xgsd_block_t bcm56260_b0_blocks[];

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56260_b0_dsymbols;
#else
extern cdk_symbols_t bcm56260_b0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 21, 22, 23, 24, 25, 26, 27, 28
};
#endif

/* Variable register array info */
extern cdk_xgsd_numel_info_t bcm56260_b0_numel_info;

/* Chip information structure */
static cdk_xgsd_chip_info_t bcm56261_b0_chip_info = {

    /* CMIC block */
    BCM56260_B0_CMIC_BLOCK,

    /* CMC instance */
    0,

    /* Other (non-CMIC) block types */
    16,
    bcm56260_b0_blktype_names,

    /* Address calculation */
    NULL,

    /* Other (non-CMIC) blocks */
    21,
    bcm56260_b0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_1(0x1fe001ff),

    /* Chip flags */
    CDK_XGSD_CHIP_FLAG_IPROC |
    BCM56260_B0_CHIP_FLAG_ACCESS |
    BCM56260_B0_CHIP_FLAG_FREQ118 |
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56260_b0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56260_b0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm56260_b0_numel_info,

    /* Configuration dependent memory max index */
    NULL,

    /* Pipe info */
    NULL,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56261_b0_setup(cdk_dev_t *dev);

int
bcm56261_b0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56261_b0_chip_info;

    return cdk_xgsd_setup(dev);
}

