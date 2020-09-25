#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53600_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>

#include <cdk/chip/bcm53600_a0_defs.h>
#include <cdk/arch/robo_chip.h>

#include "bcm53600_a0_bmd.h"
#include "bcm53600_a0_internal.h"

#define MEM_RESET_MAX_POLL      100

static int
_epic_init(int unit, int port)
{
    int ioerr = 0;
    TH_PCTLr_t th_pctl;
    STS_OVERRIDE_Pr_t sts_override_p;

    /* Clear link status */
    ioerr += READ_STS_OVERRIDE_Pr(unit, port, &sts_override_p);
    STS_OVERRIDE_Pr_LINK_STSf_SET(sts_override_p, 0);
    ioerr += WRITE_STS_OVERRIDE_Pr(unit, port, sts_override_p);

    /* Set forwarding state */
    ioerr += READ_TH_PCTLr(unit, port, &th_pctl);
    TH_PCTLr_STP_STATEf_SET(th_pctl, 3);
    ioerr += WRITE_TH_PCTLr(unit, port, th_pctl);

    return ioerr;
}

static int
_gpic_init(int unit, int port)
{
    int ioerr = 0;
    G_PCTLr_t g_pctl;
    STS_OVERRIDE_GPr_t sts_override_gp;

    /* Clear link status */
    ioerr += READ_STS_OVERRIDE_GPr(unit, port, &sts_override_gp);
    STS_OVERRIDE_GPr_LINK_STSf_SET(sts_override_gp, 0);
    ioerr += WRITE_STS_OVERRIDE_GPr(unit, port, sts_override_gp);

    /* Set forwarding state */
    ioerr += READ_G_PCTLr(unit, port, &g_pctl);
    G_PCTLr_G_STP_STATEf_SET(g_pctl, 3);
    ioerr += WRITE_G_PCTLr(unit, port, g_pctl);

    return ioerr;
}

int
bcm53600_a0_bmd_init(int unit)
{
    int ioerr = 0;
    int rv = CDK_E_NONE;
    MST_CONr_t mst_con;
    SWMODEr_t swmode;
    GMNGCFGr_t gmngcfg;
    IMP_PCTLr_t imp_pctl;
    PHYSCAN_CTLr_t physcan_ctrl;
    RST_TABLE_MEMr_t rst_table_mem;
    NEW_CONTROLr_t new_ctrl;
    SW_XOFF_PORT_CTLr_t sw_xoff_port_ctl;
    VLAN_GLOBAL_CTLr_t vlan_global_ctl;
    GLB_VLAN_ING_FILTER_CTLr_t glb_vlan_ing;
    int port, retry;
    cdk_pbmp_t pbmp;

    BMD_CHECK_UNIT(unit);

    /* Reset all memories */
    RST_TABLE_MEMr_SET(rst_table_mem, 0xffffffff);
    ioerr += WRITE_RST_TABLE_MEMr(unit, rst_table_mem);

    /* Wait for memory reset to complete */
    for (retry = 0; retry < MEM_RESET_MAX_POLL; retry++) {
        ioerr += READ_RST_TABLE_MEMr(unit, &rst_table_mem);
        if (RST_TABLE_MEMr_GET(rst_table_mem) == 0) {
            break;
        }
    }
    if (retry >= MEM_RESET_MAX_POLL) {
        ioerr += CDK_E_TIMEOUT;
    }

    /* Disable PHY auto-scan */
    ioerr += READ_PHYSCAN_CTLr(unit, &physcan_ctrl);
    PHYSCAN_CTLr_EN_PHY_SCANf_SET(physcan_ctrl, 0);
    ioerr += WRITE_PHYSCAN_CTLr(unit, physcan_ctrl);
    
    /* bcm53600 VLAN not need to enable */
    /* Enable spanning tree */
    READ_MST_CONr(unit, &mst_con);
    MST_CONr_EN_802_1Sf_SET(mst_con, 1);
    WRITE_MST_CONr(unit, mst_con);
    
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

    ioerr += READ_IMP_PCTLr(unit, &imp_pctl);
    IMP_PCTLr_RX_UC_DLF_ENf_SET(imp_pctl, 1);
    IMP_PCTLr_RX_MC_DLF_ENf_SET(imp_pctl, 1);
    IMP_PCTLr_RX_BC_ENf_SET(imp_pctl, 1);
    ioerr += WRITE_IMP_PCTLr(unit, imp_pctl);

    /* Enable management port */
    ioerr += READ_GMNGCFGr(unit, &gmngcfg);
    GMNGCFGr_FRM_MNGPf_SET(gmngcfg, 2);
    GMNGCFGr_RX_BPDU_ENf_SET(gmngcfg, 1);
    ioerr += WRITE_GMNGCFGr(unit, gmngcfg);

    /* Flow control: enable global XOFF */ 
    ioerr += READ_NEW_CONTROLr(unit, &new_ctrl);
    NEW_CONTROLr_EN_SW_FLOW_CONf_SET(new_ctrl, 1);
    ioerr += WRITE_NEW_CONTROLr(unit, new_ctrl);

    /* Flow control: disable XOFF on each port */
    SW_XOFF_PORT_CTLr_SET(sw_xoff_port_ctl, 0, 0);
    ioerr += WRITE_SW_XOFF_PORT_CTLr(unit, sw_xoff_port_ctl);

    /* Disable shared VLANs */
    ioerr += READ_VLAN_GLOBAL_CTLr(unit, &vlan_global_ctl);
    VLAN_GLOBAL_CTLr_VID_MAC_CTRLf_SET(vlan_global_ctl, 1);
    ioerr += WRITE_VLAN_GLOBAL_CTLr(unit, vlan_global_ctl);

    /* Drop packets with unknown VLAN */    
    ioerr += READ_GLB_VLAN_ING_FILTER_CTLr(unit, &glb_vlan_ing);
    GLB_VLAN_ING_FILTER_CTLr_EN_UNREGISTERED_DROPf_SET(glb_vlan_ing, 1);
    ioerr += WRITE_GLB_VLAN_ING_FILTER_CTLr(unit, glb_vlan_ing);

    return ioerr ? CDK_E_IO : rv;
}
#endif /* CDK_CONFIG_INCLUDE_BCM53600_A0 */
