/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

#include <board/board_config.h>
#include <bmd/bmd_device.h>
#include <cdk/cdk_string.h>
#include <phy/phy_buslist.h>

#if defined(CDK_CONFIG_INCLUDE_BCM56150_A0) && CDK_CONFIG_INCLUDE_BCM56150_A0 == 1
#include <cdk/chip/bcm56150_a0_defs.h>
#define _CHIP_DYN_CONFIG        DCFG_HGD_1
#endif

static cdk_port_map_port_t _skip_ports[] = { -1 };

/* Dynamic configuration flags */
#ifndef _CHIP_DYN_CONFIG 
#define _CHIP_DYN_CONFIG        0
#endif

#ifdef CDK_CONFIG_ARCH_XGSM_INSTALLED

#include <cdk/arch/xgsm_miim.h>

static uint32_t
_phy_addr(int port)
{
    if (port >= 18 && port < 26) {
        return 0x14 + (port - 18) + CDK_XGSM_MIIM_EBUS(0);
    } 
    /* Should not get here */
    return 0;
}

static int 
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    return cdk_xgsm_miim_read(unit, addr, reg, val);
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    return cdk_xgsm_miim_write(unit, addr, reg, val);
}

static int
_phy_inst(int port)
{
    return port - 2;
}

static phy_bus_t _phy_bus_miim_ext = {
    "bcm56150_13g",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};

#endif /* CDK_CONFIG_ARCH_XGSM_INSTALLED */

static phy_bus_t *_phy_bus[] = {
#ifdef CDK_CONFIG_ARCH_XGSM_INSTALLED
#ifdef PHY_BUS_BCM56150_MIIM_INT_INSTALLED
    &phy_bus_bcm56150_miim_int,
#endif
    &_phy_bus_miim_ext,
#endif
    NULL
};

static int                     
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    phy_ctrl_t *lpc = pc;
    uint32_t rx_pol, tx_pol;
    uint32_t rx_map, tx_map;

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }
        if (CDK_STRSTR(lpc->drv->drv_name, "tsc") != NULL) {
            if ((PHY_CTRL_PHY_INST(lpc) & 0x3) == 0) {
                /* Remap Rx lanes */
                rx_map = 0x0123;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxLaneRemap,
                                    rx_map, NULL);
                PHY_VERB(lpc, ("Remap Rx lanes (0x%04"PRIx32")\n", rx_map));

                /* Remap Tx lanes */
                tx_map = 0x3210;
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxLaneRemap,
                                    tx_map, NULL);
                PHY_VERB(lpc, ("Remap Tx lanes (0x%04"PRIx32")\n", tx_map));
            }

            /* Invert Rx polarity on all lanes */
            rx_pol = 0x0000;
            rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiRxPolInvert,
                                rx_pol, NULL);
            PHY_VERB(lpc, ("Flip Rx pol (0x%04"PRIx32")\n", rx_pol));
            
            /* Invert Tx polarity on all lanes */
            tx_pol = 0x0000;
            rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxPolInvert,
                                tx_pol, NULL);
            PHY_VERB(lpc, ("Flip Tx pol (0x%04"PRIx32")\n", tx_pol));
        }
        lpc = lpc->next;
    }

    return rv;
}

static int 
_phy_init_cb(phy_ctrl_t *pc)
{
    return CDK_E_NONE;
}

#if defined(CDK_CONFIG_INCLUDE_BCM56150_A0) && CDK_CONFIG_INCLUDE_BCM56150_A0 == 1
static int
_port_modid_set(int unit, int port, int modid)
{
    int ioerr = 0;
    int lport;
    PORT_TABm_t port_tab;
    EGR_PORTr_t egr_port;

    /* Get logical port number */
    lport = (BMD_PORT_P2L(unit))(unit, port, 0);
    if (lport < 0) {
        return 0;
    }

    ioerr += READ_PORT_TABm(unit,lport, &port_tab);
    PORT_TABm_MY_MODIDf_SET(port_tab, modid);
    ioerr += WRITE_PORT_TABm(unit, lport, port_tab);

    ioerr += READ_EGR_PORTr(unit, lport, &egr_port);
    EGR_PORTr_MY_MODIDf_SET(egr_port, modid);
    ioerr += WRITE_EGR_PORTr(unit, lport, egr_port);

    return ioerr;
}
#endif

static int 
_bmd_post_init(int unit)
{
    int ioerr = 0;
#if defined(CDK_CONFIG_INCLUDE_BCM56150_A0) && CDK_CONFIG_INCLUDE_BCM56150_A0 == 1
    PORT_TABm_t port_tab;
    XLPORT_CONFIGr_t xlport_cfg;
    MODPORT_MAPm_t modport_map;
    HG_TRUNK_GROUPr_t hg_trunk_grp;
    HG_TRUNK_BITMAPr_t hg_trunk_bmap;
    USER_TRUNK_HASH_SELECTr_t ut_hash_sel;
    cdk_pbmp_t ge_pbmp, xl_pbmp;
    uint32_t hg_pbmp;
    int port, lport;
    int modid, dest_unit, dest_modid;
    int trunk_id, fval, port_idx;
 
    /* Physical to logical mapping function is required */
    if (BMD_PORT_P2L(unit) == NULL) {
        return CDK_E_INTERNAL;
    }

    /* We assume that units 0 and 1 are used */
    if (unit == 0) {
        dest_unit  = 1;
    } else if (unit == 1) {
        dest_unit  = 0;
    } else {
        return CDK_E_CONFIG;
    }
    dest_modid = BMD_MODID(dest_unit);

    /* Configure module ID */
    modid = BMD_MODID(unit);

    ioerr += _port_modid_set(unit, CMIC_PORT, modid);

    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_GPORT, &ge_pbmp);
    CDK_PBMP_ITER(ge_pbmp, port) {
        ioerr += _port_modid_set(unit, port, modid);
    }
    CDK_XGSM_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &xl_pbmp);
    CDK_PBMP_ITER(xl_pbmp, port) {
        ioerr += _port_modid_set(unit, port, modid);
        ioerr += READ_XLPORT_CONFIGr(unit, port, &xlport_cfg);
        XLPORT_CONFIGr_MY_MODIDf_SET(xlport_cfg, modid);
        ioerr += WRITE_XLPORT_CONFIGr(unit, port, xlport_cfg);
    }

    /* Prepare HiGig trunk group configuration */
    HG_TRUNK_GROUPr_CLR(hg_trunk_grp);
    HG_TRUNK_BITMAPr_CLR(hg_trunk_bmap);
    hg_pbmp = 0;
    port_idx = 0;

    CDK_PBMP_ITER(xl_pbmp, port) {
        /* Skip unused ports */
        if (BMD_PORT_PROPERTIES(unit, port) == 0) {
            continue;
        }

        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_HG) {
            /* Get logical port number */
            lport = (BMD_PORT_P2L(unit))(unit, port, 0);
            if (lport < 0) {
                return CDK_E_INTERNAL;
            }

            /* Mark port as HiGig trunk member */
            ioerr += READ_PORT_TABm(unit, lport, &port_tab);
            PORT_TABm_HIGIG_TRUNKf_SET(port_tab, 0x1);
            ioerr += WRITE_PORT_TABm(unit, lport, port_tab);

            /* Add port to HiGig trunk members */
            if ((port_idx % 2) == 0) {
                HG_TRUNK_GROUPr_HIGIG_TRUNK_PORT0f_SET(hg_trunk_grp, lport);
                HG_TRUNK_GROUPr_HIGIG_TRUNK_PORT2f_SET(hg_trunk_grp, lport);
            } else {
                HG_TRUNK_GROUPr_HIGIG_TRUNK_PORT1f_SET(hg_trunk_grp, lport);
                HG_TRUNK_GROUPr_HIGIG_TRUNK_PORT3f_SET(hg_trunk_grp, lport);
            }
            hg_pbmp |= (1 << port_idx);
        }
        port_idx++;      
    }

    /* Trunk Port Selection Criteria is ingress port number (type 7) */
    HG_TRUNK_GROUPr_HIGIG_TRUNK_RTAGf_SET(hg_trunk_grp, 7);

    /* Set the HiGig trunk member bitmap */
    HG_TRUNK_BITMAPr_HIGIG_TRUNK_BITMAPf_SET(hg_trunk_bmap, hg_pbmp);

    /* Write trunk group to hardware */
    trunk_id = 0;
    ioerr += WRITE_HG_TRUNK_GROUPr(unit, trunk_id, hg_trunk_grp);
    ioerr += WRITE_HG_TRUNK_BITMAPr(unit, trunk_id, hg_trunk_bmap);

    /* Configure egress HiGig ports for destination module ID */
    ioerr += READ_MODPORT_MAPm(unit, dest_modid, &modport_map);
    MODPORT_MAPm_HIGIG_PORT_BITMAPf_SET(modport_map, hg_pbmp);
    ioerr += WRITE_MODPORT_MAPm(unit, dest_modid, modport_map);

    /* Add XLPORTs to front panel ports if GE mode */
    CDK_PBMP_ITER(xl_pbmp, port) {
        if (BMD_PORT_PROPERTIES(unit, port) & BMD_PORT_GE) {
            CDK_PBMP_PORT_ADD(ge_pbmp, port);
        }
    }

    /* Set this for HiGig trunk load balancing */
    CDK_PBMP_ITER(ge_pbmp, port) {
        lport = (BMD_PORT_P2L(unit))(unit, port, 0);
        if (lport < 0) {
            continue;
        }
        fval = (lport % 2);
        USER_TRUNK_HASH_SELECTr_CLR(ut_hash_sel);
        USER_TRUNK_HASH_SELECTr_TRUNK_CFG_VALf_SET(ut_hash_sel, fval);
        ioerr += WRITE_USER_TRUNK_HASH_SELECTr(unit, lport, ut_hash_sel);
    }
#endif

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}

static board_chip_config_t _chip_config = {
    _skip_ports,
    _phy_bus,
    NULL,
    _CHIP_DYN_CONFIG,
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    &_chip_config,
    NULL
};

board_config_t board_bcm56150_13g = {
    "bcm56150_13g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
    &_bmd_post_init,
};
