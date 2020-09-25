/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Common XGS chip functions.
 */

#include <cdk/cdk_assert.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_chip.h>

#include <cdk/arch/xgs_cmic.h>

/*
 * Global mode flags for XGS architecture
 */
uint32_t cdk_xgs_chip_flags[CDK_CONFIG_MAX_UNITS];

/*
 * Basic CMIC Initialization
 */
#define CMIC_BIG_ENDIAN_PIO               0x01000001
#define CMIC_BIG_ENDIAN_DMA_PACKET        0x02000002
#define CMIC_BIG_ENDIAN_DMA_OTHER         0x04000004

static int
_cmic_endian_config(int unit)
{
    CMIC_ENDIANESS_SELr_t ces; 
    uint32_t endian_sel = 0;
    int ioerr = 0;

    if (CDK_DEV_FLAGS(unit) & CDK_DEV_BE_PIO) {
        endian_sel |= CMIC_BIG_ENDIAN_PIO; 
    }
    if (CDK_DEV_FLAGS(unit) & CDK_DEV_BE_PACKET) {
        endian_sel |= CMIC_BIG_ENDIAN_DMA_PACKET; 
    }
    if (CDK_DEV_FLAGS(unit) & CDK_DEV_BE_OTHER) {
        endian_sel |= CMIC_BIG_ENDIAN_DMA_OTHER; 
    }

    CMIC_ENDIANESS_SELr_SET(ces, endian_sel);
    ioerr += WRITE_CMIC_ENDIANESS_SELr(unit, ces); 

    return ioerr; 
}

static int
_cmic_burst_config(int unit)
{
    CMIC_CONFIGr_t cc; 
    int ioerr = 0;
    
    /* Read the current CMIC_CONFIG register */
    ioerr += READ_CMIC_CONFIGr(unit, &cc); 

    /* Enable Read and Write Bursting */
    CMIC_CONFIGr_RD_BRST_ENf_SET(cc, 1);
    CMIC_CONFIGr_WR_BRST_ENf_SET(cc, 1);

    /* Write the config */
    ioerr += WRITE_CMIC_CONFIGr(unit, cc); 
    
    return ioerr; 
}

int 
cdk_xgs_cmic_init(int unit)
{
    CMIC_IRQ_MASKr_t irq_mask;
    int ioerr = 0;

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 

    /*
     * Certain PCIe cores may occasionally return invalid data in the
     * first PCI read following a soft-reset (CPS reset). The following
     * read operation is a dummy read to ensure that any invalid data
     * is flushed from the PCI read pipeline.
     */
    ioerr += READ_CMIC_IRQ_MASKr(unit, &irq_mask); 

    /* Configure endian */
    ioerr += _cmic_endian_config(unit); 
    
    /* Configure Bursting */
    ioerr += _cmic_burst_config(unit); 

    /* Disable Interrupts */
    CMIC_IRQ_MASKr_CLR(irq_mask);
    ioerr += WRITE_CMIC_IRQ_MASKr(unit, irq_mask); 
    
    return ioerr; 
}
