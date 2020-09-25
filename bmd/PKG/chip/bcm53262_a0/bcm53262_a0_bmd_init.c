#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53262_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>

#include <cdk/chip/bcm53262_a0_defs.h>
#include <cdk/arch/robo_chip.h>
#include <cdk/cdk_debug.h>

#include "bcm53262_a0_internal.h"
#include "bcm53262_a0_bmd.h"

static int 
_epic_init(int unit, int port)
{
    int ioerr = 0;
    TH_PCTLr_t th_pctl;
    STS_OVERRIDE_Pr_t sts_override_p;
    DEFAULT_1Q_TAGr_t default_tag;
    uint32_t	reg_value;
        
    ioerr += READ_DEFAULT_1Q_TAGr(unit, port, &default_tag);
    reg_value = DEFAULT_1Q_TAGr_DEF_TAGf_GET(default_tag);
    reg_value = (1 | (reg_value & 0xf000));
    DEFAULT_1Q_TAGr_DEF_TAGf_SET(default_tag, reg_value);
    ioerr += WRITE_DEFAULT_1Q_TAGr(unit, port, default_tag);

    /* Clear link status */
    ioerr += READ_STS_OVERRIDE_Pr(unit, port, &sts_override_p);
    STS_OVERRIDE_Pr_LINK_STSf_SET(sts_override_p, 0);
    ioerr += WRITE_STS_OVERRIDE_Pr(unit, port, sts_override_p);

    /* Set forwarding state */
    ioerr += READ_TH_PCTLr(unit, port, &th_pctl);
    TH_PCTLr_MISTP_STATEf_SET(th_pctl, 5);
    ioerr += WRITE_TH_PCTLr(unit, port, th_pctl);
    return ioerr;
}

static int
_gpic_init(int unit, int port)
{
    int ioerr = 0;
    G_PCTLr_t g_pctl;
    STS_OVERRIDE_GPr_t sts_override_gp;
    DEFAULT_1Q_TAGr_t default_tag;
    uint32_t	reg_value;
        
    ioerr += READ_DEFAULT_1Q_TAGr(unit, port, &default_tag);
    reg_value = DEFAULT_1Q_TAGr_DEF_TAGf_GET(default_tag);
    reg_value = (1 | (reg_value & 0xf000));
    DEFAULT_1Q_TAGr_DEF_TAGf_SET(default_tag, reg_value);
    ioerr += WRITE_DEFAULT_1Q_TAGr(unit, port, default_tag);

    /* Clear link status */
    ioerr += READ_STS_OVERRIDE_GPr(unit, port, &sts_override_gp);
    STS_OVERRIDE_GPr_LINK_STSf_SET(sts_override_gp, 0);
    ioerr += WRITE_STS_OVERRIDE_GPr(unit, port, sts_override_gp);

    /* Set forwarding state */
    ioerr += READ_G_PCTLr(unit, port, &g_pctl);
    G_PCTLr_G_MISTP_STATEf_SET(g_pctl, 5);
    ioerr += WRITE_G_PCTLr(unit, port, g_pctl);
    return ioerr;
}

int
bcm53262_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    VLAN_CTRL0r_t vlan_ctrl0;
    VLAN_CTRL4r_t vlan_ctrl4;
    MST_CONr_t mst_con;
    LED_FUNC1_CTLr_t led_func1_ctl;
    SWMODEr_t swmode;
    STS_OVERRIDE_IMPr_t stsoviimp;
    IMP_PCTLr_t imp_pctl;
    GMNGCFGr_t gmngcfg;
    FCON_Q0_100_TH_CTRL_1r_t fcon_q0_ctrl_1r;
    FCON_Q0_100_TH_CTRL_2r_t fcon_q0_ctrl_2r;
    FCON_GLOB_TH_CTRL_1r_t fcon_glob_ctrl_1r;
    FCON_GLOB_TH_CTRL_2r_t fcon_glob_ctrl_2r;
    FCON_FLOWMIXr_t fcon_flowmix;
    FCON_MISC_TXFLOW_CTRLr_t fcon_misc_txflow_ctrl;
    FCON_Q1_100_TH_CTRL_1r_t fcon_q1_ctrl_1r;
    FCON_Q1_100_TH_CTRL_2r_t fcon_q1_ctrl_2r;
    FCON_Q2_100_TH_CTRL_1r_t fcon_q2_ctrl_1r;
    FCON_Q2_100_TH_CTRL_2r_t fcon_q2_ctrl_2r;
    FCON_Q3_100_TH_CTRL_1r_t fcon_q3_ctrl_1r;
    FCON_Q3_100_TH_CTRL_2r_t fcon_q3_ctrl_2r;
    FCON_RX_FCON_CTRLr_t fcon_rx_fcon_ctrl;
    FCON_DLF_TH_CTRLr_t fcon_dlf_th_ctrl;
    FCON_BCST_TH_CTRLr_t fcon_bcst_th_ctrl;
    TOTAL_HYST_THRESH_Q1r_t total_hyst_th_q1;
    TOTAL_DROP_THRESH_Q1r_t total_drop_th_q1;
    TOTAL_HYST_THRESH_Q2r_t total_hyst_th_q2;
    TOTAL_DROP_THRESH_Q2r_t total_drop_th_q2;
    TOTAL_HYST_THRESH_Q3r_t total_hyst_th_q3;
    TOTAL_DROP_THRESH_Q3r_t total_drop_th_q3;
    TOTAL_DLF_DROP_THRESH_Q1r_t total_dlf_drop_th_q1;
    TOTAL_DLF_DROP_THRESH_Q2r_t total_dlf_drop_th_q2;
    TOTAL_DLF_DROP_THRESH_Q3r_t total_dlf_drop_th_q3;
    PHYSCAN_CTLr_t physcan_ctrl;
    int port;
    cdk_pbmp_t pbmp;

    BMD_CHECK_UNIT(unit);
    /* Configure EPICs */
    CDK_ROBO_BLKTYPE_PBMP_GET(unit, BLKTYPE_EPIC, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        ioerr += _epic_init(unit, port);
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_attach(unit, port);
        }
    }
    /* Configure GPICs */
    CDK_ROBO_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPIC, &pbmp);
    CDK_PBMP_ITER(pbmp, port) {
        ioerr += _gpic_init(unit, port);
        if (CDK_SUCCESS(rv)) {
            rv = bmd_phy_attach(unit, port);
        }
    }
    /* Enable frame forwarding */
    READ_SWMODEr(unit, &swmode);
    SWMODEr_SW_FWDG_MODEf_SET(swmode, 1);
    SWMODEr_SW_FWDG_ENf_SET(swmode, 1);
    WRITE_SWMODEr(unit, swmode);

    READ_STS_OVERRIDE_IMPr(unit, &stsoviimp);
    STS_OVERRIDE_IMPr_SW_ORDf_SET(stsoviimp, 1);
    STS_OVERRIDE_IMPr_DUPLX_MODEf_SET(stsoviimp, 1);
    STS_OVERRIDE_IMPr_LINK_STSf_SET(stsoviimp, 1);  
    STS_OVERRIDE_IMPr_GIGA_SPEEDf_SET(stsoviimp, 1);  
    WRITE_STS_OVERRIDE_IMPr(unit, stsoviimp);

    /*
     * Enable All flow (unicast, multicast, broadcast)
     * in MII port of mgmt chip
     */
    READ_IMP_PCTLr(unit, &imp_pctl);
    IMP_PCTLr_MIRX_UC_ENf_SET(imp_pctl, 1);
    IMP_PCTLr_MIRX_MC_ENf_SET(imp_pctl, 1);
    IMP_PCTLr_MIRX_BC_ENf_SET(imp_pctl, 1);  
    WRITE_IMP_PCTLr(unit, imp_pctl);

    /* Enable management port */
    READ_GMNGCFGr(unit, &gmngcfg);
    GMNGCFGr_FRM_MNGPf_SET(gmngcfg, 2);
    GMNGCFGr_RXBPDU_ENf_SET(gmngcfg, 1);
    WRITE_GMNGCFGr(unit, gmngcfg);

    /* MMU init */
    READ_FCON_Q0_100_TH_CTRL_1r(unit, &fcon_q0_ctrl_1r);
    FCON_Q0_100_TH_CTRL_1r_BT100_HYST_THRSf_SET(fcon_q0_ctrl_1r, 0x13);
    FCON_Q0_100_TH_CTRL_1r_BT100_PAUS_THRSf_SET(fcon_q0_ctrl_1r, 0x1c);
    WRITE_FCON_Q0_100_TH_CTRL_1r(unit, fcon_q0_ctrl_1r);

    READ_FCON_Q0_100_TH_CTRL_2r(unit, &fcon_q0_ctrl_2r);
    FCON_Q0_100_TH_CTRL_2r_BT100_DROP_THRSf_SET(fcon_q0_ctrl_2r, 0x98);
    FCON_Q0_100_TH_CTRL_2r_BT100_MCDROP_THRSf_SET(fcon_q0_ctrl_2r, 0x73);
    WRITE_FCON_Q0_100_TH_CTRL_2r(unit, fcon_q0_ctrl_2r);

    READ_FCON_GLOB_TH_CTRL_1r(unit, &fcon_glob_ctrl_1r);
    FCON_GLOB_TH_CTRL_1r_FCON_GLOB_HYST_THf_SET(fcon_glob_ctrl_1r, 0x44);
    FCON_GLOB_TH_CTRL_1r_FCON_GLOB_PAUSE_THf_SET(fcon_glob_ctrl_1r, 0x7e);
    WRITE_FCON_GLOB_TH_CTRL_1r(unit, fcon_glob_ctrl_1r);

    READ_FCON_GLOB_TH_CTRL_2r(unit, &fcon_glob_ctrl_2r);
    FCON_GLOB_TH_CTRL_2r_FCON_GLOB_DROP_THf_SET(fcon_glob_ctrl_2r, 0x9b);
    FCON_GLOB_TH_CTRL_2r_FCON_GLOB_MCDROP_THf_SET(fcon_glob_ctrl_2r, 0x7e);
    WRITE_FCON_GLOB_TH_CTRL_2r(unit, fcon_glob_ctrl_2r);

    READ_FCON_FLOWMIXr(unit, &fcon_flowmix);
    FCON_FLOWMIXr_QOS_RSRV_QUOTA_OPTf_SET(fcon_flowmix, 0x3);
    FCON_FLOWMIXr_EN_PARKING_PREVENTIONf_SET(fcon_flowmix, 1);
    FCON_FLOWMIXr_EN_MCAST_BLANCEf_SET(fcon_flowmix, 1);
    FCON_FLOWMIXr_EN_MCAST_DROPf_SET(fcon_flowmix, 1);
    FCON_FLOWMIXr_EN_UCAST_DROPf_SET(fcon_flowmix, 1);
    FCON_FLOWMIXr_EN_TXQ_PAUSEf_SET(fcon_flowmix, 1);
    FCON_FLOWMIXr_EN_RX_DROPf_SET(fcon_flowmix, 0);
    FCON_FLOWMIXr_EN_RX_PAUSEf_SET(fcon_flowmix, 0);
    WRITE_FCON_FLOWMIXr(unit, fcon_flowmix);

    READ_FCON_MISC_TXFLOW_CTRLr(unit, &fcon_misc_txflow_ctrl);
    FCON_MISC_TXFLOW_CTRLr_RESERVE_BLANCEf_SET(fcon_misc_txflow_ctrl, 0x1);
    WRITE_FCON_MISC_TXFLOW_CTRLr(unit, fcon_misc_txflow_ctrl);

    READ_FCON_Q1_100_TH_CTRL_1r(unit, &fcon_q1_ctrl_1r);
    FCON_Q1_100_TH_CTRL_1r_BT100_HYST_THRSf_SET(fcon_q1_ctrl_1r, 0x13);
    FCON_Q1_100_TH_CTRL_1r_BT100_PAUS_THRSf_SET(fcon_q1_ctrl_1r, 0x1c);
    WRITE_FCON_Q1_100_TH_CTRL_1r(unit, fcon_q1_ctrl_1r);

    READ_FCON_Q1_100_TH_CTRL_2r(unit, &fcon_q1_ctrl_2r);
    FCON_Q1_100_TH_CTRL_2r_BT100_DROP_THRSf_SET(fcon_q1_ctrl_2r, 0x98);
    FCON_Q1_100_TH_CTRL_2r_BT100_MCDROP_THRSf_SET(fcon_q1_ctrl_2r, 0x73);
    WRITE_FCON_Q1_100_TH_CTRL_2r(unit, fcon_q1_ctrl_2r);

    READ_FCON_Q2_100_TH_CTRL_1r(unit, &fcon_q2_ctrl_1r);
    FCON_Q2_100_TH_CTRL_1r_BT100_HYST_THRSf_SET(fcon_q2_ctrl_1r, 0x13);
    FCON_Q2_100_TH_CTRL_1r_BT100_PAUS_THRSf_SET(fcon_q2_ctrl_1r, 0x1c);
    WRITE_FCON_Q2_100_TH_CTRL_1r(unit, fcon_q2_ctrl_1r);

    READ_FCON_Q2_100_TH_CTRL_2r(unit, &fcon_q2_ctrl_2r);
    FCON_Q2_100_TH_CTRL_2r_BT100_DROP_THRSf_SET(fcon_q2_ctrl_2r, 0x98);
    FCON_Q2_100_TH_CTRL_2r_BT100_MCDROP_THRSf_SET(fcon_q2_ctrl_2r, 0x73);
    WRITE_FCON_Q2_100_TH_CTRL_2r(unit, fcon_q2_ctrl_2r);

    READ_FCON_Q3_100_TH_CTRL_1r(unit, &fcon_q3_ctrl_1r);
    FCON_Q3_100_TH_CTRL_1r_BT100_HYST_THRSf_SET(fcon_q3_ctrl_1r, 0x13);
    FCON_Q3_100_TH_CTRL_1r_BT100_PAUS_THRSf_SET(fcon_q3_ctrl_1r, 0x1c);
    WRITE_FCON_Q3_100_TH_CTRL_1r(unit, fcon_q3_ctrl_1r);

    READ_FCON_Q3_100_TH_CTRL_2r(unit, &fcon_q3_ctrl_2r);
    FCON_Q3_100_TH_CTRL_2r_BT100_DROP_THRSf_SET(fcon_q3_ctrl_2r, 0x98);
    FCON_Q3_100_TH_CTRL_2r_BT100_MCDROP_THRSf_SET(fcon_q3_ctrl_2r, 0x73);
    WRITE_FCON_Q3_100_TH_CTRL_2r(unit, fcon_q3_ctrl_2r);

    READ_FCON_RX_FCON_CTRLr(unit, &fcon_rx_fcon_ctrl);
    FCON_RX_FCON_CTRLr_EN_UNPAUSE_HDLf_SET(fcon_rx_fcon_ctrl, 1);
    WRITE_FCON_RX_FCON_CTRLr(unit, fcon_rx_fcon_ctrl);

    READ_FCON_DLF_TH_CTRLr(unit, &fcon_dlf_th_ctrl);
    FCON_DLF_TH_CTRLr_TOTAL_INDV_DLFTH_DROPf_SET(fcon_dlf_th_ctrl, 0x7e);
    WRITE_FCON_DLF_TH_CTRLr(unit, fcon_dlf_th_ctrl);

    READ_FCON_BCST_TH_CTRLr(unit, &fcon_bcst_th_ctrl);
    FCON_BCST_TH_CTRLr_TOTAL_INDV_BCSTTH_DROPf_SET(fcon_bcst_th_ctrl, 0x7e);
    WRITE_FCON_BCST_TH_CTRLr(unit, fcon_bcst_th_ctrl);

    READ_TOTAL_HYST_THRESH_Q1r(unit, &total_hyst_th_q1);
    TOTAL_HYST_THRESH_Q1r_TL_HYST_TH_Q1f_SET(total_hyst_th_q1, 0x46);
    TOTAL_HYST_THRESH_Q1r_TL_PAUSE_TH_Q1f_SET(total_hyst_th_q1, 0x80);
    WRITE_TOTAL_HYST_THRESH_Q1r(unit, total_hyst_th_q1);

    READ_TOTAL_DROP_THRESH_Q1r(unit, &total_drop_th_q1);
    TOTAL_DROP_THRESH_Q1r_TL_DROP_TH_Q1f_SET(total_drop_th_q1, 0x9d);
    TOTAL_DROP_THRESH_Q1r_RESERVED_Rf_SET(total_drop_th_q1, 0x7e);
    WRITE_TOTAL_DROP_THRESH_Q1r(unit, total_drop_th_q1);

    READ_TOTAL_HYST_THRESH_Q2r(unit, &total_hyst_th_q2);
    TOTAL_HYST_THRESH_Q2r_TL_HYST_TH_Q2f_SET(total_hyst_th_q2, 0x48);
    TOTAL_HYST_THRESH_Q2r_TL_PAUSE_TH_Q2f_SET(total_hyst_th_q2, 0x82);
    WRITE_TOTAL_HYST_THRESH_Q2r(unit, total_hyst_th_q2);

    READ_TOTAL_DROP_THRESH_Q2r(unit, &total_drop_th_q2);
    TOTAL_DROP_THRESH_Q2r_TL_DROP_TH_Q2f_SET(total_drop_th_q2, 0x9f);
    TOTAL_DROP_THRESH_Q2r_RESERVED_Rf_SET(total_drop_th_q2, 0x7e);
    WRITE_TOTAL_DROP_THRESH_Q2r(unit, total_drop_th_q2);

    READ_TOTAL_HYST_THRESH_Q3r(unit, &total_hyst_th_q3);
    TOTAL_HYST_THRESH_Q3r_TL_HYST_TH_Q3f_SET(total_hyst_th_q3, 0x4a);
    TOTAL_HYST_THRESH_Q3r_TL_PAUSE_TH_Q3f_SET(total_hyst_th_q3, 0x84);
    WRITE_TOTAL_HYST_THRESH_Q3r(unit, total_hyst_th_q3);

    READ_TOTAL_DROP_THRESH_Q3r(unit, &total_drop_th_q3);
    TOTAL_DROP_THRESH_Q3r_TL_DROP_TH_Q3f_SET(total_drop_th_q3, 0xa1);
    TOTAL_DROP_THRESH_Q3r_RESERVED_Rf_SET(total_drop_th_q3, 0x7e);
    WRITE_TOTAL_DROP_THRESH_Q3r(unit, total_drop_th_q3);

    READ_TOTAL_DLF_DROP_THRESH_Q1r(unit, &total_dlf_drop_th_q1);
    TOTAL_DLF_DROP_THRESH_Q1r_TOTAL_DLF_DROP_THRESH_Q1f_SET(total_dlf_drop_th_q1, 0x7e);
    TOTAL_DLF_DROP_THRESH_Q1r_TOTAL_BC_DROP_THRESH_Q1f_SET(total_dlf_drop_th_q1, 0x7e);
    WRITE_TOTAL_DLF_DROP_THRESH_Q1r(unit, total_dlf_drop_th_q1);

    READ_TOTAL_DLF_DROP_THRESH_Q2r(unit, &total_dlf_drop_th_q2);
    TOTAL_DLF_DROP_THRESH_Q2r_TOTAL_DLF_DROP_THRESH_Q2f_SET(total_dlf_drop_th_q2, 0x7e);
    TOTAL_DLF_DROP_THRESH_Q2r_TOTAL_BC_DROP_THRESH_Q2f_SET(total_dlf_drop_th_q2, 0x7e);
    WRITE_TOTAL_DLF_DROP_THRESH_Q2r(unit, total_dlf_drop_th_q2);

    READ_TOTAL_DLF_DROP_THRESH_Q3r(unit, &total_dlf_drop_th_q3);
    TOTAL_DLF_DROP_THRESH_Q3r_TOTAL_DLF_DROP_THRESH_Q3f_SET(total_dlf_drop_th_q3, 0x7e);
    TOTAL_DLF_DROP_THRESH_Q3r_TOTAL_BC_DROP_THRESH_Q3f_SET(total_dlf_drop_th_q3, 0x7e);
    WRITE_TOTAL_DLF_DROP_THRESH_Q3r(unit, total_dlf_drop_th_q3);

    /* Disable PHY auto-scan */
    READ_PHYSCAN_CTLr(unit, &physcan_ctrl);
    PHYSCAN_CTLr_EN_PHY_SCANf_SET(physcan_ctrl, 0);
    WRITE_PHYSCAN_CTLr(unit, physcan_ctrl);

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

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53262_A0 */
