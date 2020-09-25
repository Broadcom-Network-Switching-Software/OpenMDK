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

#include <cdk/chip/bcm56218_a0_defs.h>

/* Block types */
extern const char *bcm56218_a0_blktype_names[];

/* Block structures */
extern cdk_xgs_block_t bcm56218_a0_blocks[];

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56218_a0_dsymbols;
#else
extern cdk_symbols_t bcm56218_a0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 4, 5, 3
};
#endif

/* Chip-specific memory sizes */
static uint32_t
bcm56218_a0_mem_maxidx(uint32_t addr, uint32_t maxidx)
{
    switch (addr) {
    case FP_COUNTER_TABLEm:
    case FP_METER_TABLEm:
    case FP_POLICY_TABLEm:
    case FP_TCAMm:
        return 511; /* 0x1ff */
    }
    return maxidx;
}

/* Chip information structure */
static cdk_xgs_chip_info_t bcm53714_a0_chip_info = {

    /* CMIC block */
    BCM56218_A0_CMIC_BLOCK,

    /* Other (non-CMIC) block types */
    8,
    bcm56218_a0_blktype_names,

    /* Address calculation */
    NULL,

    /* Other (non-CMIC) blocks */
    11,
    bcm56218_a0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_1(0x3ffffff9),

    /* Chip flags */
    CDK_XGS_CHIP_FLAG_CLAUSE45 |
    CDK_XGS_CHIP_FLAG_SCHAN_EXT |
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56218_a0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56218_a0_symbols,
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
    &bcm56218_a0_mem_maxidx,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm53714_a0_setup(cdk_dev_t *dev);

int
bcm53714_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm53714_a0_chip_info;

    return cdk_xgs_setup(dev);
}

