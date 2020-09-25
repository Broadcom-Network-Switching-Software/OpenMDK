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
const char *bcm56150_a0_blktype_names[] = {
    "cmic",
    "epipe",
    "gport",
    "ipipe",
    "iproc",
    "mmu",
    "ser",
    "top",
    "xlport"
};

/* Block structures */
cdk_xgsm_block_t bcm56150_a0_blocks[] = 
{
    { BCM56150_A0_BLKTYPE_GPORT,         2,   0,   CDK_PBMP_1(0x000003fc) },
    { BCM56150_A0_BLKTYPE_GPORT,         3,   0,   CDK_PBMP_1(0x0003fc00) },
    { BCM56150_A0_BLKTYPE_GPORT,         4,   0,   CDK_PBMP_1(0x03fc0000) },
    { BCM56150_A0_BLKTYPE_XLPORT,        5,   0,   CDK_PBMP_1(0x3c000000) },
    { BCM56150_A0_BLKTYPE_XLPORT,        6,   0,   CDK_PBMP_2(0xc0000000, 0x00000003) },
    { BCM56150_A0_BLKTYPE_IPIPE,        10,   1,   CDK_PBMP_1(0x3fffffff) },
    { BCM56150_A0_BLKTYPE_EPIPE,        11,   1,   CDK_PBMP_1(0x3fffffff) },
    { BCM56150_A0_BLKTYPE_MMU,          12,   2,   CDK_PBMP_1(0x3fffffff) },
    { BCM56150_A0_BLKTYPE_IPROC,        15,   0,   CDK_PBMP_1(0x00000001) },
    { BCM56150_A0_BLKTYPE_TOP,          16,   0,   CDK_PBMP_2(0xfffffffd, 0x00000003) },
    { BCM56150_A0_BLKTYPE_SER,          19,   0,   CDK_PBMP_2(0xfffffffd, 0x00000003) }
};

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
    0, 9, 8, 7, 6, 5, 4, 3, 2, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33
};
#endif

/* Index ranges for this chip */
static cdk_xgsm_numel_range_t _numel_ranges[] = {
    {  0,  0, CDK_PBMP_1(0x3ffffffd)                         }, /*  0 */
    {  0,  7, CDK_PBMP_1(0x3ffffffd)                         }, /*  1 */
    {  0,  1, CDK_PBMP_1(0x3ffffffd)                         }, /*  2 */
    {  0,  7, CDK_PBMP_1(0x3ffffffc)                         }, /*  3 */
    {  0,  0, CDK_PBMP_1(0x3ffffffc)                         }, /*  4 */
    {  0,  0, CDK_PBMP_1(0x3c000001)                         }  /*  5 */
};

/* Register array encodings for this chip */
static cdk_xgsm_numel_encoding_t _numel_encodings[] = {
    { { 7 } },
    { {  0, -1 } },
    { {  1, -1 } },
    { {  2, -1 } },
    { {  3, -1 } },
    { {  4, -1 } },
    { {  5, -1 } }
};

/* Variable register array info */
cdk_xgsm_numel_info_t bcm56150_a0_numel_info = {
    _numel_ranges,
    _numel_encodings
};

/* Chip information structure */
static cdk_xgsm_chip_info_t bcm56150_a0_chip_info = {

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
bcm56150_a0_setup(cdk_dev_t *dev);

int
bcm56150_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56150_a0_chip_info;

    return cdk_xgsm_setup(dev);
}

