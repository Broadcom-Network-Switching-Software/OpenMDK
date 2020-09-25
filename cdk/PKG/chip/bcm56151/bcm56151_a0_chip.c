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

#include <cdk/chip/bcm56150_a0_defs.h>

/* Block types */
extern const char *bcm56150_a0_blktype_names[];

/* Block structures */
extern cdk_xgsm_block_t bcm56150_a0_blocks[];

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56150_a0_dsymbols;
#else
extern cdk_symbols_t bcm56150_a0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33
};
#endif

/* Variable register array info */
extern cdk_xgsm_numel_info_t bcm56150_a0_numel_info;

/* Chip information structure */
static cdk_xgsm_chip_info_t bcm56151_a0_chip_info = {

    /* CMIC block */
    BCM56150_A0_CMIC_BLOCK,

    /* CMC instance */
    0,

    /* Other (non-CMIC) block types */
    9,
    bcm56150_a0_blktype_names,

    /* Address calculation */
    NULL,

    /* Other (non-CMIC) blocks */
    11,
    bcm56150_a0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_2(0xfffffffd, 0x00000003),

    /* Chip flags */
    CDK_XGSM_CHIP_FLAG_IPROC |
    BCM56150_A0_CHIP_FLAG_TSC_10G |
    BCM56150_A0_CHIP_FLAG_NO_PHY |
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56150_a0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56150_a0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm56150_a0_numel_info,

    /* Configuration dependent memory max index */
    NULL,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56151_a0_setup(cdk_dev_t *dev);

int
bcm56151_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56151_a0_chip_info;

    return cdk_xgsm_setup(dev);
}

