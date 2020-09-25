/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default XGS internal PHY access functions.
 */

#include <cdk_config.h>

#if defined(CDK_CONFIG_INCLUDE_BCM53400_A0) && CDK_CONFIG_INCLUDE_BCM53400_A0 == 1

#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_miim.h>
#include <cdk/chip/bcm53400_a0_defs.h>

#include <phy/phy.h>

#define INDACC_POLLING_TIMEOUT          1000

#define PHY_REG_ADDR_DEVAD(_r)          (((_r) >> 27) & 0x1f)
#define QSGMII_REG_ADDR_REG(_r)         ((((_r) & 0x8000) >> 11) | ((_r) & 0xf))
#define QSGMII_REG_ADDR_LANE(_r)        (((_r) >> 16) & 0x7ff)
#define QSGMII_REG_ADDR_LANE_SET(_nr, _r) ((_nr) |= ((_r) & 0x7ff0000))
#define IS_QSGMII_REGISTER(_r)          (((_r) & 0xf800f000) == 0x8000)

static int
_is_qsgmii_port(int unit, int port)
{
    cdk_pbmp_t pbmp;
    
    if (CDK_XGSD_FLAGS(unit) & CHIP_FLAG_GE) {
        CDK_XGSD_BLKTYPE_PBMP_GET(unit, BLKTYPE_GXPORT, &pbmp);
        return CDK_PBMP_MEMBER(pbmp, port);
    }

    return 0;
}

static int
_indirect_access_read(int unit, uint32_t phy_addr,
                      uint32_t phy_reg, uint32_t *phy_data)
{
    int rv = CDK_E_NONE;
    int ioerr = 0;
    int idx;
    uint32_t lane;
    CHIP_INDACC_RDATAr_t indacc_rdata;
    CHIP_INDACC_CTLSTSr_t indacc_ctlsts;

    lane = QSGMII_REG_ADDR_LANE(phy_reg);
    if (lane > 15) {
        return CDK_E_PARAM;
    }

    CHIP_INDACC_CTLSTSr_CLR(indacc_ctlsts);
    CHIP_INDACC_CTLSTSr_TARGET_SELECTf_SET(indacc_ctlsts, (lane / 8));
    CHIP_INDACC_CTLSTSr_RD_REQf_SET(indacc_ctlsts, 1);
    /* Assigning the reg_addr in Clause22 format */
    CHIP_INDACC_CTLSTSr_ADDRESSf_SET(indacc_ctlsts, (phy_reg & 0x1f));
    ioerr += WRITE_CHIP_INDACC_CTLSTSr(unit, indacc_ctlsts);

    for (idx = 0; idx < INDACC_POLLING_TIMEOUT; idx++) {
        ioerr += READ_CHIP_INDACC_CTLSTSr(unit, &indacc_ctlsts);
        if (CHIP_INDACC_CTLSTSr_RD_RDYf_GET(indacc_ctlsts)) {
            break;
        }
    }
    if (idx >= INDACC_POLLING_TIMEOUT) {
        CDK_WARN(("bcm53400_a0_bmd_attach[%d]: "
                  "In-direct access polling timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    ioerr += READ_CHIP_INDACC_RDATAr(unit, &indacc_rdata);
    *phy_data = CHIP_INDACC_RDATAr_GET(indacc_rdata);

    CHIP_INDACC_CTLSTSr_CLR(indacc_ctlsts);
    ioerr += WRITE_CHIP_INDACC_CTLSTSr(unit, indacc_ctlsts);

    return ioerr ? CDK_E_IO : rv;
}

static int
_indirect_access_write(int unit, uint32_t phy_addr,
                       uint32_t phy_reg, uint32_t phy_data)
{
    int rv = CDK_E_NONE;
    int ioerr = 0;
    int idx;
    uint32_t lane;
    CHIP_INDACC_WDATAr_t indacc_wdata;
    CHIP_INDACC_CTLSTSr_t indacc_ctlsts;
    
    CHIP_INDACC_WDATAr_SET(indacc_wdata, phy_data);
    ioerr += WRITE_CHIP_INDACC_WDATAr(unit, indacc_wdata);

    lane = QSGMII_REG_ADDR_LANE(phy_reg);
    if (lane > 15) {
        return CDK_E_PARAM;
    }

    CHIP_INDACC_CTLSTSr_CLR(indacc_ctlsts);
    CHIP_INDACC_CTLSTSr_TARGET_SELECTf_SET(indacc_ctlsts, (lane / 8));
    CHIP_INDACC_CTLSTSr_WR_REQf_SET(indacc_ctlsts, 1);
    /* Assigning the reg_addr in Clause22 format */
    CHIP_INDACC_CTLSTSr_ADDRESSf_SET(indacc_ctlsts, (phy_reg & 0x1f));
    ioerr += WRITE_CHIP_INDACC_CTLSTSr(unit, indacc_ctlsts);
    
    for (idx = 0; idx < INDACC_POLLING_TIMEOUT; idx++) {
        ioerr += READ_CHIP_INDACC_CTLSTSr(unit, &indacc_ctlsts);
        if (CHIP_INDACC_CTLSTSr_WR_RDYf_GET(indacc_ctlsts)) {
            break;
        }
    }
    if (idx >= INDACC_POLLING_TIMEOUT) {
        CDK_WARN(("bcm53400_a0_bmd_attach[%d]: "
                  "In-direct access polling timeout\n", unit));
        return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
    }

    CHIP_INDACC_CTLSTSr_CLR(indacc_ctlsts);
    ioerr += WRITE_CHIP_INDACC_CTLSTSr(unit, indacc_ctlsts);

    return ioerr ? CDK_E_IO : rv;
}

static int
_mdio_addr_to_port(uint32_t phy_addr)
{
    if (CDK_XGSD_MIIM_IBUS_NUM(phy_addr) == 1) {
        return (phy_addr & 0x1f) + 17;
    } else {
        return (phy_addr & 0x1f) + 1;
    }
}

static int
_qsgmii_access_check(int unit, int port, uint32_t phy_reg)
{
    int qflag;
    
    /* Check whether the access is to QSGMII or not */
    qflag = 0;
    if (PHY_REG_ADDR_DEVAD(phy_reg) == 0) {
        /* PCS registers use DEVAD 0 */
        if (IS_QSGMII_REGISTER(phy_reg)) {
            qflag = 1;
        } else if ((QSGMII_REG_ADDR_REG(phy_reg) < 0x10) &&
                   (_is_qsgmii_port(unit, port))) {
            qflag = 1; 
        }
    }
    return qflag;
}

static int
_sbus_mdio_read(int unit, uint32_t phy_addr, 
                uint32_t phy_reg, uint32_t *phy_data)
{
    int rv = CDK_E_NONE;
    int port, idx;
    uint32_t data[16];
    uint32_t reg_addr, reg_data;
    XLPORT_WC_UCMEM_DATAm_t ucmem_data;

    port = _mdio_addr_to_port(phy_addr);
    if (_qsgmii_access_check(unit, port, phy_reg)) {
        /* TSCQ SBUS access */
        *phy_data = 0;

        /* AER process : AER block selection */
        reg_addr = 0x1f;
        QSGMII_REG_ADDR_LANE_SET(reg_addr, phy_reg);
        reg_data = 0xffd0;
        rv = _indirect_access_write(unit, phy_addr, reg_addr, reg_data);
    
        if (CDK_SUCCESS(rv)) {
            /* AER process : lane control */
            reg_addr = 0x1e;
            QSGMII_REG_ADDR_LANE_SET(reg_addr, phy_reg);
            reg_data = QSGMII_REG_ADDR_LANE(phy_reg) & 0x7;
            rv = _indirect_access_write(unit, phy_addr, reg_addr, reg_data);
        }
        
        /* Target register block selection */
        if (CDK_SUCCESS(rv)) {
            reg_addr = 0x1f;
            QSGMII_REG_ADDR_LANE_SET(reg_addr, phy_reg);
            reg_data = phy_reg & 0xfff0;
            rv = _indirect_access_write(unit, phy_addr, reg_addr, reg_data);
        }
        
        /* Read data */
        if (CDK_SUCCESS(rv)) {
            reg_addr = QSGMII_REG_ADDR_REG(phy_reg);
            QSGMII_REG_ADDR_LANE_SET(reg_addr, phy_reg);
            rv = _indirect_access_read(unit, phy_addr, reg_addr, &reg_data);
        }
        
        if (CDK_SUCCESS(rv)) {
            *phy_data = reg_data;
        }
    } else {
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
    uint32_t reg_addr, reg_data;
    XLPORT_WC_UCMEM_DATAm_t ucmem_data;
    
    port = _mdio_addr_to_port(phy_addr);
    if (_qsgmii_access_check(unit, port, phy_reg)) {  
        /* TSCQ SBUS access */
        /* AER process : AER block selection */
        reg_addr = 0x1f;
        QSGMII_REG_ADDR_LANE_SET(reg_addr, phy_reg);
        reg_data = 0xffd0;
        rv = _indirect_access_write(unit, phy_addr, reg_addr, reg_data);
    
        if (CDK_SUCCESS(rv)) {
            /* AER process : lane control */
            reg_addr = 0x1e;
            QSGMII_REG_ADDR_LANE_SET(reg_addr, phy_reg);
            reg_data = QSGMII_REG_ADDR_LANE(phy_reg) & 0x7;
            rv = _indirect_access_write(unit, phy_addr, reg_addr, reg_data);
        }
            
        /* Target register block selection */
        if (CDK_SUCCESS(rv)) {
            reg_addr = 0x1f;
            QSGMII_REG_ADDR_LANE_SET(reg_addr, phy_reg);
            reg_data = phy_reg & 0xfff0;
            rv = _indirect_access_write(unit, phy_addr, reg_addr, reg_data);
        }
    
        /* Write data */
        if (CDK_SUCCESS(rv)) {
            reg_addr = QSGMII_REG_ADDR_REG(phy_reg);
            QSGMII_REG_ADDR_LANE_SET(reg_addr, phy_reg);
            rv = _indirect_access_write(unit, phy_addr, reg_addr, phy_data);
        }
    } else {
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
    if (port >= 2 && port < 18) {
        return ((port - 2) & ~0xf) + 1 + CDK_XGSD_MIIM_IBUS(0);
    } else if (port >= 18 && port < 38) {
        return ((port - 18) & ~0x3) + 1 + CDK_XGSD_MIIM_IBUS(1);
    } 
    /* Should not get here */
    return 0;
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
    return port - 2;
}

static uint32_t
_bus_cap(uint32_t addr)
{
    return (PHY_BUS_CAP_WR_MASK);
}

phy_bus_t phy_bus_bcm53400_miim_int = {
    "bcm53400_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst,
    _bus_cap
};

#else

/* ISO C forbids empty source files */
int bcm53400_miim_int_not_empty;

#endif /* CDK_CONFIG_INCLUDE_BCM53400_A0 */
