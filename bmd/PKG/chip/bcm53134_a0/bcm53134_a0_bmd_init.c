#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53134_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>

#include <cdk/chip/bcm53134_a0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm53134_a0_bmd.h"
#include "bcm53134_a0_internal.h"

static int
_gpic_init(int unit, int port)
{
    int ioerr = 0;
    G_PCTLr_t g_pctl;
    STS_OVERRIDE_GMIIPr_t sts_override_gp;
    STS_OVERRIDE_P5r_t sts_override_p5;

    if(port == 5) {
        /* Clear link status */
        ioerr += READ_STS_OVERRIDE_P5r(unit, &sts_override_p5);
        STS_OVERRIDE_P5r_LINK_STSf_SET(sts_override_p5, 0);
        ioerr += WRITE_STS_OVERRIDE_P5r(unit, sts_override_p5);
            
    } else {    
        /* Clear link status */
        ioerr += READ_STS_OVERRIDE_GMIIPr(unit, port, &sts_override_gp);
        STS_OVERRIDE_GMIIPr_LINK_STSf_SET(sts_override_gp, 0);
        ioerr += WRITE_STS_OVERRIDE_GMIIPr(unit, port, sts_override_gp);
    }
    /* Set forwarding state */
    ioerr += READ_G_PCTLr(unit, port, &g_pctl);
    G_PCTLr_G_MISTP_STATEf_SET(g_pctl, 5);
    /* change back from 8051 to re-write RX-discard */
    G_PCTLr_RX_DISf_SET(g_pctl, 0);
    ioerr += WRITE_G_PCTLr(unit, port, g_pctl);

    return ioerr;
}

int
bcm53134_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    VLAN_CTRL0r_t vlan_ctrl0;
    VLAN_CTRL4r_t vlan_ctrl4;
    MST_CONr_t mst_con;
    SWMODEr_t swmode;
    GMNGCFGr_t gmngcfg;
    BRCM_HDR_CTRLr_t hdrctrl;
    STS_OVERRIDE_IMPr_t stsoviimp;
    IMP_CTLr_t impstl;
    STRAP_PIN_STATUSr_t strap_value;
    int port;
    cdk_pbmp_t pbmp;
    
    BMD_CHECK_UNIT(unit);

    /* Enable VLANs */
    ioerr += READ_VLAN_CTRL0r(unit, &vlan_ctrl0);
    VLAN_CTRL0r_VLAN_ENf_SET(vlan_ctrl0, 1);
    ioerr += WRITE_VLAN_CTRL0r(unit, vlan_ctrl0);

    /* Drop packet if VLAN mismatch */
    ioerr += READ_VLAN_CTRL4r(unit, &vlan_ctrl4);
    VLAN_CTRL4r_INGR_VID_CHKf_SET(vlan_ctrl4, 1);
    ioerr += WRITE_VLAN_CTRL4r(unit, vlan_ctrl4);

    /* Enable spanning tree */
    ioerr += READ_MST_CONr(unit, &mst_con);
    MST_CONr_EN_802_1Sf_SET(mst_con, 1);
    ioerr += WRITE_MST_CONr(unit, mst_con);

    /* Configure GPICs */
    CDK_ROBO_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPIC, &pbmp);
    /* ports 4 and 7 are not valid for SF3 */
    CDK_PBMP_PORT_REMOVE(pbmp, 4);
    CDK_PBMP_PORT_REMOVE(pbmp, 7);
    CDK_PBMP_ITER(pbmp, port) {
        ioerr += _gpic_init(unit, port);
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_attach(unit, port);
        }
    }
    /* Enable management port */
    ioerr += READ_GMNGCFGr(unit, &gmngcfg);
    GMNGCFGr_FRM_MNGPf_SET(gmngcfg, 2);
    GMNGCFGr_RXBPDU_ENf_SET(gmngcfg, 1);
    ioerr += WRITE_GMNGCFGr(unit, gmngcfg);

    /* Enable frame forwarding */
    ioerr += READ_SWMODEr(unit, &swmode);
    SWMODEr_SW_FWDG_MODEf_SET(swmode, 1);
    SWMODEr_SW_FWDG_ENf_SET(swmode, 1);
    ioerr += WRITE_SWMODEr(unit, swmode);

    /* force enabling the BRCM header tag */
    ioerr += READ_BRCM_HDR_CTRLr(unit, &hdrctrl);
    BRCM_HDR_CTRLr_BRCM_HDR_ENf_SET(hdrctrl, 1);
    ioerr += WRITE_BRCM_HDR_CTRLr(unit, hdrctrl);

    ioerr += READ_STRAP_PIN_STATUSr(unit, &strap_value);
    /* check if IMP0 is RGMII mode */
    if (STRAP_PIN_STATUSr_IMP_MODEf_GET(strap_value) != IMP_MODE_RGMII) {
        return CDK_E_CONFIG;
    }

    ioerr += READ_STS_OVERRIDE_IMPr(unit, &stsoviimp);
    STS_OVERRIDE_IMPr_MII_SW_ORf_SET(stsoviimp, 1);
    STS_OVERRIDE_IMPr_DUPLX_MODEf_SET(stsoviimp, 1);
    STS_OVERRIDE_IMPr_LINK_STSf_SET(stsoviimp, 1);  
    
    STS_OVERRIDE_IMPr_SPEEDf_SET(stsoviimp, 0);  
    ioerr += WRITE_STS_OVERRIDE_IMPr(unit, stsoviimp);

    ioerr += READ_STRAP_PIN_STATUSr(unit, &strap_value);
    /* check if IMP1 is RGMII mode */
    if (STRAP_PIN_STATUSr_GMII_MODEf_GET(strap_value) != GMII_MODE_RGMII) {
        return CDK_E_CONFIG;
    }

    ioerr += READ_IMP_CTLr(unit, &impstl);
    IMP_CTLr_RX_UCST_ENf_SET(impstl, 1);
    IMP_CTLr_RX_MCST_ENf_SET(impstl, 1);
    IMP_CTLr_RX_BCST_ENf_SET(impstl, 1);
    /* back from 8051, need to disable tx/rx discard */
    IMP_CTLr_RX_DISf_SET(impstl, 0);
    IMP_CTLr_TX_DISf_SET(impstl, 0);
    ioerr += WRITE_IMP_CTLr(unit, impstl);

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53134_A0 */
