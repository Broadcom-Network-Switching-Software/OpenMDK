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

#include <cdk/chip/bcm53570_b0_defs.h>

/* Block types */
const char *bcm53570_b0_blktype_names[] = {
    "avs",
    "clport",
    "cmic",
    "crypto",
    "epipe",
    "gport",
    "ipipe",
    "iproc",
    "mmu",
    "pgw_ge",
    "pgw_ge8p",
    "pmq",
    "ser",
    "taf",
    "top",
    "xlport"
};

/* Block structures */
cdk_xgsd_block_t bcm53570_b0_blocks[] = 
{
    { BCM53570_b0_BLKTYPE_CLPORT,        2,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x03c00000) },
    { BCM53570_b0_BLKTYPE_GPORT,        29,   0,   CDK_PBMP_1(0x000003fc) },
    { BCM53570_b0_BLKTYPE_GPORT,        33,   0,   CDK_PBMP_1(0x0003fc00) },
    { BCM53570_b0_BLKTYPE_GPORT,        37,   0,   CDK_PBMP_1(0x03fc0000) },
    { BCM53570_b0_BLKTYPE_GPORT,        41,   0,   CDK_PBMP_2(0xfc000000, 0x00000003) },
    { BCM53570_b0_BLKTYPE_GPORT,        42,   0,   CDK_PBMP_2(0x00000000, 0x000003fc) },
    { BCM53570_b0_BLKTYPE_GPORT,        45,   0,   CDK_PBMP_2(0x00000000, 0x0003fc00) },
    { BCM53570_b0_BLKTYPE_GPORT,        46,   0,   CDK_PBMP_2(0x00000000, 0x03fc0000) },
    { BCM53570_b0_BLKTYPE_XLPORT,        4,   0,   CDK_PBMP_2(0x00000000, 0x3c000000) },
    { BCM53570_b0_BLKTYPE_XLPORT,        5,   0,   CDK_PBMP_3(0x00000000, 0xc0000000, 0x00000003) },
    { BCM53570_b0_BLKTYPE_XLPORT,        6,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x0000003c) },
    { BCM53570_b0_BLKTYPE_XLPORT,        7,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x000003c0) },
    { BCM53570_b0_BLKTYPE_XLPORT,       24,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x00003c00) },
    { BCM53570_b0_BLKTYPE_XLPORT,       25,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x0003c000) },
    { BCM53570_b0_BLKTYPE_XLPORT,       26,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x003c0000) },
    { BCM53570_b0_BLKTYPE_TAF,           9,   0,   CDK_PBMP_4(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff) },
    { BCM53570_b0_BLKTYPE_IPIPE,        10,   1,   CDK_PBMP_3(0xffffffff, 0xffffffff, 0x00000003) },
    { BCM53570_b0_BLKTYPE_EPIPE,        11,   1,   CDK_PBMP_3(0xffffffff, 0xffffffff, 0x00000003) },
    { BCM53570_b0_BLKTYPE_MMU,          12,   2,   CDK_PBMP_3(0xffffffff, 0xffffffff, 0x00000003) },
    { BCM53570_b0_BLKTYPE_CRYPTO,       15,   0,   CDK_PBMP_1(0x00000001) },
    { BCM53570_b0_BLKTYPE_IPROC,        15,   0,   CDK_PBMP_1(0x00000001) },
    { BCM53570_b0_BLKTYPE_TOP,          16,   0,   CDK_PBMP_3(0xfffffffd, 0xffffffff, 0x03ffffff) },
    { BCM53570_b0_BLKTYPE_AVS,          17,   0,   CDK_PBMP_3(0xfffffffd, 0xffffffff, 0x03ffffff) },
    { BCM53570_b0_BLKTYPE_SER,          19,   0,   CDK_PBMP_3(0xfffffffd, 0xffffffff, 0x03ffffff) },
    { BCM53570_b0_BLKTYPE_PGW_GE8P,     28,   0,   CDK_PBMP_1(0x0003fffc) },
    { BCM53570_b0_BLKTYPE_PMQ,          30,   0,   CDK_PBMP_1(0x00000004) },
    { BCM53570_b0_BLKTYPE_PGW_GE8P,     32,   0,   CDK_PBMP_1(0x00000400) },
    { BCM53570_b0_BLKTYPE_PMQ,          34,   0,   CDK_PBMP_1(0x00000400) },
    { BCM53570_b0_BLKTYPE_PGW_GE8P,     36,   0,   CDK_PBMP_2(0xfffc0000, 0x00000003) },
    { BCM53570_b0_BLKTYPE_PMQ,          38,   0,   CDK_PBMP_1(0x00040000) },
    { BCM53570_b0_BLKTYPE_PGW_GE,       40,   0,   CDK_PBMP_2(0xfc000000, 0x000003ff) },
    { BCM53570_b0_BLKTYPE_PMQ,          43,   0,   CDK_PBMP_1(0x04000000) },
    { BCM53570_b0_BLKTYPE_PGW_GE,       44,   0,   CDK_PBMP_2(0x00000000, 0x03fffc00) },
    { BCM53570_b0_BLKTYPE_PMQ,          47,   0,   CDK_PBMP_2(0x00000000, 0x03fffc00) }
};

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm53570_b0_dsymbols;
#else
extern cdk_symbols_t bcm53570_b0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85
};
#endif

/* Index ranges for this chip */
static cdk_xgsd_numel_range_t _numel_ranges[] = {
    {  0,  0, CDK_PBMP_3(0x00000000, 0xfc000000, 0x00000003) }, /*  0 */
    {  0,  0, CDK_PBMP_2(0xfffffffd, 0x03ffffff)             }, /*  1 */
    {  0,  0, CDK_PBMP_3(0xfffffffd, 0xffffffff, 0x00000003) }, /*  2 */
    {  0,  1, CDK_PBMP_3(0xfffffffc, 0xffffffff, 0x00000003) }, /*  3 */
    {  0,  7, CDK_PBMP_2(0xfffffffd, 0x03ffffff)             }, /*  4 */
    {  0,  7, CDK_PBMP_3(0x00000000, 0xfc000000, 0x00000003) }, /*  5 */
    {  0, 63, CDK_PBMP_3(0x00000000, 0xfc000000, 0x00000003) }, /*  6 */
    {  0,  1, CDK_PBMP_3(0x00000000, 0xfc000000, 0x00000003) }, /*  7 */
    {  0, 63, CDK_PBMP_3(0xfffffffd, 0xffffffff, 0x00000003) }, /*  8 */
    {  0,  0, CDK_PBMP_3(0xfffffffc, 0xffffffff, 0x00000003) }, /*  9 */
    {  0,  7, CDK_PBMP_3(0xfffffffd, 0xffffffff, 0x00000003) }, /* 10 */
    {  0,  7, CDK_PBMP_2(0xfffffffc, 0x03ffffff)             }, /* 11 */
    {  0,  7, CDK_PBMP_3(0xfffffffc, 0xffffffff, 0x00000003) }, /* 12 */
    {  0,  0, CDK_PBMP_2(0xfffffffc, 0x03ffffff)             }  /* 13 */
};

/* Register array encodings for this chip */
static cdk_xgsd_numel_encoding_t _numel_encodings[] = {
    { { 15 } },
    { {  0, -1 } },
    { {  1, -1 } },
    { {  2, -1 } },
    { {  3, -1 } },
    { {  4, -1 } },
    { {  5, -1 } },
    { {  6, -1 } },
    { {  7, -1 } },
    { {  8, -1 } },
    { {  9, -1 } },
    { { 10, -1 } },
    { { 11, -1 } },
    { { 12, -1 } },
    { { 13, -1 } }
};

/* Variable register array info */
cdk_xgsd_numel_info_t bcm53570_b0_numel_info = {
    _numel_ranges,
    _numel_encodings
};

/* Chip information structure */
static cdk_xgsd_chip_info_t bcm53570_b0_chip_info = {

    /* CMIC block */
    BCM53570_b0_CMIC_BLOCK,

    /* CMC instance */
    0,

    /* Other (non-CMIC) block types */
    16,
    bcm53570_b0_blktype_names,

    /* Address calculation */
    NULL,

    /* Other (non-CMIC) blocks */
    34,
    bcm53570_b0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_3(0xfffffffd, 0xffffffff, 0x03ffffff),

    /* Chip flags */
    CDK_XGSD_CHIP_FLAG_IPROC |
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm53570_b0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm53570_b0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm53570_b0_numel_info,

    /* Configuration dependent memory max index */
    NULL,

    /* Pipe info */
    NULL,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm53570_b0_setup(cdk_dev_t *dev);

int
bcm53570_b0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm53570_b0_chip_info;

    return cdk_xgsd_setup(dev);
}

