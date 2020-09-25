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

#include <cdk/chip/bcm53125_a0_defs.h>

/* Block types */
const char *bcm53125_a0_blktype_names[] = {
    "cpic",
    "exp",
    "gpic",
    "spi",
    "sys"
};

/* Block structures */
cdk_robo_block_t bcm53125_a0_blocks[] = 
{
    { BCM53125_A0_BLKTYPE_GPIC,       CDK_PBMP_1(0x0000003f) },
    { BCM53125_A0_BLKTYPE_CPIC,       CDK_PBMP_1(0x00000100) },
    { BCM53125_A0_BLKTYPE_EXP,        CDK_PBMP_1(0x0000013f) },
    { BCM53125_A0_BLKTYPE_SPI,        CDK_PBMP_1(0x0000013f) },
    { BCM53125_A0_BLKTYPE_SYS,        CDK_PBMP_1(0x0000013f) }
};

/* Symbol table */
#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
extern cdk_symbols_t bcm53125_a0_dsymbols;
#else
extern cdk_symbols_t bcm53125_a0_symbols;
#endif
#endif

/* Physical port numbers */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
static cdk_port_map_port_t _ports[] = {
    8, 0, 1, 2, 3, 4, 5
};
#endif

/* Declare function first to prevent compiler warnings */
extern uint32_t
bcm53125_a0_blockport_addr(int port, int size, uint32_t offset);

uint32_t
bcm53125_a0_blockport_addr(int port, int size, uint32_t offset)
{
    if (port < 0) {
        port = 0;
    }
    if ((offset & 0xff00) == 0x1000 ||  /* GMII */
        (offset & 0xff00) == 0x2000) {  /* MIB */
        /* Use page size */
        size = 0x100;
    }
    port -= (offset >> 16);
    return (offset & 0xffff) + (port * size); 
}

/* Chip information structure */
static cdk_robo_chip_info_t bcm53125_a0_chip_info = {

    /* Block types */
    5,
    bcm53125_a0_blktype_names,

    /* Address calculation */
    bcm53125_a0_blockport_addr,

    /* Chip blocks */
    5,
    bcm53125_a0_blocks,

    /* Valid ports for this chip */
    CDK_PBMP_1(0x0000013f),

    /* Chip flags */
    0,

#if CDK_CONFIG_INCLUDE_CHIP_SYMBOLS == 1
#if CDK_CONFIG_CHIP_SYMBOLS_USE_DSYMS == 1
    &bcm53125_a0_dsymbols,
#else
    &bcm53125_a0_symbols,
#endif
#endif

    /* Port map */
#if CDK_CONFIG_INCLUDE_PORT_MAP == 1
    sizeof(_ports)/sizeof(_ports[0]),
    _ports,
#endif

};

/* Declare function first to prevent compiler warnings */
extern int
bcm53125_a0_setup(cdk_dev_t *dev);

int
bcm53125_a0_setup(cdk_dev_t *dev)
{
    dev->chip_info = &bcm53125_a0_chip_info;

    return cdk_robo_setup(dev);
}

