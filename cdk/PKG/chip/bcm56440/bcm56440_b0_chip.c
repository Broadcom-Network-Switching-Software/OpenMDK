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

#include <cdk/chip/bcm56440_b0_defs.h>

/* Block types */
const char *bcm56440_b0_blktype_names[] = {
    "ces",
    "ci",
    "cmic",
    "epipe",
    "gport",
    "ipipe",
    "lls",
    "mmu",
    "mxqport",
    "top"
};

/* Block structures */
cdk_xgsm_block_t bcm56440_b0_blocks[] = 
{
    { BCM56440_B0_BLKTYPE_GPORT,         6,   0,   CDK_PBMP_1(0x000001fe) },
    { BCM56440_B0_BLKTYPE_GPORT,         7,   0,   CDK_PBMP_1(0x0001fe00) },
    { BCM56440_B0_BLKTYPE_GPORT,         8,   0,   CDK_PBMP_1(0x01fe0000) },
    { BCM56440_B0_BLKTYPE_MXQPORT,       9,   0,   CDK_PBMP_1(0x02000000) },
    { BCM56440_B0_BLKTYPE_MXQPORT,      10,   0,   CDK_PBMP_1(0x04000000) },
    { BCM56440_B0_BLKTYPE_MXQPORT,      11,   0,   CDK_PBMP_2(0x08000000, 0x00000007) },
    { BCM56440_B0_BLKTYPE_MXQPORT,      12,   0,   CDK_PBMP_1(0xf0000000) },
    { BCM56440_B0_BLKTYPE_IPIPE,         1,   0,   CDK_PBMP_2(0xffffffff, 0x0000007f) },
    { BCM56440_B0_BLKTYPE_EPIPE,         2,   0,   CDK_PBMP_2(0xffffffff, 0x0000007f) },
    { BCM56440_B0_BLKTYPE_MMU,           3,   0,   CDK_PBMP_2(0xffffffff, 0x0000007f) },
    { BCM56440_B0_BLKTYPE_TOP,          13,   0,   CDK_PBMP_2(0xffffffff, 0x0000007f) },
    { BCM56440_B0_BLKTYPE_LLS,          14,   0,   CDK_PBMP_2(0xffffffff, 0x0000007f) },
    { BCM56440_B0_BLKTYPE_CI,           15,   0,   CDK_PBMP_2(0xffffffff, 0x0000007f) },
    { BCM56440_B0_BLKTYPE_CI,           16,   0,   CDK_PBMP_2(0xffffffff, 0x0000007f) },
    { BCM56440_B0_BLKTYPE_CI,           17,   0,   CDK_PBMP_2(0xffffffff, 0x0000007f) },
    { BCM56440_B0_BLKTYPE_CES,          18,   0,   CDK_PBMP_2(0xffffffff, 0x0000007f) }
};

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56440_b0_dsymbols;
#else
extern cdk_symbols_t bcm56440_b0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28
};
#endif

/* Declare function first to prevent compiler warnings */
extern uint32_t
bcm56440_b0_blockport_addr(int block, int port, uint32_t offset, uint32_t idx);

uint32_t
bcm56440_b0_blockport_addr(int block, int port, uint32_t offset, uint32_t idx)
{
    if (block & 0x10) {
        block &= 0xf;
        block |= 0x400;
    }
    if (port < 0) {
        /* Memory calculation */
        offset = ((offset & 0xfc000000) >> 2) | (offset & 0xfffff);
        return ((block * 0x100000) | (offset + idx)); 
    }
    /* Register calculation */
    offset >>= 2;
    if (offset & 0x00800000) {
        offset |= 0x00080000;
    }
    offset = (offset & 0x3f080000) | ((offset >> 6) & 0xfff);
    return ((block * 0x100000) | (port * 0x1000) | (offset + idx)); 
}

/* Index ranges for this chip */
static cdk_xgsm_numel_range_t _numel_ranges[] = {
    {  0,  0, CDK_PBMP_2(0xfffffffe, 0x0000000f)             }, /*  0 */
    {  0,  0, CDK_PBMP_2(0xffffffff, 0x0000000f)             }, /*  1 */
    {  0,  0, CDK_PBMP_2(0xffffffff, 0x0000007f)             }, /*  2 */
    {  0,  0, CDK_PBMP_1(0xfe000001)                         }, /*  3 */
    {  0,  0, CDK_PBMP_1(0x1e000000)                         }, /*  4 */
    {  1,  7, CDK_PBMP_1(0x1e000000)                         }, /*  5 */
    {  0,  7, CDK_PBMP_2(0xffffffff, 0x0000000f)             }  /*  6 */
};

/* Register array encodings for this chip */
static cdk_xgsm_numel_encoding_t _numel_encodings[] = {
    { { 8 } },
    { {  0, -1 } },
    { {  1, -1 } },
    { {  2, -1 } },
    { {  3, -1 } },
    { {  4, -1 } },
    { {  1,  5, -1 } },
    { {  6, -1 } }
};

/* Variable register array info */
cdk_xgsm_numel_info_t bcm56440_b0_numel_info = {
    _numel_ranges,
    _numel_encodings
};

/* Chip information structure */
static cdk_xgsm_chip_info_t bcm56440_b0_chip_info = {

    /* CMIC block */
    BCM56440_B0_CMIC_BLOCK,

    /* CMC instance */
    0,

    /* Other (non-CMIC) block types */
    10,
    bcm56440_b0_blktype_names,

    /* Address calculation */
    bcm56440_b0_blockport_addr,

    /* Other (non-CMIC) blocks */
    16,
    bcm56440_b0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_2(0x1fffffff, 0x00000008),

    /* Chip flags */
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56440_b0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56440_b0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm56440_b0_numel_info,

    /* Configuration dependent memory max index */
    NULL,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56440_b0_setup(cdk_dev_t *dev);

int
bcm56440_b0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56440_b0_chip_info;

    return cdk_xgsm_setup(dev);
}

