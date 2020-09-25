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

#include <cdk/chip/bcm56634_a0_defs.h>

/* Block types */
const char *bcm56634_a0_blktype_names[] = {
    "cmic",
    "epipe",
    "esm",
    "gport",
    "gxport",
    "ipipe",
    "mmu",
    "sport",
    "xqport"
};

/* Block structures */
cdk_xgs_block_t bcm56634_a0_blocks[] = 
{
    { BCM56634_A0_BLKTYPE_GPORT,        6,      CDK_PBMP_1(0x00003ffc) },
    { BCM56634_A0_BLKTYPE_GPORT,        7,      CDK_PBMP_1(0x03ffc000) },
    { BCM56634_A0_BLKTYPE_GXPORT,       8,      CDK_PBMP_1(0x04000000) },
    { BCM56634_A0_BLKTYPE_GXPORT,       9,      CDK_PBMP_1(0x08000000) },
    { BCM56634_A0_BLKTYPE_GXPORT,       10,     CDK_PBMP_1(0x10000000) },
    { BCM56634_A0_BLKTYPE_GXPORT,       11,     CDK_PBMP_1(0x20000000) },
    { BCM56634_A0_BLKTYPE_SPORT,        12,     CDK_PBMP_1(0x00000002) },
    { BCM56634_A0_BLKTYPE_XQPORT,       16,     CDK_PBMP_2(0xc0000000, 0x00000003) },
    { BCM56634_A0_BLKTYPE_XQPORT,       17,     CDK_PBMP_2(0x00000000, 0x0000003c) },
    { BCM56634_A0_BLKTYPE_XQPORT,       18,     CDK_PBMP_2(0x00000000, 0x000003c0) },
    { BCM56634_A0_BLKTYPE_XQPORT,       19,     CDK_PBMP_2(0x00000000, 0x00003c00) },
    { BCM56634_A0_BLKTYPE_XQPORT,       20,     CDK_PBMP_2(0x00000000, 0x0003c000) },
    { BCM56634_A0_BLKTYPE_XQPORT,       21,     CDK_PBMP_2(0x00000000, 0x003c0000) },
    { BCM56634_A0_BLKTYPE_IPIPE,        0,      CDK_PBMP_2(0xffffffff, 0x007fffff) },
    { BCM56634_A0_BLKTYPE_EPIPE,        1,      CDK_PBMP_2(0xffffffff, 0x007fffff) },
    { BCM56634_A0_BLKTYPE_MMU,          2,      CDK_PBMP_2(0xffffffff, 0x01ffffff) },
    { BCM56634_A0_BLKTYPE_ESM,          4,      CDK_PBMP_2(0xffffffff, 0x007fffff) }
};

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56634_a0_dsymbols;
#else
extern cdk_symbols_t bcm56634_a0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 30, 31, 32, 33, 34, 35, 36, 37, 2, 3, 4, 5, 6, 7, 8, 9, 38, 39, 40, 41, 10, 11, 12, 13, 14, 15, 16, 17, 42, 43, 44, 45, 18, 19, 20, 21, 22, 23, 24, 25, 46, 47, 48, 49, 50, 51, 52, 53, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29
};
#endif

/* Declare function first to prevent compiler warnings */
extern uint32_t
bcm56634_a0_blockport_addr(int block, int port, uint32_t offset);

uint32_t
bcm56634_a0_blockport_addr(int block, int port, uint32_t offset)
{
    if (block & 0x10) {
        block &= 0xf;
        block |= 0x400;
    }
    return ((block * 0x100000) | (port * 0x1000) | (offset & ~0xf00000)); 
}

/* Index ranges for this chip */
static cdk_xgs_numel_range_t _numel_ranges[] = {
    {  0,  0, CDK_PBMP_2(0xffffffff, 0x003fffff)             }, /*  0 */
    {  0,  0, CDK_PBMP_2(0xfffffffe, 0x007fffff)             }, /*  1 */
    {  0,  7, CDK_PBMP_2(0xffffffff, 0x007fffff)             }, /*  2 */
    {  8,  9, CDK_PBMP_2(0xfc000001, 0x004c4cc4)             }, /*  3 */
    { 10, 47, CDK_PBMP_1(0x00000001)                         }, /*  4 */
    {  8, 23, CDK_PBMP_2(0xfc000001, 0x000c4cc4)             }, /*  5 */
    { 24, 47, CDK_PBMP_1(0x00000001)                         }, /*  6 */
    {  0,  0, CDK_PBMP_2(0xfffffffe, 0x003fffff)             }, /*  7 */
    {  0,  0, CDK_PBMP_2(0x00000000, 0x00400000)             }, /*  8 */
    {  8, 25, CDK_PBMP_2(0xfc000001, 0x004c4cc4)             }, /*  9 */
    { 26, 47, CDK_PBMP_1(0x00000001)                         }, /* 10 */
    {  0,  7, CDK_PBMP_2(0x00000000, 0x00400000)             }, /* 11 */
    {  0,  1, CDK_PBMP_2(0xffffffff, 0x01ffffff)             }, /* 12 */
    {  2,  7, CDK_PBMP_2(0x7c000000, 0x00444444)             }, /* 13 */
    {  0,  1, CDK_PBMP_2(0xffffffff, 0x00ffffff)             }, /* 14 */
    {  0,  1, CDK_PBMP_2(0xffffffff, 0x007fffff)             }, /* 15 */
    {  0,  0, CDK_PBMP_2(0x7c000000, 0x00444444)             }, /* 16 */
    {  0,  0, CDK_PBMP_2(0x83ffffff, 0x01bbbbbb)             }, /* 17 */
    {  0,  0, CDK_PBMP_2(0xffffffff, 0x01ffffff)             }, /* 18 */
    {  0,  0, CDK_PBMP_2(0xffffffff, 0x00ffffff)             }, /* 19 */
    {  0,  0, CDK_PBMP_2(0xfc000000, 0x004c4cc4)             }, /* 20 */
    {  0, 15, CDK_PBMP_2(0xfc000000, 0x004c4cc4)             }, /* 21 */
    {  0,  7, CDK_PBMP_2(0xfffffffe, 0x003fffff)             }, /* 22 */
    {  8, 23, CDK_PBMP_2(0xfc000000, 0x000c4cc4)             }  /* 23 */
};

/* Register array encodings for this chip */
static cdk_xgs_numel_encoding_t _numel_encodings[] = {
    { { 19 } },
    { {  0, -1 } },
    { {  1, -1 } },
    { {  2,  3,  4, -1 } },
    { {  2,  5,  6, -1 } },
    { {  7, -1 } },
    { {  8, -1 } },
    { {  2,  9, 10, -1 } },
    { { 11, -1 } },
    { { 12, 13, -1 } },
    { { 14, 13, -1 } },
    { { 15, 13, -1 } },
    { { 16, -1 } },
    { { 17, -1 } },
    { { 18, -1 } },
    { { 19, -1 } },
    { { 20, -1 } },
    { { 21, -1 } },
    { { 22, 23, -1 } }
};

/* Variable register array info */
cdk_xgs_numel_info_t bcm56634_a0_numel_info = {
    _numel_ranges,
    _numel_encodings
};

/* Chip information structure */
static cdk_xgs_chip_info_t bcm56634_a0_chip_info = {

    /* CMIC block */
    BCM56634_A0_CMIC_BLOCK,

    /* Other (non-CMIC) block types */
    9,
    bcm56634_a0_blktype_names,

    /* Address calculation */
    bcm56634_a0_blockport_addr,

    /* Other (non-CMIC) blocks */
    17,
    bcm56634_a0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_2(0xffffffff, 0x01ffffff),

    /* Chip flags */
    CDK_XGS_CHIP_FLAG_CLAUSE45 |
    CDK_XGS_CHIP_FLAG_SCHAN_EXT |
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56634_a0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56634_a0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm56634_a0_numel_info,

    /* Configuration dependent memory max index */
    NULL,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56634_a0_setup(cdk_dev_t *dev);

int
bcm56634_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56634_a0_chip_info;

    return cdk_xgs_setup(dev);
}

