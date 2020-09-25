/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default internal XGXS PHY access functions.
 */

#include <cdk_config.h>

#if defined(CDK_CONFIG_INCLUDE_BCM56560_B0) && CDK_CONFIG_INCLUDE_BCM56560_B0 == 1

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/chip/bcm56560_b0_defs.h>
#include <cdk/cdk_debug.h>
#include <../PKG/chip/bcm56560_b0/bcm56560_b0_internal.h>

#include <phy/phy.h>

#define TSC_REG_ADDR_TSCID_SET(_phy_reg, _phyad)    \
                            ((_phy_reg) |= ((_phyad) & 0x1f) << 19)

static int
_mdio_addr_to_port(uint32_t phy_addr)
{
    /*
     * Internal phy address:
     * bus 0 phy 1 to 28 are mapped to Physical port 1 to 28
     * bus 1 phy 1 to 20 are mapped to Physical port 29 to 48
     * bus 2 phy 1 to 24 are mapped to Physical port 49 to 72
     */
    uint32_t bus = CDK_XGSD_MIIM_IBUS_NUM(phy_addr);
    if (bus == 2) {
        return (phy_addr & 0x1f) + 48;
    } else if (bus == 1) {
        return (phy_addr & 0x1f) + 28;
    } else {
        return (phy_addr & 0x1f) + 0;
    }
}

static int
_sbus_mdio_read(int unit, uint32_t phy_addr,
                uint32_t phy_reg, uint32_t *phy_data)
{
    int rv = CDK_E_NONE;
    int port, idx;
    uint32_t data[4];
    CLPORT_WC_UCMEM_DATAm_t cl_ucmem_data;
    CLG2PORT_WC_UCMEM_DATAm_t clg_ucmem_data;
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;
    CXXPORT_WC_UCMEM_DATA0m_t cxx_ucmem_data0;
    CXXPORT_WC_UCMEM_DATA1m_t cxx_ucmem_data1;
    CXXPORT_WC_UCMEM_DATA2m_t cxx_ucmem_data2;
    cdk_pbmp_t clpbmp, clgpbmp;

    port = _mdio_addr_to_port(phy_addr);

    TSC_REG_ADDR_TSCID_SET(phy_reg, phy_addr);

    bcm56560_b0_clport_pbmp_get(unit, &clpbmp);
    bcm56560_b0_clg2port_pbmp_get(unit, &clgpbmp);

    CDK_MEMSET(data, 0, sizeof(data));
    data[0] = phy_reg & 0xffffffff;

    if (port >= 17 && port <= 28) {
        if (port >= 17 && port <= 20) {
            for (idx = 0; idx < 4; idx++) {
                CXXPORT_WC_UCMEM_DATA0m_SET(cxx_ucmem_data0, idx, data[idx]);
            }
            rv = WRITE_CXXPORT_WC_UCMEM_DATA0m(unit, 0, cxx_ucmem_data0, 17);

            if (CDK_SUCCESS(rv)) {
                rv = READ_CXXPORT_WC_UCMEM_DATA0m(unit, 0, &cxx_ucmem_data0, 17);
            }
            *phy_data = CXXPORT_WC_UCMEM_DATA0m_GET(cxx_ucmem_data0, 1);
        } else if (port >= 21 && port <= 24) {
            for (idx = 0; idx < 4; idx++) {
                CXXPORT_WC_UCMEM_DATA1m_SET(cxx_ucmem_data1, idx, data[idx]);
            }
            rv = WRITE_CXXPORT_WC_UCMEM_DATA1m(unit, 0, cxx_ucmem_data1, 17);

            if (CDK_SUCCESS(rv)) {
                rv = READ_CXXPORT_WC_UCMEM_DATA1m(unit, 0, &cxx_ucmem_data1, 17);
            }
            *phy_data = CXXPORT_WC_UCMEM_DATA1m_GET(cxx_ucmem_data1, 1);
        } else {
            for (idx = 0; idx < 4; idx++) {
                CXXPORT_WC_UCMEM_DATA2m_SET(cxx_ucmem_data2, idx, data[idx]);
            }
            rv = WRITE_CXXPORT_WC_UCMEM_DATA2m(unit, 0, cxx_ucmem_data2, 17);

            if (CDK_SUCCESS(rv)) {
                rv = READ_CXXPORT_WC_UCMEM_DATA2m(unit, 0, &cxx_ucmem_data2, 17);
            }
            *phy_data = CXXPORT_WC_UCMEM_DATA2m_GET(cxx_ucmem_data2, 1);
        }
    } else if (port >= 53 && port <= 64) {
        if (port >= 53 && port <= 56) {
            for (idx = 0; idx < 4; idx++) {
                CXXPORT_WC_UCMEM_DATA0m_SET(cxx_ucmem_data0, idx, data[idx]);
            }
            rv = WRITE_CXXPORT_WC_UCMEM_DATA0m(unit, 0, cxx_ucmem_data0, 53);

            if (CDK_SUCCESS(rv)) {
                rv = READ_CXXPORT_WC_UCMEM_DATA0m(unit, 0, &cxx_ucmem_data0, 53);
            }
            *phy_data = CXXPORT_WC_UCMEM_DATA0m_GET(cxx_ucmem_data0, 1);
        } else if (port >= 57 && port <= 60) {
            for (idx = 0; idx < 4; idx++) {
                CXXPORT_WC_UCMEM_DATA1m_SET(cxx_ucmem_data1, idx, data[idx]);
            }
            rv = WRITE_CXXPORT_WC_UCMEM_DATA1m(unit, 0, cxx_ucmem_data1, 53);

            if (CDK_SUCCESS(rv)) {
                rv = READ_CXXPORT_WC_UCMEM_DATA1m(unit, 0, &cxx_ucmem_data1, 53);
            }
            *phy_data = CXXPORT_WC_UCMEM_DATA1m_GET(cxx_ucmem_data1, 1);
        } else {
            for (idx = 0; idx < 4; idx++) {
                CXXPORT_WC_UCMEM_DATA2m_SET(cxx_ucmem_data2, idx, data[idx]);
            }
            rv = WRITE_CXXPORT_WC_UCMEM_DATA2m(unit, 0, cxx_ucmem_data2, 53);

            if (CDK_SUCCESS(rv)) {
                rv = READ_CXXPORT_WC_UCMEM_DATA2m(unit, 0, &cxx_ucmem_data2, 53);
            }
            *phy_data = CXXPORT_WC_UCMEM_DATA2m_GET(cxx_ucmem_data2, 1);
        }
    } else if (CDK_PBMP_MEMBER(clgpbmp, port)) {
        for (idx = 0; idx < 4; idx++) {
            CLG2PORT_WC_UCMEM_DATAm_SET(clg_ucmem_data, idx, data[idx]);
        }
        rv = WRITE_CLG2PORT_WC_UCMEM_DATAm(unit, 0, clg_ucmem_data, port);

        if (CDK_SUCCESS(rv)) {
            rv = READ_CLG2PORT_WC_UCMEM_DATAm(unit, 0, &clg_ucmem_data, port);
        }
        *phy_data = CLG2PORT_WC_UCMEM_DATAm_GET(clg_ucmem_data, 1);
    } else if (CDK_PBMP_MEMBER(clpbmp, port)) {
        for (idx = 0; idx < 4; idx++) {
            CLPORT_WC_UCMEM_DATAm_SET(cl_ucmem_data, idx, data[idx]);
        }
        rv = WRITE_CLPORT_WC_UCMEM_DATAm(unit, 0, cl_ucmem_data, port);

        if (CDK_SUCCESS(rv)) {
            rv = READ_CLPORT_WC_UCMEM_DATAm(unit, 0, &cl_ucmem_data, port);
        }
        *phy_data = CLPORT_WC_UCMEM_DATAm_GET(cl_ucmem_data, 1);
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
    CLPORT_WC_UCMEM_DATAm_t cl_ucmem_data;
    CLG2PORT_WC_UCMEM_DATAm_t clg_ucmem_data;
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;
    CXXPORT_WC_UCMEM_DATA0m_t cxx_ucmem_data0;
    CXXPORT_WC_UCMEM_DATA1m_t cxx_ucmem_data1;
    CXXPORT_WC_UCMEM_DATA2m_t cxx_ucmem_data2;
    cdk_pbmp_t clpbmp, clgpbmp;

    port = _mdio_addr_to_port(phy_addr);
    TSC_REG_ADDR_TSCID_SET(phy_reg, phy_addr);

    bcm56560_b0_clport_pbmp_get(unit, &clpbmp);
    bcm56560_b0_clg2port_pbmp_get(unit, &clgpbmp);

    if ((phy_data & 0xffff0000) == 0) {
        phy_data |= 0xffff0000;
    }

    CDK_MEMSET(data, 0, sizeof(data));
    data[0] = phy_reg & 0xffffffff;
    data[1] = ((phy_data & 0xffff) << 16) |
              ((~phy_data & 0xffff0000) >> 16);
    data[2] = 1; /* for TSC register write */

    if (port >= 17 && port <= 28) {
        if (port >= 17 && port <= 20) {
            for (idx = 0; idx < 4; idx++) {
                CXXPORT_WC_UCMEM_DATA0m_SET(cxx_ucmem_data0, idx, data[idx]);
            }
            rv = WRITE_CXXPORT_WC_UCMEM_DATA0m(unit, 0, cxx_ucmem_data0, 17);
        } else if (port >= 21 && port <= 24) {
            for (idx = 0; idx < 4; idx++) {
                CXXPORT_WC_UCMEM_DATA1m_SET(cxx_ucmem_data1, idx, data[idx]);
            }
            rv = WRITE_CXXPORT_WC_UCMEM_DATA1m(unit, 0, cxx_ucmem_data1, 17);
        } else {
            for (idx = 0; idx < 4; idx++) {
                CXXPORT_WC_UCMEM_DATA2m_SET(cxx_ucmem_data2, idx, data[idx]);
            }
            rv = WRITE_CXXPORT_WC_UCMEM_DATA2m(unit, 0, cxx_ucmem_data2, 17);
        }
    } else if (port >= 53 && port <= 64) {
        if (port >= 53 && port <= 56) {
            for (idx = 0; idx < 4; idx++) {
                CXXPORT_WC_UCMEM_DATA0m_SET(cxx_ucmem_data0, idx, data[idx]);
            }
            rv = WRITE_CXXPORT_WC_UCMEM_DATA0m(unit, 0, cxx_ucmem_data0, 53);
        } else if (port >= 57 && port <= 60) {
            for (idx = 0; idx < 4; idx++) {
                CXXPORT_WC_UCMEM_DATA1m_SET(cxx_ucmem_data1, idx, data[idx]);
            }
            rv = WRITE_CXXPORT_WC_UCMEM_DATA1m(unit, 0, cxx_ucmem_data1, 53);
        } else {
            for (idx = 0; idx < 4; idx++) {
                CXXPORT_WC_UCMEM_DATA2m_SET(cxx_ucmem_data2, idx, data[idx]);
            }
            rv = WRITE_CXXPORT_WC_UCMEM_DATA2m(unit, 0, cxx_ucmem_data2, 53);
        }
    } else if (CDK_PBMP_MEMBER(clgpbmp, port)) {
        for (idx = 0; idx < 4; idx++) {
            CLG2PORT_WC_UCMEM_DATAm_SET(clg_ucmem_data, idx, data[idx]);
        }
        rv = WRITE_CLG2PORT_WC_UCMEM_DATAm(unit, 0, clg_ucmem_data, port);
    } else if (CDK_PBMP_MEMBER(clpbmp, port)) {
        for (idx = 0; idx < 4; idx++) {
            CLPORT_WC_UCMEM_DATAm_SET(cl_ucmem_data, idx, data[idx]);
        }
        rv = WRITE_CLPORT_WC_UCMEM_DATAm(unit, 0, cl_ucmem_data, port);
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
    /*
     * Internal phy address:
     * bus 0 phy 1 to 28 are mapped to Physical port 1 to 28
     * bus 1 phy 1 to 20 are mapped to Physical port 29 to 48
     * bus 2 phy 1 to 24 are mapped to Physical port 49 to 72
     */

    if (port > 48) {
        return ((port - 49) & ~0x3) + 1 + CDK_XGSD_MIIM_IBUS(2);
    } else if (port > 28) {
        return ((port - 29) & ~0x3) + 1 + CDK_XGSD_MIIM_IBUS(1);
    } else {
        return ((port - 1) & ~0x3) + 1 + CDK_XGSD_MIIM_IBUS(0);
    }
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

phy_bus_t phy_bus_bcm56560_b0_miim_int = {
    "bcm56560_b0_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst,
    _bus_cap
};

#else

/* ISO C forbids empty source files */
int bcm56560_b0_miim_int_not_empty;

#endif /* CDK_CONFIG_INCLUDE_BCM56560_B0 */
