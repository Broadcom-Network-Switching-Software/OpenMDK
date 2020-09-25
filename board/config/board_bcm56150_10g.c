/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

#include <board/board_config.h>
#include <cdk/cdk_string.h>
#include <phy/phy_buslist.h>

#if defined(CDK_CONFIG_INCLUDE_BCM56150_A0) && CDK_CONFIG_INCLUDE_BCM56150_A0 == 1
#include <cdk/chip/bcm56150_a0_defs.h>
#endif

static cdk_port_map_port_t _skip_ports[] = { -1 };

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
    "bcm56150_10g",
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

static board_chip_config_t _chip_config = {
    _skip_ports,
    _phy_bus,
    NULL,
    _CHIP_DYN_CONFIG,
};

static board_chip_config_t *_chip_configs[] = {
    &_chip_config,
    NULL
};

board_config_t board_bcm56150_10g = {
    "bcm56150_10g",
    _chip_configs,
    &_phy_reset_cb,
    &_phy_init_cb,
};
