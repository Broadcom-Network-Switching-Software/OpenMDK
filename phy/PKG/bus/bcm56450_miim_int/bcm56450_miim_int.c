/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Default XGS internal PHY access functions.
 */

#include <cdk_config.h>

#ifdef CDK_CONFIG_ARCH_XGSM_INSTALLED

#include <cdk/cdk_device.h>
#include <cdk/arch/xgsm_miim.h>

#include <phy/phy.h>

/* PHY address and instanse for Unicore/Unicore (MXQ6/MXQ7) PHY bus driver*/ 
static uint32_t
_phy_addr(int port)
{
    if (port == 42) {
        return 0x3 + CDK_XGSM_MIIM_IBUS(2);
    }
    if (port >= 38) {
        port = (port - 38) + 0x6;
        return port + CDK_XGSM_MIIM_IBUS(1);
    } 
    if (port >= 35) {
        port = (port - 35) + 0x2;
        return port + CDK_XGSM_MIIM_IBUS(1);
    } 
    if (port >= 32) {
        port = 0x9;
        return port + CDK_XGSM_MIIM_IBUS(1);
    }        
    if (port >= 29) {
        port = 0xd;
        return port + CDK_XGSM_MIIM_IBUS(1);
    }
    if (port >= 25) {
        port = (port - 25) * 4 + 1;
        return port + CDK_XGSM_MIIM_IBUS(1);
    }
    return port + CDK_XGSM_MIIM_IBUS(0);
}

static int
_phy_inst(int port)
{
    if (port == 42) {
        return 0;
    }
    if (port >= 29) {
        return ((port - 29) % 3) + 1;
    }
    if (port >= 25) {
        return 0;
    }
    return (port - 1) & 0x3;
}

/* PHY address and instanse for Warpcore/Warpcore (MXQ6/MXQ7) PHY bus driver*/ 
static uint32_t
_phy_addr_ww(int port)
{
    if ((port == 25 || (port >= 35 && port <= 37)) || 
        (port == 27 || (port >= 32 && port <= 34))) {
        return 0x9 + CDK_XGSM_MIIM_IBUS(1);
    }
    if ((port == 26 || (port >= 38 && port <= 40)) || 
        (port >= 28 && port <= 31)) {
        return 0xd + CDK_XGSM_MIIM_IBUS(1);
    }
    if (port == 42) {
        return 0x3 + CDK_XGSM_MIIM_IBUS(2);
    }
    return port + CDK_XGSM_MIIM_IBUS(0);
}

static int
_phy_inst_ww(int port)
{
    /* PHY instanse offset for port 25 ~ port 40 */
    int phy_inst_tlb[16] = {1, 1, 0, 0, 1, 2, 3, 1,
                            2, 3, 1, 3, 3, 1, 3, 3};
    if (port == 42) {
        return 0;
    }
    if (port >= 25 && port <= 40) {
        return phy_inst_tlb[port-25];
    }
    return (port - 1) & 0x3;
}

/* PHY address and instanse for Warpcore/Unicore (MXQ6/MXQ7) PHY bus driver*/ 
static uint32_t
_phy_addr_wu(int port)
{
    if ((port == 25 || (port >= 35 && port <= 37)) || 
        (port == 27 || (port >= 32 && port <= 34))) {
        return 0x9 + CDK_XGSM_MIIM_IBUS(1);
    }
    if (port == 42) {
        return 0x3 + CDK_XGSM_MIIM_IBUS(2);
    }
    if (port >= 38) {
        port = (port - 38) + 0x6;
        return port + CDK_XGSM_MIIM_IBUS(1);
    } 
    if (port >= 29) {
        port = 0xd;
        return port + CDK_XGSM_MIIM_IBUS(1);
    }
    if (port >= 25) {
        port = (port - 25) * 4 + 1;
        return port + CDK_XGSM_MIIM_IBUS(1);
    }
    return port + CDK_XGSM_MIIM_IBUS(0);
}

static int
_phy_inst_wu(int port)
{
    int phy_inst_tlb[16] = {1, 0, 0, 0, 1, 2, 3, 1,
                            2, 3, 1, 3, 3, 1, 2, 3};
    if (port == 42) {
        return 0;
    }
    if (port >= 25 && port <= 40) {
        return phy_inst_tlb[port-25];
    }
    return (port - 1) & 0x3;
}

/* PHY address and instanse for Unicore/Warpcore (MXQ6/MXQ7) PHY bus driver*/ 
static uint32_t
_phy_addr_uw(int port)
{
    if ((port == 26 || (port >= 38 && port <= 40)) || 
        (port >= 28 && port <= 31)) {
        return 0xd + CDK_XGSM_MIIM_IBUS(1);
    }
    if (port == 42) {
        return 0x3 + CDK_XGSM_MIIM_IBUS(2);
    }
    if (port >= 35) {
        port = (port - 35) + 0x2;
        return port + CDK_XGSM_MIIM_IBUS(1);
    } 
    if (port >= 32) {
        port = 0x9;
        return port + CDK_XGSM_MIIM_IBUS(1);
    }        
    if (port >= 25) {
        port = (port - 25) * 4 + 1;
        return port + CDK_XGSM_MIIM_IBUS(1);
    }
    return port + CDK_XGSM_MIIM_IBUS(0);
}

static int
_phy_inst_uw(int port)
{
    int phy_inst_tlb[16] = {0, 1, 0, 0, 1, 2, 3, 1,
                            2, 3, 1, 2, 3, 1, 3, 3};
    if (port == 42) {
        return 0;
    }
    if (port >= 25 && port <= 40) {
        return phy_inst_tlb[port-25];
    }
    return (port - 1) & 0x3;
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


phy_bus_t phy_bus_bcm56450_miim_int = {
    "bcm56450_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};

phy_bus_t phy_bus_bcm56450_miim_int_ww = {
    "bcm56450_miim_int_ww",
    _phy_addr_ww,
    _read,
    _write,
    _phy_inst_ww
};

phy_bus_t phy_bus_bcm56450_miim_int_wu = {
    "bcm56450_miim_int_wu",
    _phy_addr_wu,
    _read,
    _write,
    _phy_inst_wu
};

phy_bus_t phy_bus_bcm56450_miim_int_uw = {
    "bcm56450_miim_int_uw",
    _phy_addr_uw,
    _read,
    _write,
    _phy_inst_uw
};

#else

/* ISO C forbids empty source files */
int bcm56450_miim_int_not_empty;

#endif /* CDK_CONFIG_ARCH_XGSM_INSTALLED */
