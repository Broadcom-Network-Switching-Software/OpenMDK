/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default internal XGXS PHY access functions.
 */

#include <cdk_config.h>

#if defined(CDK_CONFIG_INCLUDE_BCM56670_A0) && CDK_CONFIG_INCLUDE_BCM56670_A0 == 1

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/chip/bcm56670_a0_defs.h>
#include <cdk/cdk_debug.h>
#include <../PKG/chip/bcm56670_a0/bcm56670_a0_internal.h>

#include <phy/phy.h>

#define TSC_REG_ADDR_TSCID_SET(_phy_reg, _phyad)    \
                            ((_phy_reg) |= ((_phyad) & 0x1f) << 19)

static int
_mdio_addr_to_port(uint32_t phy_addr)
{
    uint32_t mdio_addr = (phy_addr & 0x1f);
    /*
     * Internal phy address:
     * bus 0 phy 1 to 28 are mapped to Physical port 1 to 28
     * bus 1 phy 1 to 20 are mapped to Physical port 29 to 48
     * bus 2 phy 1 to 24 are mapped to Physical port 49 to 72
     */
    uint32_t bus = CDK_XGSD_MIIM_IBUS_NUM(phy_addr);

    if (bus == 2) {
        return mdio_addr + 20;
    } else if (bus == 1) {
        if (mdio_addr < 0x9) {
            return mdio_addr + 12;
        } else {
            return mdio_addr + 36;
        }
    } else {
        if (mdio_addr < 0xd) {
            return mdio_addr;
        } else {
            return mdio_addr + 40;
        }
    }
}

static int
_sbus_mdio_read(int unit, uint32_t phy_addr,
                uint32_t phy_reg, uint32_t *phy_data)
{
    int rv = CDK_E_NONE;
    int port, idx;
    uint32_t data[4];
    CPMPORT_WC_UCMEM_DATAm_t cpm_ucmem_data;
    CLPORT_WC_UCMEM_DATAm_t cl_ucmem_data;
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;

    port = _mdio_addr_to_port(phy_addr);

    TSC_REG_ADDR_TSCID_SET(phy_reg, phy_addr);

    CDK_MEMSET(data, 0, sizeof(data));
    data[0] = phy_reg & 0xffffffff;

    if (IS_FALCON(unit, port)) {
        if (IS_CPRI(unit, port)) {
            for (idx = 0; idx < 4; idx++) {
                CPMPORT_WC_UCMEM_DATAm_SET(cpm_ucmem_data, idx, data[idx]);
            }
            rv = WRITE_CPMPORT_WC_UCMEM_DATAm(unit, port >> 4, 0, cpm_ucmem_data);
            if (CDK_SUCCESS(rv)) {
                rv = READ_CPMPORT_WC_UCMEM_DATAm(unit, port >> 4, 0, &cpm_ucmem_data);
            }
            *phy_data = CPMPORT_WC_UCMEM_DATAm_GET(cpm_ucmem_data, 1);
        } else {
            for (idx = 0; idx < 4; idx++) {
                CLPORT_WC_UCMEM_DATAm_SET(cl_ucmem_data, idx, data[idx]);
            }
            rv = WRITE_CLPORT_WC_UCMEM_DATAm(unit, 0, cl_ucmem_data, port);
    
            if (CDK_SUCCESS(rv)) {
                rv = READ_CLPORT_WC_UCMEM_DATAm(unit, 0, &cl_ucmem_data, port);
            }
            *phy_data = CLPORT_WC_UCMEM_DATAm_GET(cl_ucmem_data, 1);
        }
    } else {
        for (idx = 0; idx < 4; idx++) {
            XLPORT_WC_UCMEM_DATAm_SET(xl_ucmem_data, idx, data[idx]);
        }
        rv = WRITE_XLPORT_WC_UCMEM_DATAm(unit, 0, xl_ucmem_data, port);

        if (CDK_SUCCESS(rv)) {
            rv = READ_XLPORT_WC_UCMEM_DATAm(unit, 0, &xl_ucmem_data, port);
        }
        *phy_data = XLPORT_WC_UCMEM_DATAm_GET(xl_ucmem_data, 1);
    }

    return rv;
}

static int
_sbus_mdio_write(int unit, uint32_t phy_addr,
                 uint32_t phy_reg, uint32_t phy_data)
{
    int rv = CDK_E_NONE;
    int port, idx;
    uint32_t data[4];
    CPMPORT_WC_UCMEM_DATAm_t cpm_ucmem_data;
    CLPORT_WC_UCMEM_DATAm_t cl_ucmem_data;
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

    if (IS_FALCON(unit, port)) {
        if (IS_CPRI(unit, port)) {
            for (idx = 0; idx < 4; idx++) {
                CPMPORT_WC_UCMEM_DATAm_SET(cpm_ucmem_data, idx, data[idx]);
            }
            rv = WRITE_CPMPORT_WC_UCMEM_DATAm(unit, port >> 4, 0, cpm_ucmem_data);
        } else {
            for (idx = 0; idx < 4; idx++) {
                CLPORT_WC_UCMEM_DATAm_SET(cl_ucmem_data, idx, data[idx]);
            }
            rv = WRITE_CLPORT_WC_UCMEM_DATAm(unit, 0, cl_ucmem_data, port);
            }
    } else {
        for (idx = 0; idx < 4; idx++) {
            XLPORT_WC_UCMEM_DATAm_SET(xl_ucmem_data, idx, data[idx]);
        }
        rv = WRITE_XLPORT_WC_UCMEM_DATAm(unit, 0, xl_ucmem_data, port);
    }

    return rv;
}


static uint32_t
_phy_addr(int port)
{
    uint32_t phy_addr = 0;
    /*
     * Internal phy address:
     * bus 0 (CPRI port) phy 1 to 12 are mapped to Physical port 1 to 12
     *                   phy 13 to 24 are mapped to Physical port 53 to 64
     * bus 1 (CL   port) phy 1 to  8 are mapped to Physical port 13 to 20
     *                   phy 9 to 16 are mapped to Physical port 45 to 52
     * bus 2 phy 1 to 24 are mapped to Physical port 21 to 44
     */

    if (port >= 53) {
        phy_addr = ((port - 39) & ~0x3) + 1 + CDK_XGSD_MIIM_IBUS(0);
    } else if (port >= 45) {
        phy_addr = ((port - 35) & ~0x3) + 1 + CDK_XGSD_MIIM_IBUS(1);
    } else if (port >= 21) {
        phy_addr = ((port - 21) & ~0x3) + 1 + CDK_XGSD_MIIM_IBUS(2);
    } else if (port >= 13) {
        phy_addr = ((port - 13) & ~0x3) + 1 + CDK_XGSD_MIIM_IBUS(1);
    } else {
        phy_addr = ((port - 1) & ~0x3) + 1 + CDK_XGSD_MIIM_IBUS(0);
    }

    return phy_addr;
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

phy_bus_t phy_bus_bcm56670_miim_int = {
    "bcm56670_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst,
    _bus_cap
};

#else

/* ISO C forbids empty source files */
int bcm56670_miim_int_not_empty;

#endif /* CDK_CONFIG_INCLUDE_BCM56670_A0 */
