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

#include <cdk/chip/bcm56160_a0_defs.h>

/* Block types */
const char *bcm56160_a0_blktype_names[] = {
    "avs",
    "cmic",
    "epipe",
    "gport",
    "ipipe",
    "iproc",
    "mmu",
    "pgw_ge",
    "pmq",
    "ser",
    "top",
    "xlport"
};

/* Block structures */
cdk_xgsd_block_t bcm56160_a0_blocks[] = 
{
    { BCM56160_A0_BLKTYPE_GPORT,        33,   0,   CDK_PBMP_1(0x000003fc) },
    { BCM56160_A0_BLKTYPE_GPORT,        34,   0,   CDK_PBMP_1(0x0003fc00) },
    { BCM56160_A0_BLKTYPE_GPORT,        37,   0,   CDK_PBMP_1(0x03fc0000) },
    { BCM56160_A0_BLKTYPE_GPORT,        38,   0,   CDK_PBMP_2(0xfc000000, 0x00000003) },
    { BCM56160_A0_BLKTYPE_XLPORT,        4,   0,   CDK_PBMP_2(0x00000000, 0x0000003c) },
    { BCM56160_A0_BLKTYPE_XLPORT,        5,   0,   CDK_PBMP_2(0x00000000, 0x000003c0) },
    { BCM56160_A0_BLKTYPE_IPIPE,        10,   1,   CDK_PBMP_1(0xffffffff) },
    { BCM56160_A0_BLKTYPE_EPIPE,        11,   1,   CDK_PBMP_1(0xffffffff) },
    { BCM56160_A0_BLKTYPE_MMU,          12,   2,   CDK_PBMP_1(0xffffffff) },
    { BCM56160_A0_BLKTYPE_IPROC,        15,   0,   CDK_PBMP_1(0x00000001) },
    { BCM56160_A0_BLKTYPE_TOP,          16,   0,   CDK_PBMP_2(0xfffffffd, 0x000003ff) },
    { BCM56160_A0_BLKTYPE_AVS,          17,   0,   CDK_PBMP_2(0xfffffffd, 0x000003ff) },
    { BCM56160_A0_BLKTYPE_SER,          19,   0,   CDK_PBMP_2(0xfffffffd, 0x000003ff) },
    { BCM56160_A0_BLKTYPE_PGW_GE,       32,   0,   CDK_PBMP_1(0x0003fffc) },
    { BCM56160_A0_BLKTYPE_PMQ,          35,   0,   CDK_PBMP_1(0x0003fffc) },
    { BCM56160_A0_BLKTYPE_PGW_GE,       36,   0,   CDK_PBMP_2(0xfffc0000, 0x00000003) },
    { BCM56160_A0_BLKTYPE_PMQ,          39,   0,   CDK_PBMP_2(0xfffc0000, 0x00000003) }
};

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56160_a0_dsymbols;
#else
extern cdk_symbols_t bcm56160_a0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41
};
#endif

/* Index ranges for this chip */
static cdk_xgsd_numel_range_t _numel_ranges[] = {
    {  0,  0, CDK_PBMP_1(0xfffffffd)                         }, /*  0 */
    {  0,  7, CDK_PBMP_1(0xfffffffd)                         }, /*  1 */
    {  0,  1, CDK_PBMP_1(0xfffffffd)                         }, /*  2 */
    {  0,  0, CDK_PBMP_1(0xfffffffc)                         }, /*  3 */
    {  0,  7, CDK_PBMP_1(0xfffffffc)                         }  /*  4 */
};

/* Register array encodings for this chip */
static cdk_xgsd_numel_encoding_t _numel_encodings[] = {
    { { 6 } },
    { {  0, -1 } },
    { {  1, -1 } },
    { {  2, -1 } },
    { {  3, -1 } },
    { {  4, -1 } }
};

/* Variable register array info */
cdk_xgsd_numel_info_t bcm56160_a0_numel_info = {
    _numel_ranges,
    _numel_encodings
};

/* Chip information structure */
static cdk_xgsd_chip_info_t bcm56160_a0_chip_info = {

    /* CMIC block */
    BCM56160_A0_CMIC_BLOCK,

    /* CMC instance */
    0,

    /* Other (non-CMIC) block types */
    12,
    bcm56160_a0_blktype_names,

    /* Address calculation */
    NULL,

    /* Other (non-CMIC) blocks */
    17,
    bcm56160_a0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_2(0xfffffffd, 0x000003ff),

    /* Chip flags */
    CDK_XGSD_CHIP_FLAG_IPROC |
    BCM56160_A0_CHIP_FLAG_FREQ300 |
    BCM56160_A0_CHIP_FLAG_QTC25G |
    BCM56160_A0_CHIP_FLAG_TSC40G |
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56160_a0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56160_a0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm56160_a0_numel_info,

    /* Configuration dependent memory max index */
    NULL,

    /* Pipe info */
    NULL,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56160_a0_setup(cdk_dev_t *dev);

int
bcm56160_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56160_a0_chip_info;

    return cdk_xgsd_setup(dev);
}

