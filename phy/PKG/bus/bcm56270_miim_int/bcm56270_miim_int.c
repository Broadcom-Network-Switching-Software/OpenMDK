/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default XGS internal PHY access functions.
 */

#include <cdk_config.h>

#if defined(CDK_CONFIG_INCLUDE_BCM56270_A0) && CDK_CONFIG_INCLUDE_BCM56270_A0 == 1

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsm_miim.h>
#include <cdk/chip/bcm56270_a0_defs.h>
#include <cdk/cdk_debug.h>

#include <phy/phy.h>

#define INDACC_POLLING_TIMEOUT          1000

#define PHY_REG_ADDR_DEVAD(_r)          (((_r) >> 27) & 0x1f)
#define QSGMII_REG_ADDR_REG(_r)         ((((_r) & 0x8000) >> 11) | ((_r) & 0xf))
#define QSGMII_REG_ADDR_LANE(_r)        (((_r) >> 16) & 0x7ff)
#define QSGMII_REG_ADDR_LANE_SET(_nr, _r) ((_nr) |= ((_r) & 0x7ff0000))
#define IS_QSGMII_REGISTER(_r)          (((_r) & 0xf800f000) == 0x8000)


static int
_mdio_addr_to_port(uint32_t phy_addr)
{
    return (phy_addr & 0x1f) + 4;
}

static int
_sbus_mdio_read(int unit, uint32_t phy_addr, 
                uint32_t phy_reg, uint32_t *phy_data)
{
    int rv = CDK_E_NONE;
    int port, idx;
    uint32_t data[16];
    XLPORT_WC_UCMEM_DATAm_t ucmem_data;

    port = _mdio_addr_to_port(phy_addr);

    {
        /* TSCE SBUS access */
        phy_reg |= ((phy_addr & 0x1f) << 19);
    
        CDK_MEMSET(data, 0, sizeof(data));
        data[0] = phy_reg & 0xffffffff;
        data[2] = 0;

        for (idx = 0; idx < 4; idx++) {
            XLPORT_WC_UCMEM_DATAm_SET(ucmem_data, idx, data[idx]);
        }
        rv = WRITE_XLPORT_WC_UCMEM_DATAm(unit, 0, ucmem_data, port);

        if (CDK_SUCCESS(rv)) {
            rv = READ_XLPORT_WC_UCMEM_DATAm(unit, 0, &ucmem_data, port);
        }
        *phy_data = XLPORT_WC_UCMEM_DATAm_GET(ucmem_data, 1);
    }
    return rv;
}

static int
_sbus_mdio_write(int unit, uint32_t phy_addr,
                 uint32_t phy_reg, uint32_t phy_data)
{
    int rv = CDK_E_NONE;
    int port, idx;
    uint32_t data[16];
    XLPORT_WC_UCMEM_DATAm_t ucmem_data;
    
    port = _mdio_addr_to_port(phy_addr);
    {
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
        
        for (idx = 0; idx < 4; idx++) {
            XLPORT_WC_UCMEM_DATAm_SET(ucmem_data, idx, data[idx]);
        }
        rv = WRITE_XLPORT_WC_UCMEM_DATAm(unit, 0, ucmem_data, port);
    }
    
    return rv;
}

static uint32_t
_phy_addr(int port)
{
    if (port >= 9) {
        return 0x5 + CDK_XGSM_MIIM_IBUS(1);
    }        
    if (port >= 5) {
        return 0x1 + CDK_XGSM_MIIM_IBUS(1);
    }
    return 0x1 + CDK_XGSM_MIIM_IBUS(0);
}

static int
_phy_inst(int port)
{
    return (port - 1) % 4;
}

static int 
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    if (CDK_XGSM_MIIM_IBUS_NUM(addr) == 1) {
        return _sbus_mdio_read(unit, addr, reg, val);
    } else {
        return cdk_xgsm_miim_read(unit, addr, reg, val);
    }
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    if (CDK_XGSM_MIIM_IBUS_NUM(addr) == 1) {
        return _sbus_mdio_write(unit, addr, reg, val);
    } else {
        return cdk_xgsm_miim_write(unit, addr, reg, val);
    }
}

static uint32_t
_bus_cap(uint32_t addr)
{
    uint32_t cap = 0;

    if (CDK_XGSM_MIIM_IBUS_NUM(addr) == 1) {
        cap |= PHY_BUS_CAP_WR_MASK;
    }
    return cap;
}

phy_bus_t phy_bus_bcm56270_miim_int = {
    "bcm56270_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst,
    _bus_cap
};


#else

/* ISO C forbids empty source files */
int bcm56270_miim_int_not_empty;

#endif /* CDK_CONFIG_INCLUDE_BCM56270_A0 */
