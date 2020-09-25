/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS memory access functions.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_mem.h>
#include <cdk/arch/xgsd_schan.h>
#include <cdk/arch/xgsd_cmic.h>
#include <cdk/arch/xgsd_miim.h>

#define MIIM_PARAM_ID_OFFSET 		16
#define MIIM_PARAM_REG_ADDR_OFFSET	24

int 
cdk_xgsd_miim_write(int unit, uint32_t phy_addr, uint32_t reg, uint32_t val)
{
    int rv = CDK_E_NONE; 
    uint32_t polls;
    uint32_t phy_param;
    CMIC_CMC_MIIM_CTRLr_t miim_ctrl;
    CMIC_CMC_MIIM_STATr_t miim_stat;
    CMIC_CMC_MIIM_ADDRESSr_t miim_addr;
    CMIC_CMC_MIIM_PARAMr_t miim_param;

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 
    
    /*
     * Use clause 45 access if DEVAD specified.
     * Note that DEVAD 32 (0x20) can be used to access special DEVAD 0.
     */
    if (reg & 0x003f0000) {
        phy_addr |= CDK_XGSD_MIIM_CLAUSE45;
        reg &= 0x001fffff;
    }

    CDK_DEBUG_MIIM
        (("cdk_xgsd_miim_write[%d]: phy_addr=0x%08"PRIx32" "
          "reg_addr=%08"PRIx32" data: 0x%08"PRIx32"\n",
          unit, phy_addr, reg, val));

    /* Write address and parameter registers */
    phy_param = (phy_addr << MIIM_PARAM_ID_OFFSET) | val;
    CMIC_CMC_MIIM_PARAMr_SET(miim_param, phy_param);
    WRITE_CMIC_CMC_MIIM_PARAMr(unit, miim_param); 

    CMIC_CMC_MIIM_ADDRESSr_SET(miim_addr, reg);
    WRITE_CMIC_CMC_MIIM_ADDRESSr(unit, miim_addr); 

    /* Tell CMIC to start */
    READ_CMIC_CMC_MIIM_CTRLr(unit, &miim_ctrl);
    CMIC_CMC_MIIM_CTRLr_MIIM_WR_STARTf_SET(miim_ctrl, 1);
    WRITE_CMIC_CMC_MIIM_CTRLr(unit, miim_ctrl);

    /* Poll for completion */
    for (polls = 0; polls < CDK_CONFIG_MIIM_MAX_POLLS; polls++) {
        READ_CMIC_CMC_MIIM_STATr(unit, &miim_stat);
        if (CMIC_CMC_MIIM_STATr_MIIM_OPN_DONEf_GET(miim_stat)) {
            break; 
	}
    }
    
    /* Check for timeout and error conditions */
    if (polls == CDK_CONFIG_MIIM_MAX_POLLS) {
	rv = -1; 
        CDK_DEBUG_MIIM
            (("cdk_xgsd_miim_write[%d]: Timeout at phy_addr=0x%08"PRIx32" "
              "reg_addr=%08"PRIx32"\n",
              unit, phy_addr, reg));
    }
    
    CMIC_CMC_MIIM_CTRLr_MIIM_WR_STARTf_SET(miim_ctrl, 0);
    WRITE_CMIC_CMC_MIIM_CTRLr(unit, miim_ctrl);

    return rv;
}

int
cdk_xgsd_miim_read(int unit, uint32_t phy_addr, uint32_t reg, uint32_t *val)
{
    int rv = CDK_E_NONE; 
    uint32_t polls; 
    uint32_t phy_param;
    CMIC_CMC_MIIM_CTRLr_t miim_ctrl;
    CMIC_CMC_MIIM_STATr_t miim_stat;
    CMIC_CMC_MIIM_ADDRESSr_t miim_addr;
    CMIC_CMC_MIIM_PARAMr_t miim_param;
    CMIC_CMC_MIIM_READ_DATAr_t miim_read_data;

    CDK_ASSERT(CDK_DEV_EXISTS(unit)); 
    
    /*
     * Use clause 45 access if DEVAD specified.
     * Note that DEVAD 32 (0x20) can be used to access special DEVAD 0.
     */
    if (reg & 0x003f0000) {
        phy_addr |= CDK_XGSD_MIIM_CLAUSE45;
        reg &= 0x001fffff;
    }

    phy_param = (phy_addr << MIIM_PARAM_ID_OFFSET);
    CMIC_CMC_MIIM_PARAMr_SET(miim_param, phy_param);
    WRITE_CMIC_CMC_MIIM_PARAMr(unit, miim_param); 

    CMIC_CMC_MIIM_ADDRESSr_SET(miim_addr, reg);
    WRITE_CMIC_CMC_MIIM_ADDRESSr(unit, miim_addr); 

    /* Tell CMIC to start */
    READ_CMIC_CMC_MIIM_CTRLr(unit, &miim_ctrl);
    CMIC_CMC_MIIM_CTRLr_MIIM_RD_STARTf_SET(miim_ctrl, 1);
    WRITE_CMIC_CMC_MIIM_CTRLr(unit, miim_ctrl);

    /* Poll for completion */
    for (polls = 0; polls < CDK_CONFIG_MIIM_MAX_POLLS; polls++) {
        READ_CMIC_CMC_MIIM_STATr(unit, &miim_stat);
        if (CMIC_CMC_MIIM_STATr_MIIM_OPN_DONEf_GET(miim_stat)) {
            break; 
	}
    }
    
    /* Check for timeout and error conditions */
    if (polls == CDK_CONFIG_MIIM_MAX_POLLS) {
	rv = -1; 
        CDK_DEBUG_MIIM
            (("cdk_xgsd_miim_read[%d]: Timeout at phy_addr=0x%08"PRIx32" "
              "reg_addr=%08"PRIx32"\n",
              unit, phy_addr, reg));
    }

    CMIC_CMC_MIIM_CTRLr_MIIM_RD_STARTf_SET(miim_ctrl, 0);
    WRITE_CMIC_CMC_MIIM_CTRLr(unit, miim_ctrl);

    if (rv >= 0) {
        READ_CMIC_CMC_MIIM_READ_DATAr(unit, &miim_read_data);
        *val = CMIC_CMC_MIIM_READ_DATAr_GET(miim_read_data);
        CDK_DEBUG_MIIM
            (("cdk_xgsd_miim_read[%d]: phy_addr=0x%08"PRIx32" "
              "reg_addr=%08"PRIx32" data: 0x%08"PRIx32"\n",
              unit, phy_addr, reg, *val));
    }
    return rv;
}
