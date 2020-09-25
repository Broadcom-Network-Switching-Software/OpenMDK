/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default XGS internal PHY access functions.
 */

#include <cdk_config.h>

#if defined(CDK_CONFIG_INCLUDE_BCM53570_B0) && CDK_CONFIG_INCLUDE_BCM53570_B0 == 1

#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/chip/bcm53570_b0_defs.h>

#include <phy/phy.h>

#define TSC_REG_ADDR_TSCID_SET(_phy_reg, _phyad)    \
                            ((_phy_reg) |= ((_phyad) & 0x1f) << 19)

static int
_mdio_addr_to_port(uint32_t phy_addr_int)
{
    int bus, offset;
    int mdio_addr;

    /* Must be internal MDIO address */
    if ((phy_addr_int & 0x200) == 0) {
        return 0;
    }

    bus = CDK_XGSD_MIIM_IBUS_NUM(phy_addr_int);
    mdio_addr = phy_addr_int & 0x1f;

    if (bus == 0) {
        if (mdio_addr <= 0x18) {
            offset = 1;
        } else {
            return -1;
        }
    } else if (bus == 1) {
        if (mdio_addr <= 0x1c) {
            offset = 57;
        } else {
            return -1;
        }
    } else if (bus == 2) {
        if (mdio_addr <= 4) {
            offset = 25;
        } else if (mdio_addr <= 8) {
            offset = 37;
        } else {
            return -1;
        }
    } else if (bus == 3) {
        if (mdio_addr <= 0x4) {
            offset = 85;
        } else {
            return -1;
        }
    } else {
        return 0;
    }

    return mdio_addr + offset;
}

static int
_sbus_tsc_reg_read(int unit, uint32_t phy_addr,
                uint32_t phy_reg, uint32_t *phy_data)
{
    int rv = CDK_E_NONE;
    cdk_pbmp_t gpbmp, clpbmp;
    int port, idx;
    uint32_t data[4];
    GPORT_WC_UCMEM_DATAm_t g_ucmem_data;
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;
    CLPORT_WC_UCMEM_DATAm_t cl_ucmem_data;

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_PMQ, &gpbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_CLPORT, &clpbmp);

    /* TSCE SBUS access */
    phy_reg |= ((phy_addr & 0x1f) << 19);

    CDK_MEMSET(data, 0, sizeof(data));
    data[0] = phy_reg & 0xffffffff;
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
_sbus_tsc_reg_write(int unit, uint32_t phy_addr,
                 uint32_t phy_reg, uint32_t phy_data)
{
    int rv = CDK_E_NONE;
    cdk_pbmp_t gpbmp, clpbmp;
    int port, idx;
    uint32_t data[4];
    GPORT_WC_UCMEM_DATAm_t g_ucmem_data;
    XLPORT_WC_UCMEM_DATAm_t xl_ucmem_data;
    CLPORT_WC_UCMEM_DATAm_t cl_ucmem_data;

    port = _mdio_addr_to_port(phy_addr);

    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_PMQ, &gpbmp);
    CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_CLPORT, &clpbmp);

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
    int val;
    uint32_t addr = 0;

    if (port >= 86 && port <= 89) {
        addr = 1 + CDK_XGSD_MIIM_IBUS(3);
    }
    if (port >= 58 && port <= 85) {
        val = (port - 58) / 4;
        addr = (val * 4) + 1 + CDK_XGSD_MIIM_IBUS(1);
    }
    if (port >= 26 && port <= 57) {
        val = (port - 26) / 16;
        addr = (val * 4) + 1 + CDK_XGSD_MIIM_IBUS(2);
    }
    if (port >= 2 && port <= 25) {
        val = (port - 2) / 4;
        addr = (val * 4) + 1 + CDK_XGSD_MIIM_IBUS(0);
    }

    return addr;
}

static int
_tscx_reg_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    int ret_val;

    if (CDK_XGSD_MIIM_IBUS_NUM(addr) != 0) {
        TSC_REG_ADDR_TSCID_SET(reg, addr);
        ret_val = _sbus_tsc_reg_read(unit, addr, reg, val);
    } else {
        ret_val = cdk_xgsd_miim_read(unit, addr, reg, val);
    }
    return ret_val;
}

static int
_tscx_reg_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    int ret_val;

    if (CDK_XGSD_MIIM_IBUS_NUM(addr) != 0) {
        TSC_REG_ADDR_TSCID_SET(reg, addr);
        ret_val = _sbus_tsc_reg_write(unit, addr, reg, val);
    } else {
        ret_val = cdk_xgsd_miim_write(unit, addr, reg, val);
    }
    return ret_val;
}

static int
_phy_inst(int port)
{

    if (port >= 26 && port <= 57) {
        return port - 26;
    }
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

phy_bus_t phy_bus_bcm53570_b0_miim_int = {
    "bcm53570_b0_miim_int",
    _phy_addr,
    _tscx_reg_read,
    _tscx_reg_write,
    _phy_inst,
    _bus_cap
};

#else

/* ISO C forbids empty source files */
int bcm53570_b0_miim_int_not_empty;

#endif /* CDK_CONFIG_INCLUDE_BCM53570_B0 */
