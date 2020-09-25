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

#include <cdk/chip/bcm56634_b0_defs.h>

/* Block types */
extern const char *bcm56634_b0_blktype_names[];

/* Block structures */
extern cdk_xgs_block_t bcm56634_b0_blocks[];

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm56634_b0_dsymbols;
#else
extern cdk_symbols_t bcm56634_b0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29
};
#endif

/* Declare function first to prevent compiler warnings */
extern uint32_t
bcm56634_b0_blockport_addr(int block, int port, uint32_t offset);

/* Chip-specific memory sizes */
static uint32_t
bcm56634_b0_mem_maxidx(uint32_t addr, uint32_t maxidx)
{
    switch (addr) {
    case EGR_IPFIX_SESSION_TABLEm:
    case ING_IPFIX_SESSION_TABLEm:
        return 2047; /* 0x7ff */
    case MMU_CBPDATA0m:
    case MMU_CBPDATA1m:
    case MMU_CBPDATA10m:
    case MMU_CBPDATA11m:
    case MMU_CBPDATA12m:
    case MMU_CBPDATA13m:
    case MMU_CBPDATA14m:
    case MMU_CBPDATA15m:
    case MMU_CBPDATA16m:
    case MMU_CBPDATA17m:
    case MMU_CBPDATA18m:
    case MMU_CBPDATA19m:
    case MMU_CBPDATA2m:
    case MMU_CBPDATA20m:
    case MMU_CBPDATA21m:
    case MMU_CBPDATA22m:
    case MMU_CBPDATA23m:
    case MMU_CBPDATA24m:
    case MMU_CBPDATA25m:
    case MMU_CBPDATA26m:
    case MMU_CBPDATA27m:
    case MMU_CBPDATA28m:
    case MMU_CBPDATA29m:
    case MMU_CBPDATA3m:
    case MMU_CBPDATA30m:
    case MMU_CBPDATA31m:
    case MMU_CBPDATA4m:
    case MMU_CBPDATA5m:
    case MMU_CBPDATA6m:
    case MMU_CBPDATA7m:
    case MMU_CBPDATA8m:
    case MMU_CBPDATA9m:
        return 24575; /* 0x5fff */
    case L2_USER_ENTRYm:
    case L2_USER_ENTRY_DATA_ONLYm:
    case L2_USER_ENTRY_ONLYm:
    case VLAN_SUBNETm:
    case VLAN_SUBNET_DATA_ONLYm:
    case VLAN_SUBNET_ONLYm:
        return 255; /* 0xff */
    case L3_DEFIPm:
    case L3_DEFIP_DATA_ONLYm:
    case L3_DEFIP_HIT_ONLYm:
    case L3_DEFIP_ONLYm:
        return 6143; /* 0x17ff */
    }
    return maxidx;
}

/* Variable register array info */
extern cdk_xgs_numel_info_t bcm56634_b0_numel_info;

/* Chip information structure */
static cdk_xgs_chip_info_t bcm56521_b0_chip_info = {

    /* CMIC block */
    BCM56634_B0_CMIC_BLOCK,

    /* Other (non-CMIC) block types */
    9,
    bcm56634_b0_blktype_names,

    /* Address calculation */
    bcm56634_b0_blockport_addr,

    /* Other (non-CMIC) blocks */
    17,
    bcm56634_b0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_2(0x3ffffffd, 0x01c00000),

    /* Chip flags */
    CDK_XGS_CHIP_FLAG_CLAUSE45 |
    CDK_XGS_CHIP_FLAG_SCHAN_EXT |
    BCM56634_B0_CHIP_FLAG_NO_ESM |
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    /* Use regenerated symbol tables from the DSYM program */
    &bcm56634_b0_dsymbols,
#else
    /* Use the static per-chip symbol tables */
    &bcm56634_b0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

    /* Variable register array info */
    &bcm56634_b0_numel_info,

    /* Configuration dependent memory max index */
    &bcm56634_b0_mem_maxidx,

};

/* Declare function first to prevent compiler warnings */
extern int
bcm56521_b0_setup(cdk_dev_t *dev);

int
bcm56521_b0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm56521_b0_chip_info;

    return cdk_xgs_setup(dev);
}

