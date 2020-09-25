/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default internal XGXS PHY access functions.
 */

#include <cdk_config.h>

#if defined(CDK_CONFIG_INCLUDE_BCM56860_A0) && CDK_CONFIG_INCLUDE_BCM56860_A0 == 1

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsm_miim.h>
#include <cdk/chip/bcm56860_a0_defs.h>

#include <phy/phy.h>

#define TSC_REG_ADDR_TSCID_SET(_phy_reg, _phyad)    \
                            ((_phy_reg) |= ((_phyad) & 0x1f) << 19)

static int
_mdio_addr_to_port(uint32_t phy_addr)
{
    /* Internal phy address:
     * Bus 0: PHYAD 1-20 are mapped to physical ports   1-20
     * Bus 1: PHYAD 1-24 are mapped to physical ports  21-44
     * Bus 2: PHYAD 1-20 are mapped to physical ports  45-64
     * Bus 3: PHYAD 1-20 are mapped to physical ports  65-84
     * Bus 4: PHYAD 1-24 are mapped to physical ports  85-108
     * Bus 5: PHYAD 1-20 are mapped to physical ports 109-128
     */

    uint32_t bus = CDK_XGSM_MIIM_IBUS_NUM(phy_addr);
    
    if (bus == 5) {
        return (phy_addr & 0x1f) + 108;
    } else if (bus == 4) {
        return (phy_addr & 0x1f) + 84;
    } else if (bus == 3) {
        return (phy_addr & 0x1f) + 64;
    } else if (bus == 2) {
        return (phy_addr & 0x1f) + 44;
    } else if (bus == 1) {
        return (phy_addr & 0x1f) + 20;
    } else {
        return (phy_addr & 0x1f) + 1;
    }
}

static int
_sbus_mdio_read(int unit, uint32_t phy_addr, 
                uint32_t phy_reg, uint32_t *phy_data)
{
    int rv = CDK_E_NONE;
    int port, idx;
    uint32_t data[16];
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;

    port = _mdio_addr_to_port(phy_addr);
    TSC_REG_ADDR_TSCID_SET(phy_reg, phy_addr);

    CDK_MEMSET(data, 0, sizeof(data));
    data[0] = phy_reg & 0xffffffff;
    data[2] = 0; /* for TSC register READ */
        
    for (idx = 0; idx < 4; idx++) {
        XLPORT_WC_UCMEM_DATAm_SET(xl_ucmem_data, idx, data[idx]);
    }
    rv = WRITE_XLPORT_WC_UCMEM_DATAm(unit, 0, xl_ucmem_data, port);
    
    if (CDK_SUCCESS(rv)) {
        rv = READ_XLPORT_WC_UCMEM_DATAm(unit, 0, &xl_ucmem_data, port);
    }
    *phy_data = XLPORT_WC_UCMEM_DATAm_GET(xl_ucmem_data, 0);

    return rv;
}

static int
_sbus_mdio_write(int unit, uint32_t phy_addr,
                 uint32_t phy_reg, uint32_t phy_data)
{
    int rv = CDK_E_NONE;
    int port, idx;
    uint32_t data[16];
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;

    port = _mdio_addr_to_port(phy_addr);
    TSC_REG_ADDR_TSCID_SET(phy_reg, phy_addr);

    if ((phy_data & 0xffff0000) == 0) {
        phy_data |= 0xffff0000;
    }
    
    CDK_MEMSET(data, 0, sizeof(data));
    data[0] = phy_reg & 0xffffffff;
    data[1] = ((phy_data & 0xffff) << 16) | 
              ((~phy_data & 0xffff0000) >> 16);
    data[2] = 1; /* for TSC register write */

    for (idx = 0; idx < 4; idx++) {
        XLPORT_WC_UCMEM_DATAm_SET(xl_ucmem_data, idx, data[idx]);
    }
    rv = WRITE_XLPORT_WC_UCMEM_DATAm(unit, 0, xl_ucmem_data, port);
    
    return rv;
}

static uint32_t
_phy_addr(int port)
{
    /* Internal phy address:
     * Bus 0: PHYAD 1-20 are mapped to physical ports   1-20
     * Bus 1: PHYAD 1-24 are mapped to physical ports  21-44
     * Bus 2: PHYAD 1-20 are mapped to physical ports  45-64
     * Bus 3: PHYAD 1-20 are mapped to physical ports  65-84
     * Bus 4: PHYAD 1-24 are mapped to physical ports  85-108
     * Bus 5: PHYAD 1-20 are mapped to physical ports 109-128
     */
    if (port > 108) {
        return ((port - 109) & ~0x3) + 1 + CDK_XGSM_MIIM_IBUS(5);
    }
    if (port > 84) {
        return ((port - 85) & ~0x3) + 1 + CDK_XGSM_MIIM_IBUS(4);
    }
    if (port > 64) {
        return ((port - 65) & ~0x3) + 1 + CDK_XGSM_MIIM_IBUS(3);
    }
    if (port > 44) {
        return ((port - 45) & ~0x3) + 1 + CDK_XGSM_MIIM_IBUS(2);
    }
    if (port > 20) {
        return ((port - 21) & ~0x3) + 1 + CDK_XGSM_MIIM_IBUS(1);
    }
    return ((port - 1) & ~0x3) + 1 + CDK_XGSM_MIIM_IBUS(0);
}

static int 
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    return _sbus_mdio_read(unit, addr, reg, val);
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    return _sbus_mdio_write(unit, addr, reg, val);
}

static int
_phy_inst(int port)
{
    return port - 1;
}

static uint32_t
_bus_cap(uint32_t addr)
{
    return (PHY_BUS_CAP_WR_MASK);
}

phy_bus_t phy_bus_bcm56860_miim_int = {
    "bcm56860_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst,
    _bus_cap
};

#else

/* ISO C forbids empty source files */
int bcm56860_miim_int_not_empty;

#endif /* CDK_CONFIG_INCLUDE_BCM56860_A0 */
