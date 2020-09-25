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

#include <cdk/chip/bcm56860_a0_defs.h>

/* Block types */
const char *bcm56860_a0_blktype_names[] = {
    "avs",
    "cmic",
    "cport",
    "epipe",
    "ipipe",
    "lbport",
    "mmu",
    "pgw_cl",
    "ser",
    "top",
    "xlport"
};

/* Block structures */
cdk_xgsm_block_t bcm56860_a0_blocks[] = 
{
    { BCM56860_A0_BLKTYPE_CPORT,        14,   0,   CDK_PBMP_1(0x00000002) },
    { BCM56860_A0_BLKTYPE_CPORT,        19,   0,   CDK_PBMP_1(0x00200000) },
    { BCM56860_A0_BLKTYPE_CPORT,        24,   0,   CDK_PBMP_2(0x00000000, 0x00000002) },
    { BCM56860_A0_BLKTYPE_CPORT,        29,   0,   CDK_PBMP_2(0x00000000, 0x00200000) },
    { BCM56860_A0_BLKTYPE_CPORT,        34,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x00000002) },
    { BCM56860_A0_BLKTYPE_CPORT,        39,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x00200000) },
    { BCM56860_A0_BLKTYPE_CPORT,        44,   0,   CDK_PBMP_4(0x00000000, 0x00000000, 0x00000000, 0x00000002) },
    { BCM56860_A0_BLKTYPE_CPORT,        49,   0,   CDK_PBMP_4(0x00000000, 0x00000000, 0x00000000, 0x00200000) },
    { BCM56860_A0_BLKTYPE_LBPORT,       54,   0,   CDK_PBMP_5(0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002) },
    { BCM56860_A0_BLKTYPE_XLPORT,       15,   0,   CDK_PBMP_1(0x0000001e) },
    { BCM56860_A0_BLKTYPE_XLPORT,       16,   0,   CDK_PBMP_1(0x000001e0) },
    { BCM56860_A0_BLKTYPE_XLPORT,       17,   0,   CDK_PBMP_1(0x00001e00) },
    { BCM56860_A0_BLKTYPE_XLPORT,       18,   0,   CDK_PBMP_1(0x0001e000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       20,   0,   CDK_PBMP_2(0xe0000000, 0x00000001) },
    { BCM56860_A0_BLKTYPE_XLPORT,       21,   0,   CDK_PBMP_1(0x1e000000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       22,   0,   CDK_PBMP_1(0x01e00000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       23,   0,   CDK_PBMP_1(0x001e0000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       25,   0,   CDK_PBMP_2(0x00000000, 0x0000001e) },
    { BCM56860_A0_BLKTYPE_XLPORT,       26,   0,   CDK_PBMP_2(0x00000000, 0x000001e0) },
    { BCM56860_A0_BLKTYPE_XLPORT,       27,   0,   CDK_PBMP_2(0x00000000, 0x00001e00) },
    { BCM56860_A0_BLKTYPE_XLPORT,       28,   0,   CDK_PBMP_2(0x00000000, 0x0001e000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       30,   0,   CDK_PBMP_3(0x00000000, 0xe0000000, 0x00000001) },
    { BCM56860_A0_BLKTYPE_XLPORT,       31,   0,   CDK_PBMP_2(0x00000000, 0x1e000000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       32,   0,   CDK_PBMP_2(0x00000000, 0x01e00000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       33,   0,   CDK_PBMP_2(0x00000000, 0x001e0000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       35,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x0000001e) },
    { BCM56860_A0_BLKTYPE_XLPORT,       36,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x000001e0) },
    { BCM56860_A0_BLKTYPE_XLPORT,       37,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x00001e00) },
    { BCM56860_A0_BLKTYPE_XLPORT,       38,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x0001e000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       40,   0,   CDK_PBMP_4(0x00000000, 0x00000000, 0xe0000000, 0x00000001) },
    { BCM56860_A0_BLKTYPE_XLPORT,       41,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x1e000000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       42,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x01e00000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       43,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x001e0000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       45,   0,   CDK_PBMP_4(0x00000000, 0x00000000, 0x00000000, 0x0000001e) },
    { BCM56860_A0_BLKTYPE_XLPORT,       46,   0,   CDK_PBMP_4(0x00000000, 0x00000000, 0x00000000, 0x000001e0) },
    { BCM56860_A0_BLKTYPE_XLPORT,       47,   0,   CDK_PBMP_4(0x00000000, 0x00000000, 0x00000000, 0x00001e00) },
    { BCM56860_A0_BLKTYPE_XLPORT,       48,   0,   CDK_PBMP_4(0x00000000, 0x00000000, 0x00000000, 0x0001e000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       50,   0,   CDK_PBMP_5(0x00000000, 0x00000000, 0x00000000, 0xe0000000, 0x00000001) },
    { BCM56860_A0_BLKTYPE_XLPORT,       51,   0,   CDK_PBMP_4(0x00000000, 0x00000000, 0x00000000, 0x1e000000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       52,   0,   CDK_PBMP_4(0x00000000, 0x00000000, 0x00000000, 0x01e00000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       53,   0,   CDK_PBMP_4(0x00000000, 0x00000000, 0x00000000, 0x001e0000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       55,   0,   CDK_PBMP_1(0x00000000) },
    { BCM56860_A0_BLKTYPE_XLPORT,       56,   0,   CDK_PBMP_1(0x00000000) },
    { BCM56860_A0_BLKTYPE_IPIPE,         1,   1,   CDK_PBMP_4(0xffffffff, 0xffffffff, 0xffffffff, 0x000003ff) },
    { BCM56860_A0_BLKTYPE_EPIPE,         2,   1,   CDK_PBMP_4(0xffffffff, 0xffffffff, 0xffffffff, 0x000003ff) },
    { BCM56860_A0_BLKTYPE_MMU,           3,   2,   CDK_PBMP_4(0xffffffff, 0xffffffff, 0xffffffff, 0x001fffff) },
    { BCM56860_A0_BLKTYPE_PGW_CL,        6,   0,   CDK_PBMP_1(0x0001fffe) },
    { BCM56860_A0_BLKTYPE_PGW_CL,        7,   0,   CDK_PBMP_2(0xfffe0000, 0x00000001) },
    { BCM56860_A0_BLKTYPE_PGW_CL,        8,   0,   CDK_PBMP_2(0x00000000, 0x0001fffe) },
    { BCM56860_A0_BLKTYPE_PGW_CL,        9,   0,   CDK_PBMP_3(0x00000000, 0xfffe0000, 0x00000001) },
    { BCM56860_A0_BLKTYPE_PGW_CL,       10,   0,   CDK_PBMP_3(0x00000000, 0x00000000, 0x0001fffe) },
    { BCM56860_A0_BLKTYPE_PGW_CL,       11,   0,   CDK_PBMP_4(0x00000000, 0x00000000, 0xfffe0000, 0x00000001) },
    { BCM56860_A0_BLKTYPE_PGW_CL,       12,   0,   CDK_PBMP_4(0x00000000, 0x00000000, 0x00000000, 0x0001fffe) },
    { BCM56860_A0_BLKTYPE_PGW_CL,       13,   0,   CDK_PBMP_5(0x00000000, 0x00000000, 0x00000000, 0xfffe0000, 0x00000001) },
    { BCM56860_A0_BLKTYPE_TOP,          57,   0,   CDK_PBMP_5(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000003) },
    { BCM56860_A0_BLKTYPE_SER,          58,   0,   CDK_PBMP_5(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000003) },
    { BCM56860_A0_BLKTYPE_AVS,          59,   0,   CDK_PBMP_5(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000003) }
};

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56860_a0_dsymbols;
#else
extern cdk_symbols_t bcm56860_a0_symbols;
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
bcm56860_a0_blockport_addr(int block, int port, uint32_t offset, uint32_t idx);

uint32_t
bcm56860_a0_blockport_addr(int block, int port, uint32_t offset, uint32_t idx)
{
    if (port < 0) {
        return (offset + idx);
    } else {
        if (block >= 6 && block <= 13) { /* BLKTYPE_PGW_CL */
            if (block & 0x1) {
                port ^= 0xc;
            }
        }
        return ((offset | port) + (idx * 0x100));
    }
}

/* Index ranges for this chip */
static cdk_xgsm_numel_range_t _numel_ranges[] = {
    {  0,  0, CDK_PBMP_4(0xffffffff, 0x001fffff, 0xffffffff, 0x001fffff) }, /*  0 */
    {  0,  0, CDK_PBMP_3(0x0000ffff, 0x00000000, 0x0000ffff) }, /*  1 */
    {  0,  4, CDK_PBMP_3(0x0000ffff, 0x00000000, 0x0000ffff) }, /*  2 */
    {  0,  9, CDK_PBMP_3(0x0000ffff, 0x00000000, 0x0000ffff) }, /*  3 */
    {  0,  3, CDK_PBMP_4(0xffffffff, 0x001fffff, 0xffffffff, 0x001fffff) }, /*  4 */
    {  0,  1, CDK_PBMP_4(0xffffffff, 0x001fffff, 0xffffffff, 0x001fffff) }  /*  5 */
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
cdk_xgsm_numel_info_t bcm56860_a0_numel_info = {
    _numel_ranges,
    _numel_encodings
};

/* Chip information structure */
static cdk_xgsm_chip_info_t bcm56860_a0_chip_info = {

    /* CMIC block */
    BCM56860_A0_CMIC_BLOCK,

    /* CMC instance */
    0,

    /* Other (non-CMIC) block types */
    11,
    bcm56860_a0_blktype_names,

    /* Address calculation */
    bcm56860_a0_blockport_addr,

    /* Other (non-CMIC) blocks */
    57,
    bcm56860_a0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_5(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000003),

    /* Chip flags */
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56860_a0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56860_a0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm56860_a0_numel_info,

    /* Configuration dependent memory max index */
    NULL,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56860_a0_setup(cdk_dev_t *dev);

int
bcm56860_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56860_a0_chip_info;

    return cdk_xgsm_setup(dev);
}
