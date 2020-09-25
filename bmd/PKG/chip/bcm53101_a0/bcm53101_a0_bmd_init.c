#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53101_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>

#include <cdk/chip/bcm53101_a0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm53101_a0_bmd.h"
#include "bcm53101_a0_internal.h"

static int
_epic_init(int unit, int port)
{
    int ioerr = 0;
    G_PCTLr_t g_pctl;
    STS_OVERRIDE_GMIIPr_t sts_override_gp;

    /* Clear link status */
    ioerr += READ_STS_OVERRIDE_GMIIPr(unit, port, &sts_override_gp);
    STS_OVERRIDE_GMIIPr_LINK_STSf_SET(sts_override_gp, 0);
    ioerr += WRITE_STS_OVERRIDE_GMIIPr(unit, port, sts_override_gp);

    /* Set forwarding state */
    ioerr += READ_G_PCTLr(unit, port, &g_pctl);
    G_PCTLr_G_MISTP_STATEf_SET(g_pctl, 5);
    ioerr += WRITE_G_PCTLr(unit, port, g_pctl);

    return ioerr;
}

int
bcm53101_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    VLAN_CTRL0r_t vlan_ctrl0;
    VLAN_CTRL4r_t vlan_ctrl4;
    MST_CONr_t mst_con;
    LED_FUNC1_CTLr_t led_func1_ctl;
    SWMODEr_t swmode;
    GMNGCFGr_t gmngcfg;
    BRCM_HDR_CTRLr_t hdrctrl;
    STRAP_VALUEr_t strap_value;
    SWITCH_CTRLr_t switch_ctrl;
    IMP_RGMII_CTL_GPr_t imp_rgmii_ctl_gp;
    STS_OVERRIDE_IMPr_t stsoviimp;
    IMP_CTLr_t impstl;
    uint32_t temp, rgmii = 0;
    CTRL_REGr_t ctrl_reg;
        
    int port;
    cdk_pbmp_t pbmp;

    BMD_CHECK_UNIT(unit);

    /* Enable VLANs */
    READ_VLAN_CTRL0r(unit, &vlan_ctrl0);
    VLAN_CTRL0r_VLAN_ENf_SET(vlan_ctrl0, 1);
    WRITE_VLAN_CTRL0r(unit, vlan_ctrl0);

    /* Drop packet if VLAN mismatch */
    READ_VLAN_CTRL4r(unit, &vlan_ctrl4);
    VLAN_CTRL4r_INGR_VID_CHKf_SET(vlan_ctrl4, 1);
    WRITE_VLAN_CTRL4r(unit, vlan_ctrl4);

    /* Enable spanning tree */
    READ_MST_CONr(unit, &mst_con);
    MST_CONr_EN_802_1Sf_SET(mst_con, 1);
    WRITE_MST_CONr(unit, mst_con);

    /* Configure LEDs */
    LED_FUNC1_CTLr_SET(led_func1_ctl, 0x4320);
    WRITE_LED_FUNC1_CTLr(unit, led_func1_ctl);

    /* Fixed for port 5 with external phy */
    READ_CTRL_REGr(unit, &ctrl_reg);
    CTRL_REGr_MDC_TIMING_ENHf_SET(ctrl_reg, 1);
    WRITE_CTRL_REGr(unit, ctrl_reg);

    /* Configure GPICs */
    CDK_ROBO_BLKTYPE_PBMP_GET(unit, BLKTYPE_EPIC, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        ioerr += _epic_init(unit, port);
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_attach(unit, port);
        }
    }
    
    /* Enable management port */
    READ_GMNGCFGr(unit, &gmngcfg);
    GMNGCFGr_FRM_MNGPf_SET(gmngcfg, 2);
    WRITE_GMNGCFGr(unit, gmngcfg);

    /* Enable frame forwarding */
    READ_SWMODEr(unit, &swmode);
    SWMODEr_SW_FWDG_MODEf_SET(swmode, 1);
    SWMODEr_SW_FWDG_ENf_SET(swmode, 1);
    WRITE_SWMODEr(unit, swmode);

    /* force enabling the BRCM header tag */
    READ_BRCM_HDR_CTRLr(unit, &hdrctrl);
    BRCM_HDR_CTRLr_BRCM_HDR_ENf_SET(hdrctrl, 1);
    WRITE_BRCM_HDR_CTRLr(unit, hdrctrl);

    /* check if RGMII mode */
    READ_STRAP_VALUEr(unit, &strap_value);
    if (STRAP_VALUEr_FINAL_MII1_MODEf_GET(strap_value) == GMII_MODE_RGMII) {
        rgmii = 1;
    }

    if (rgmii) {
        /* Select 2.5V as MII voltage */
        READ_SWITCH_CTRLr(unit, &switch_ctrl);
        temp = 1; /* 2.5V */
        SWITCH_CTRLr_MII1_VOL_SELf_SET(switch_ctrl, temp);
        WRITE_SWITCH_CTRLr(unit, switch_ctrl);
    
        /* Enable RGMII tx/rx clock delay mode */
        READ_IMP_RGMII_CTL_GPr(unit, &imp_rgmii_ctl_gp); 
        IMP_RGMII_CTL_GPr_RXC_DLL_DLY_ENf_SET(imp_rgmii_ctl_gp, 1); 
        IMP_RGMII_CTL_GPr_TXC_DLL_DLY_ENf_SET(imp_rgmii_ctl_gp, 1); 
        WRITE_IMP_RGMII_CTL_GPr(unit, imp_rgmii_ctl_gp);
    }
    /* force enabling the BRCM header tag */
    READ_STS_OVERRIDE_IMPr(unit, &stsoviimp);
    STS_OVERRIDE_IMPr_MII_SW_ORf_SET(stsoviimp, 1);
    STS_OVERRIDE_IMPr_DUPLX_MODEf_SET(stsoviimp, 1);
    STS_OVERRIDE_IMPr_LINK_STSf_SET(stsoviimp, 1);  
    
    if (rgmii) {
        /* Speed 1000MB */
        STS_OVERRIDE_IMPr_SPEEDf_SET(stsoviimp, 2);  
    } else {
        STS_OVERRIDE_IMPr_SPEEDf_SET(stsoviimp, 0);  
    }          
    WRITE_STS_OVERRIDE_IMPr(unit, stsoviimp);

    /* force enabling the BRCM header tag */
    READ_IMP_CTLr(unit, &impstl);
    IMP_CTLr_RX_UCST_ENf_SET(impstl, 1);
    IMP_CTLr_RX_MCST_ENf_SET(impstl, 1);
    IMP_CTLr_RX_BCST_ENf_SET(impstl, 1);
    WRITE_IMP_CTLr(unit, impstl);

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53101_A0 */
