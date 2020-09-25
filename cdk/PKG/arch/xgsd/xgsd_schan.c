/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * S-Channel (internal command bus) support
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_debug.h>

#include <cdk/arch/xgsd_schan.h>
#include <cdk/arch/xgsd_cmic.h>

/*
 * Resets the CMIC S-Channel interface. This is required when we sent
 * a message and did not receive a response after the poll count was
 * exceeded.
 */

static void
_schan_reset(int unit)
{
    CMIC_CMC_SCHAN_CTRLr_t schan_ctrl;
    
    CDK_DEBUG_SCHAN(("cdk_xgsd_schan_op[%d]: S-channel reset\n", unit));

    READ_CMIC_CMC_SCHAN_CTRLr(unit, &schan_ctrl); 
    
    /* Toggle S-Channel abort bit in CMIC_CMC_SCHAN_CTRL register */
    CMIC_CMC_SCHAN_CTRLr_ABORTf_SET(schan_ctrl, 1); 
    WRITE_CMIC_CMC_SCHAN_CTRLr(unit, schan_ctrl); 

    CDK_CONFIG_MEMORY_BARRIER; 

    CMIC_CMC_SCHAN_CTRLr_ABORTf_SET(schan_ctrl, 0); 
    WRITE_CMIC_CMC_SCHAN_CTRLr(unit, schan_ctrl); 

    CDK_CONFIG_MEMORY_BARRIER; 
}

int
cdk_xgsd_schan_op(int unit,
                 schan_msg_t *msg,
                 int dwc_write, int dwc_read)
{
    int i, rv = CDK_E_NONE; 
    uint32_t polls = 0; 
    CMIC_CMC_SCHAN_CTRLr_t schan_ctrl; 
    uint32_t msg_addr;

    /* S-Channel message buffer address */
    msg_addr = CMIC_CMC_SCHAN_MESSAGEr;

    /* Write raw S-Channel Data: dwc_write words */
    CDK_DEBUG_SCHAN(("cdk_xgsd_schan_op[%d]: S-channel write:", unit));
    for (i = 0; i < dwc_write; i++) {
        CDK_XGSD_CMC_WRITE(unit, msg_addr + i*4, msg->dwords[i]);
        CDK_DEBUG_SCHAN((" 0x%08"PRIx32"", msg->dwords[i]));
    }
    CDK_DEBUG_SCHAN(("\n"));

    /* Tell CMIC to start */
    READ_CMIC_CMC_SCHAN_CTRLr(unit, &schan_ctrl); 
    CMIC_CMC_SCHAN_CTRLr_MSG_STARTf_SET(schan_ctrl, 1); 
    WRITE_CMIC_CMC_SCHAN_CTRLr(unit, schan_ctrl); 

    CDK_CONFIG_MEMORY_BARRIER; 
    
    /* Poll for completion */
    for (polls = 0; polls < CDK_CONFIG_SCHAN_MAX_POLLS; polls++) {
        READ_CMIC_CMC_SCHAN_CTRLr(unit, &schan_ctrl); 
        if (CMIC_CMC_SCHAN_CTRLr_MSG_DONEf_GET(schan_ctrl)) {
            break; 
        }
    }

    /* Check for timeout and error conditions */
    if (polls == CDK_CONFIG_SCHAN_MAX_POLLS) {
        CDK_DEBUG_SCHAN(("cdk_xgsd_schan_op[%d]: S-channel timeout\n", unit));
        rv = CDK_E_TIMEOUT; 
    }

    if (CMIC_CMC_SCHAN_CTRLr_NACKf_GET(schan_ctrl)) {
        CDK_DEBUG_SCHAN(("cdk_xgsd_schan_op[%d]: S-channel NAK\n", unit));
        rv = CDK_E_FAIL; 
    }
            
    if (CMIC_CMC_SCHAN_CTRLr_SER_CHECK_FAILf_GET(schan_ctrl)) {
        CDK_DEBUG_SCHAN(("cdk_xgsd_schan_op[%d]: S-channel SER error\n", unit));
        rv = CDK_E_FAIL; 
    }
            
    if (CMIC_CMC_SCHAN_CTRLr_TIMEOUTf_GET(schan_ctrl)) {
        CDK_DEBUG_SCHAN(("cdk_xgsd_schan_op[%d]: S-channel TO error\n", unit));
        rv = CDK_E_FAIL; 
    }
            
    CMIC_CMC_SCHAN_CTRLr_MSG_DONEf_SET(schan_ctrl, 0);
    WRITE_CMIC_CMC_SCHAN_CTRLr(unit, schan_ctrl); 

    CDK_CONFIG_MEMORY_BARRIER; 

    if (CDK_FAILURE(rv)) {
        _schan_reset(unit);
        return rv; 
    }

    /* Read in data from S-Channel buffer space, if any */
    CDK_DEBUG_SCHAN(("cdk_xgsd_schan_op[%d]: S-channel read:", unit));
    for (i = 0; i < dwc_read; i++) {
         CDK_XGSD_CMC_READ(unit, msg_addr + 4*i, &msg->dwords[i]); 
         CDK_DEBUG_SCHAN((" 0x%08"PRIx32"", msg->dwords[i]));
    }
    CDK_DEBUG_SCHAN(("\n"));

    return CDK_E_NONE; 
}


