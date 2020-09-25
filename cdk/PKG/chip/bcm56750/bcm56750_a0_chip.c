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

#include <cdk/chip/bcm56850_a0_defs.h>

/* Block types */
extern const char *bcm56850_a0_blktype_names[];

/* Block structures */
extern cdk_xgsm_block_t bcm56850_a0_blocks[];

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56850_a0_dsymbols;
#else
extern cdk_symbols_t bcm56850_a0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};
#endif

/* Declare function first to prevent compiler warnings */
extern uint32_t
bcm56850_a0_blockport_addr(int block, int port, uint32_t offset, uint32_t idx);

/* Chip-specific memory sizes */
static uint32_t
bcm56850_a0_mem_maxidx(int enum_val, uint32_t maxidx)
{
    switch (enum_val) {
    case L2Xm_ENUM:
    case L2_ENTRY_ONLYm_ENUM:
        return 32767; /* 0x7fff */
    case L3_ENTRY_IPV4_UNICASTm_ENUM:
    case L3_ENTRY_ONLYm_ENUM:
        return 16383; /* 0x3fff */
    case L2_ENTRY_LPm_ENUM:
    case L2_HITDA_ONLYm_ENUM:
    case L2_HITDA_ONLY_Xm_ENUM:
    case L2_HITDA_ONLY_Ym_ENUM:
    case L2_HITSA_ONLYm_ENUM:
    case L2_HITSA_ONLY_Xm_ENUM:
    case L2_HITSA_ONLY_Ym_ENUM:
    case L3_ENTRY_IPV4_MULTICASTm_ENUM:
    case L3_ENTRY_IPV6_UNICASTm_ENUM:
        return 8191; /* 0x1fff */
    case L3_ENTRY_HIT_ONLYm_ENUM:
    case L3_ENTRY_HIT_ONLY_Xm_ENUM:
    case L3_ENTRY_HIT_ONLY_Ym_ENUM:
    case L3_ENTRY_IPV6_MULTICASTm_ENUM:
    case L3_ENTRY_LPm_ENUM:
        return 4095; /* 0xfff */
    }
    return maxidx;
}

/* Variable register array info */
extern cdk_xgsm_numel_info_t bcm56850_a0_numel_info;

/* Chip information structure */
static cdk_xgsm_chip_info_t bcm56750_a0_chip_info = {

    /* CMIC block */
    BCM56850_A0_CMIC_BLOCK,

    /* CMC instance */
    0,

    /* Other (non-CMIC) block types */
    10,
    bcm56850_a0_blktype_names,

    /* Address calculation */
    bcm56850_a0_blockport_addr,

    /* Other (non-CMIC) blocks */
    56,
    bcm56850_a0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_5(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000047),

    /* Chip flags */
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56850_a0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56850_a0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm56850_a0_numel_info,

    /* Configuration dependent memory max index */
    &bcm56850_a0_mem_maxidx,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56750_a0_setup(cdk_dev_t *dev);

int
bcm56750_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56750_a0_chip_info;

    return cdk_xgsm_setup(dev);
}

