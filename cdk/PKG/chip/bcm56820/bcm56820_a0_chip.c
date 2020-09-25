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
const char *bcm56820_a0_blktype_names[] = {
    "cmic",
    "epipe",
    "gxport",
    "ipipe",
    "mmu",
    "qgport"
};

/* Block structures */
cdk_xgs_block_t bcm56820_a0_blocks[] = 
{
    { BCM56820_A0_BLKTYPE_GXPORT,       0,      CDK_PBMP_1(0x01fffffe) },
    { BCM56820_A0_BLKTYPE_QGPORT,       2,      CDK_PBMP_1(0x1e000000) },
    { BCM56820_A0_BLKTYPE_IPIPE,        1,      CDK_PBMP_1(0x1fffffff) },
    { BCM56820_A0_BLKTYPE_EPIPE,        4,      CDK_PBMP_1(0x1fffffff) },
    { BCM56820_A0_BLKTYPE_MMU,          13,     CDK_PBMP_1(0x1fffffff) }
};

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
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 25, 26, 27, 28
};
#endif

/* Declare function first to prevent compiler warnings */
extern uint32_t
bcm56820_a0_blockport_addr(int block, int port, uint32_t offset);

uint32_t
bcm56820_a0_blockport_addr(int block, int port, uint32_t offset)
{
    if (block == 0) {
        port += 1;
    } else if (block == 2) {
        port += 25;
    }
    return ((block * 0x100000) | (port * 0x1000) | (offset & ~0xf00000)); 
}

/* Index ranges for this chip */
static cdk_xgs_numel_range_t _numel_ranges[] = {
    {  0,  7, CDK_PBMP_1(0x1ffffffe)                         }, /*  0 */
    {  0,  0, CDK_PBMP_1(0x1ffffffe)                         }, /*  1 */
    {  0,  9, CDK_PBMP_1(0x1fffffff)                         }, /*  2 */
    { 10, 31, CDK_PBMP_1(0x00000001)                         }, /*  3 */
    {  0,  0, CDK_PBMP_1(0x01fffffe)                         }  /*  4 */
};

/* Register array encodings for this chip */
static cdk_xgs_numel_encoding_t _numel_encodings[] = {
    { { 5 } },
    { {  0, -1 } },
    { {  1, -1 } },
    { {  2,  3, -1 } },
    { {  4, -1 } }
};

/* Variable register array info */
cdk_xgs_numel_info_t bcm56820_a0_numel_info = {
    _numel_ranges,
    _numel_encodings
};

/* Chip information structure */
static cdk_xgs_chip_info_t bcm56820_a0_chip_info = {

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
    CDK_PBMP_1(0x1fffffff),

    /* Chip flags */
    CDK_XGS_CHIP_FLAG_CLAUSE45 |
    CDK_XGS_CHIP_FLAG_SCHAN_EXT |
    BCM56820_A0_CHIP_FLAG_TDM1_X |
    BCM56820_A0_CHIP_FLAG_TDM1_Y |
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
bcm56820_a0_setup(cdk_dev_t *dev);

int
bcm56820_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56820_a0_chip_info;

    return cdk_xgs_setup(dev);
}
