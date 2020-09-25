/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default XGS internal PHY access functions.
 */

#include <cdk_config.h>

#if defined(CDK_CONFIG_INCLUDE_BCM56160_A0) && CDK_CONFIG_INCLUDE_BCM56160_A0 == 1

#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/chip/bcm56160_a0_defs.h>

#include <phy/phy.h>

static int
_mdio_addr_to_port(uint32_t phy_addr)
{
    int bus, offset;
    int mdio_addr;

    /* Must be internal MDIO address */
    if ((phy_addr & 0x200) == 0) {
        return 0;
    }

    bus = CDK_XGSD_MIIM_IBUS_NUM(phy_addr);
    mdio_addr = phy_addr & 0x1f;

    if (bus == 0) {
        if (mdio_addr <= 0x8) {
            offset = 9;
        } else {
            offset = 17;
        }
    } else if (bus == 1) {
        if (mdio_addr <= 0x8) {
            offset = 33;
        } else if (mdio_addr <= 0xc) {
            offset = -7;
        } else {
            offset = 5;
        }
    } else {
        return 0;
    }

    return mdio_addr + offset;
}

static int
_sbus_mdio_read(int unit, uint32_t phy_addr,
                uint32_t phy_reg, uint32_t *phy_data)
{
    int rv = CDK_E_NONE;
    cdk_pbmp_t gpbmp, xlpbmp;
    int port, idx;
    uint32_t data[16];
    GPORT_WC_UCMEM_DATAm_t g_ucmem_data;
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_PMQ, &gpbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &xlpbmp);

    /* TSCE SBUS access */
    phy_reg |= ((phy_addr & 0x1f) << 19);

    CDK_MEMSET(data, 0, sizeof(data));
    data[0] = phy_reg & 0xffffffff;
    data[2] = 0;

    port = _mdio_addr_to_port(phy_addr);
    if (CDK_PBMP_MEMBER(gpbmp, port)) {
        cdk_xgsd_pblk_t pblk;

        rv = cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_PMQ);
        if (!CDK_SUCCESS(rv)) {
            return rv ;
        }

        for (idx = 0; idx < 4; idx++) {
            GPORT_WC_UCMEM_DATAm_SET(g_ucmem_data, idx, data[idx]);
        }

        rv = WRITE_GPORT_WC_UCMEM_DATAm(unit, pblk.bindex,
                                        (pblk.bport >> 8), g_ucmem_data);
        if (CDK_SUCCESS(rv)) {
            rv = READ_GPORT_WC_UCMEM_DATAm(unit, pblk.bindex,
                                        (pblk.bport >> 8), &g_ucmem_data);
        }
        *phy_data = GPORT_WC_UCMEM_DATAm_GET(g_ucmem_data, 1);
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
    cdk_pbmp_t gpbmp, xlpbmp;
    int port, idx;
    uint32_t data[16];
    GPORT_WC_UCMEM_DATAm_t g_ucmem_data;
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;

    port = _mdio_addr_to_port(phy_addr);

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_PMQ, &gpbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_XLPORT, &xlpbmp);

    /* TSCE SBUS access */
    phy_reg |= ((phy_addr & 0x1f) << 19);
    if ((phy_data & 0xffff0000) == 0) {
        phy_data |= 0xffff0000;
    }

    CDK_MEMSET(data, 0, sizeof(data));
    data[0] = phy_reg & 0xffffffff;
    data[1] = ((phy_data & 0xffff) << 16) |
              ((~phy_data & 0xffff0000) >> 16);
    data[2] = 1; /* for TSC register write */

    port = _mdio_addr_to_port(phy_addr);
    if (CDK_PBMP_MEMBER(gpbmp, port)) {
        cdk_xgsd_pblk_t pblk;

        rv = cdk_xgsd_port_block(unit, port, &pblk, BLKTYPE_PMQ);
        if (!CDK_SUCCESS(rv)) {
            return rv ;
        }

        for (idx = 0; idx < 4; idx++) {
            GPORT_WC_UCMEM_DATAm_SET(g_ucmem_data, idx, data[idx]);
        }
        rv = WRITE_GPORT_WC_UCMEM_DATAm(unit, pblk.bindex,
                                        (pblk.bport >> 8), g_ucmem_data);
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
    if (port >= 34 && port < 42) {
        return ((port - 34) & ~0x3) + 1 + CDK_XGSD_MIIM_IBUS(1);
    }
    if (port >= 26 && port < 34) {
        return (port - 17) + CDK_XGSD_MIIM_IBUS(0);
    }
    if (port >= 10 && port < 18) {
        return (port - 9) + CDK_XGSD_MIIM_IBUS(0);
    }
    if (port >= 2 && port < 34) {
        return (((port - 2) & ~0xf) >> 2) + 9 + CDK_XGSD_MIIM_IBUS(1);
    }

    return 0;
}

static uint32_t
_gphy_bypass_phy_addr(int port)
{
    if (port >= 38) {
        return ((port - 38) & ~0xf) + 0x5 + CDK_XGSD_MIIM_IBUS(1);
    } else if (port >= 34) {
        return ((port - 34) & ~0xf) + 0x1 + CDK_XGSD_MIIM_IBUS(1);
    } else if (port >= 18) {
        return ((port - 18) & ~0xf) + 0xd + CDK_XGSD_MIIM_IBUS(1);
    } else if (port >= 2) {
        return ((port - 2) & ~0xf) + 0x9 + CDK_XGSD_MIIM_IBUS(1);
    }

    return 0;
}

static int
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    if (CDK_XGSD_MIIM_IBUS_NUM(addr) == 1) {
        return _sbus_mdio_read(unit, addr, reg, val);
    } else {
        return cdk_xgsd_miim_read(unit, addr, reg, val);
    }
}

static int
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    if (CDK_XGSD_MIIM_IBUS_NUM(addr) == 1) {
        return _sbus_mdio_write(unit, addr, reg, val);
    } else {
        return cdk_xgsd_miim_write(unit, addr, reg, val);
    }
}

static int
_phy_inst(int port)
{
    return port - 2;
}

static uint32_t
_bus_cap(uint32_t addr)
{
    uint32_t cap = 0;

    if (CDK_XGSD_MIIM_IBUS_NUM(addr) == 1) {
        cap |= PHY_BUS_CAP_WR_MASK;
    }
    return cap;
}

phy_bus_t phy_bus_bcm56160_miim_int = {
    "bcm56160_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst,
    _bus_cap
};

phy_bus_t phy_bus_bcm56160_gphy_bypass_miim_int = {
    "bcm56160_gphy_bypass_miim_int",
    _gphy_bypass_phy_addr,
    _read,
    _write,
    _phy_inst,
    _bus_cap
};

#else

/* ISO C forbids empty source files */
int bcm56160_miim_int_not_empty;

#endif /* CDK_CONFIG_INCLUDE_BCM56160_A0 */
