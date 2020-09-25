/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_device.h>
#include <bmd/bmd_phy_ctrl.h>

#include <cdk/cdk_string.h>

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy.h>

/* 
 * We do not want to rely on dynamic memory allocation,
 * so we allocate phy_ctrl blocks from a static pool.
 * The 'bus' member of the structure indicates whether
 * the block is free or in use.
 */
#define MAX_PHYS_PER_UNIT (BMD_CONFIG_MAX_PORTS * BMD_CONFIG_MAX_PHYS)
static phy_ctrl_t _phy_ctrl[BMD_CONFIG_MAX_UNITS * MAX_PHYS_PER_UNIT];

static phy_ctrl_t *
phy_ctrl_alloc(void)
{
    int idx;
    phy_ctrl_t *pc;

    for (idx = 0, pc = &_phy_ctrl[0]; idx < COUNTOF(_phy_ctrl); idx++, pc++) {
        if (pc->bus == 0) {
            return pc;
        }
    }
    return NULL;
}

static void
phy_ctrl_free(phy_ctrl_t *pc)
{
    pc->bus = 0;
}

/*
 * Probe all PHY buses associated with BMD device
 */
int 
bmd_phy_probe_default(int unit, int port, phy_driver_t **phy_drv)
{
    phy_bus_t **bus;
    phy_driver_t **drv;
    phy_ctrl_t pc_probe;
    phy_ctrl_t *pc;
    int rv;

    /* Remove any existing PHYs on this port */
    while ((pc = bmd_phy_del(unit, port)) != 0) {
        phy_ctrl_free(pc);;
    }

    /* Bail if not PHY driver list is provided */
    if (phy_drv == NULL) {
        return CDK_E_NONE;
    }

    /* Check that we have PHY bus list */
    bus = BMD_PORT_PHY_BUS(unit, port);
    if (bus == NULL) {
        return CDK_E_CONFIG;
    }

    /* Loop over PHY buses for this port */
    while (*bus != NULL) {
        drv = phy_drv;
        /* Probe all PHY drivers on this bus */
        while (*drv != NULL) {
            /* Initialize PHY control used for probing */
            pc = &pc_probe;
            CDK_MEMSET(pc, 0, sizeof(*pc));
            pc->unit = unit;
            pc->port = port;
            pc->bus = *bus;
            pc->drv = *drv;
            if (CDK_SUCCESS(PHY_PROBE(pc))) {
                /* Found known PHY on bus */
                pc = phy_ctrl_alloc();
                if (pc == NULL) {
                    return CDK_E_MEMORY;
                }
                /* Use macro instead of assignment to avoid calls to 'memcpy' */
                CDK_MEMCPY(pc, &pc_probe, sizeof(*pc));
                /* Install PHY */
                rv = bmd_phy_add(unit, port, pc);
                if (CDK_FAILURE(rv)) {
                    return rv;
                }
                /* Move to next bus */
                break;
            }
            drv++;
        }
        bus++;
    }
    return CDK_E_NONE;
}

#endif /* BMD_CONFIG_INCLUDE_PHY */
