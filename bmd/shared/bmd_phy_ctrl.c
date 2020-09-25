/*
 *
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * This API abstracts the PHY driver interface to the BMD
 * driver implementations, i.e. a change to the PHY driver 
 * interface should only require changes to this file.
 *
 * If no PHY support is compiled in or no PHYs are detected 
 * for a port, the API will report link up and an invalid
 * (negative) port speed. This behavior simplifies handling
 * of back-to-back MAC configurations as well as simulation
 * environments.
 */

#include <bmd/bmd.h>
#include <bmd/bmd_phy_ctrl.h>
#include <bmd/bmd_phy.h>

#if BMD_CONFIG_INCLUDE_PHY == 1

#include <phy/phy.h>

bmd_phy_info_t bmd_phy_info[BMD_CONFIG_MAX_UNITS];
uint32_t bmd_phy_default_ability[BMD_CONFIG_MAX_UNITS][BMD_CONFIG_MAX_PORTS] = {{0}};

static bmd_phy_probe_func_t phy_probe_func;
static phy_driver_t **phy_drv_list;

int
bmd_phy_bus_set(int unit, int port, phy_bus_t **bus_list)
{
    BMD_PORT_PHY_BUS(unit, port) = bus_list;
    return CDK_E_NONE;
}

int
bmd_phy_bus_get(int unit, int port, phy_bus_t ***bus_list)
{
    *bus_list = BMD_PORT_PHY_BUS(unit, port);
    return CDK_E_NONE;
}

int
bmd_phy_add(int unit, int port, phy_ctrl_t *pc)
{
    pc->next = BMD_PORT_PHY_CTRL(unit, port);
    BMD_PORT_PHY_CTRL(unit, port) = pc;
    return CDK_E_NONE;
}

phy_ctrl_t *
bmd_phy_del(int unit, int port)
{
    phy_ctrl_t *pc;

    if ((pc = BMD_PORT_PHY_CTRL(unit, port)) != 0) {
        BMD_PORT_PHY_CTRL(unit, port) = pc->next;
    }
    return pc;
}

int 
bmd_phy_probe_init(bmd_phy_probe_func_t probe, phy_driver_t **drv_list)
{
    phy_probe_func = probe;
    phy_drv_list = drv_list;

    return CDK_E_NONE;
}
#endif

int 
bmd_phy_probe(int unit, int port)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    if (phy_probe_func) {
        return phy_probe_func(unit, port, phy_drv_list);
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_init(int unit, int port)
{
    int rv = CDK_E_NONE;

#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        rv = PHY_RESET(BMD_PORT_PHY_CTRL(unit, port));;
        if (CDK_SUCCESS(rv) && phy_reset_cb) {
            rv = phy_reset_cb(BMD_PORT_PHY_CTRL(unit, port));
        }
        if (CDK_SUCCESS(rv)) {
            rv = PHY_INIT(BMD_PORT_PHY_CTRL(unit, port));
        }
        /* 
         * Apply the cached default ability only if the outermost PHY
         * is the internal PHY
         */
        if (CDK_SUCCESS(rv)) {
            if (bmd_phy_default_ability[unit][port] != 0) {
                phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, port);

                if (pc && pc->drv && (pc->drv->flags & PHY_DRIVER_F_INTERNAL)) {
                    PHY_CONFIG_SET(pc, PhyConfig_AdvLocal,
                                   bmd_phy_default_ability[unit][port], NULL);
                }
            }
        }
        if (CDK_SUCCESS(rv) && phy_init_cb) {
            rv = phy_init_cb(BMD_PORT_PHY_CTRL(unit, port));
        }
    }
#endif
    return rv;
}

int 
bmd_phy_attach(int unit, int port)
{
    int rv = bmd_phy_probe(unit, port);

#if BMD_CONFIG_INCLUDE_PHY == 1
    if (CDK_SUCCESS(rv)) {
        rv = bmd_phy_init(unit, port);
    }
#endif
    return rv;
}

int 
bmd_phy_detach(int unit, int port)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    if (phy_probe_func) {
        return phy_probe_func(unit, port, NULL);
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_mode_set(int unit, int port, char *name, int mode, int enable)
{
    int rv = CDK_E_NONE;

#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, port);
    phy_event_t event;

    while (pc != NULL) {
        if (name && pc->drv && pc->drv->drv_name &&
            CDK_STRSTR(pc->drv->drv_name, name) == NULL) {
            pc = pc->next;
            continue;
        }
        switch (mode) {
        case BMD_PHY_MODE_WAN:
            rv = PHY_CONFIG_SET(pc, PhyConfig_Mode,
                                enable ? PHY_MODE_WAN : PHY_MODE_LAN, NULL);
            if (!enable && rv == CDK_E_UNAVAIL) {
                rv = CDK_E_NONE;
            }
            break;
        case BMD_PHY_MODE_2LANE:
            if (enable) {
                PHY_CTRL_FLAGS(pc) |= PHY_F_2LANE_MODE;
            } else {
                PHY_CTRL_FLAGS(pc) &= ~PHY_F_2LANE_MODE;
            }
            break;
        case BMD_PHY_MODE_SERDES:
            if (enable) {
                PHY_CTRL_FLAGS(pc) |= PHY_F_SERDES_MODE;
            } else {
                PHY_CTRL_FLAGS(pc) &= ~PHY_F_SERDES_MODE;
            }
            break;
        case BMD_PHY_MODE_MULTI_CORE:
            if (enable) {
                PHY_CTRL_FLAGS(pc) |= PHY_F_MULTI_CORE;
            } else {
                PHY_CTRL_FLAGS(pc) &= ~PHY_F_MULTI_CORE;
            }
            break;
        case BMD_PHY_MODE_PASSTHRU:
            if (enable) {
                event = PhyEvent_ChangeToPassthru;
            } else {
                event = PhyEvent_ChangeToFiber;
            }
            PHY_NOTIFY(pc, event);
            break;
        case BMD_PHY_MODE_DISABLED:
            if (enable) {
                event = PhyEvent_PhyDisable;
            } else {
                event = PhyEvent_PhyEnable;
            }
            PHY_NOTIFY(pc, event);
            break;
        default:
            rv = CDK_E_PARAM;
            break;
        }
        break;
    }
#endif
    return rv;
}

int 
bmd_phy_notify_mac_enable(int unit, int port, int enable)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_event_t event = (enable) ? PhyEvent_MacEnable : PhyEvent_MacDisable;
    phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, port);

    if (pc) {
        /* Get innermost PHY */
        while (pc->next) {
            pc = pc->next;
        }
        return PHY_NOTIFY(pc, event);
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_notify_mac_loopback(int unit, int port, int enable)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, port);

    if (pc) {
        /* Get innermost PHY */
        while (pc->next) {
            pc = pc->next;
        }
        if (enable) {
            PHY_CTRL_FLAGS(pc) |= PHY_F_MAC_LOOPBACK;
        } else {
            PHY_CTRL_FLAGS(pc) &= ~PHY_F_MAC_LOOPBACK;
        }
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_link_get(int unit, int port, int *link, int *an_done)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        int rv, an;
        rv = PHY_AUTONEG_GET(BMD_PORT_PHY_CTRL(unit, port), &an);
        if (CDK_SUCCESS(rv)) {
            rv = PHY_LINK_GET(BMD_PORT_PHY_CTRL(unit, port), link, an_done);
            if (an && !an_done) {
                *link = 0;
            }
        }
        return rv;
    }
#endif
    *link = 1;
    *an_done = 1;
    return CDK_E_NONE;
}

int 
bmd_phy_autoneg_set(int unit, int port, int an)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        return PHY_AUTONEG_SET(BMD_PORT_PHY_CTRL(unit, port), an);
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_autoneg_get(int unit, int port, int *an)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        return PHY_AUTONEG_GET(BMD_PORT_PHY_CTRL(unit, port), an);
    }
#endif
    *an = 0;
    return CDK_E_NONE;
}

int 
bmd_phy_speed_set(int unit, int port, uint32_t speed)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        return PHY_SPEED_SET(BMD_PORT_PHY_CTRL(unit, port), speed);
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_speed_get(int unit, int port, uint32_t *speed)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        return PHY_SPEED_GET(BMD_PORT_PHY_CTRL(unit, port), speed);
    }
#endif
    *speed = 0;
    return CDK_E_NONE;
}

int 
bmd_phy_duplex_set(int unit, int port, int duplex)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        return PHY_DUPLEX_SET(BMD_PORT_PHY_CTRL(unit, port), duplex);
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_duplex_get(int unit, int port, int *duplex)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        return PHY_DUPLEX_GET(BMD_PORT_PHY_CTRL(unit, port), duplex);
    }
#endif
    *duplex = 0;
    return CDK_E_NONE;
}

int 
bmd_phy_loopback_set(int unit, int port, int enable)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        return PHY_LOOPBACK_SET(BMD_PORT_PHY_CTRL(unit, port), enable);
    }
#endif
    return enable ? CDK_E_UNAVAIL : CDK_E_NONE;
}

int 
bmd_phy_loopback_get(int unit, int port, int *enable)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        return PHY_LOOPBACK_GET(BMD_PORT_PHY_CTRL(unit, port), enable);
    }
#endif
    *enable = 0;
    return CDK_E_NONE;
}

int 
bmd_phy_remote_loopback_set(int unit, int port, int enable)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    int rv;

    if (BMD_PORT_PHY_CTRL(unit, port)) {
        rv = PHY_CONFIG_SET(BMD_PORT_PHY_CTRL(unit, port), 
                            PhyConfig_RemoteLoopback, enable, NULL);
        if (rv == CDK_E_UNAVAIL && !enable) {
            rv = CDK_E_NONE;
        }
        return rv;
    }
#endif
    return enable ? CDK_E_UNAVAIL : CDK_E_NONE;
}

int 
bmd_phy_remote_loopback_get(int unit, int port, int *enable)
{
    uint32_t val = 0;

#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        int rv = PHY_CONFIG_GET(BMD_PORT_PHY_CTRL(unit, port), 
                                PhyConfig_RemoteLoopback, &val, NULL);
        if (CDK_FAILURE(rv) && rv != CDK_E_UNAVAIL) {
            return rv;
        }
    }
#endif
    *enable = (int)val;
    return CDK_E_NONE;
}

int 
bmd_phy_line_interface_set(int unit, int port, int intf)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    int pref_intf;

    if (BMD_PORT_PHY_CTRL(unit, port)) {
        switch (intf) {
        case BMD_PHY_IF_XFI:
            pref_intf = PHY_IF_XFI;
            break;
        case BMD_PHY_IF_SFI:
            pref_intf = PHY_IF_SFI;
            break;
        case BMD_PHY_IF_KR:
            pref_intf = PHY_IF_KR;
            break;
        case BMD_PHY_IF_KX:
            pref_intf = PHY_IF_KX;
            break;
        case BMD_PHY_IF_CR:
            pref_intf = PHY_IF_CR;
            break;
        case BMD_PHY_IF_SR:
            pref_intf = PHY_IF_SR;
            break;
        case BMD_PHY_IF_HIGIG:
            pref_intf = PHY_IF_HIGIG;
            break;
        default:
            pref_intf = 0;
            break;
        }
        PHY_CTRL_LINE_INTF(BMD_PORT_PHY_CTRL(unit, port)) = pref_intf;
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_line_interface_get(int unit, int port, int *intf)
{
    *intf = 0;

#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        uint32_t val = 0;
        int rv = PHY_STATUS_GET(BMD_PORT_PHY_CTRL(unit, port), 
                                PhyStatus_LineInterface, &val);
        if (CDK_FAILURE(rv) && rv != CDK_E_UNAVAIL) {
            return rv;
        }
        switch (val) {
        case PHY_IF_XFI:
            *intf = BMD_PHY_IF_XFI;
            break;
        case PHY_IF_SFI:
            *intf = BMD_PHY_IF_SFI;
            break;
        case PHY_IF_KR:
            *intf = BMD_PHY_IF_KR;
            break;
        case PHY_IF_KX:
            *intf = BMD_PHY_IF_KX;
            break;
        case PHY_IF_CR:
            *intf = BMD_PHY_IF_CR;
            break;
        case PHY_IF_SR:
            *intf = BMD_PHY_IF_SR;
            break;
        case PHY_IF_HIGIG:
            *intf = BMD_PHY_IF_HIGIG;
            break;
        default:
            break;
        }
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_eee_set(int unit, int port, int mode)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        uint32_t eee_mode = PHY_EEE_NONE;
        int rv;
        if (mode == BMD_PHY_M_EEE_802_3) {
            eee_mode = PHY_EEE_802_3;
        } else if (mode == BMD_PHY_M_EEE_AUTO) {
            eee_mode = PHY_EEE_AUTO;
        }
        rv = PHY_CONFIG_SET(BMD_PORT_PHY_CTRL(unit, port),
                            PhyConfig_EEE, eee_mode, NULL);
        if (mode == BMD_PHY_M_EEE_OFF && rv == CDK_E_UNAVAIL) {
            rv = CDK_E_NONE;
        }
        return rv;
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_eee_get(int unit, int port, int *mode)
{
    *mode = BMD_PHY_M_EEE_OFF;

#if BMD_CONFIG_INCLUDE_PHY == 1
    if (BMD_PORT_PHY_CTRL(unit, port)) {
        uint32_t eee_mode;
        int rv = PHY_CONFIG_GET(BMD_PORT_PHY_CTRL(unit, port), 
                                PhyConfig_EEE, &eee_mode, NULL);
        if (CDK_FAILURE(rv) && rv != CDK_E_UNAVAIL) {
            return rv;
        }
        if (rv == CDK_E_UNAVAIL) {
            eee_mode = BMD_PHY_M_EEE_OFF;
            rv = CDK_E_NONE;
        }        
        if (eee_mode == PHY_EEE_802_3) {
            *mode = BMD_PHY_M_EEE_802_3;
        } else if (eee_mode == PHY_EEE_AUTO) {
            *mode = BMD_PHY_M_EEE_AUTO;
        }
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_fw_helper_set(int unit, int port,
                      int (*fw_helper)(void *, uint32_t, uint32_t, void *))
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, port);

    while (pc != NULL) {
        PHY_CTRL_FW_HELPER(pc) = fw_helper;
        pc = pc->next;
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_fw_info_get(void *ctx, int *unit, int *port, const char **drv_name)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_ctrl_t *pc = (phy_ctrl_t *)ctx;

    if (unit) {
        *unit = PHY_CTRL_UNIT(pc);
    }
    if (port) {
        *port = PHY_CTRL_PORT(pc);
    }
    /*
     * To handle firmware load of the single port with multi-core PHYs,
     * the adjusted port number is only used in firmware loading
     * to make sure the firmware is loading to each multi-core PHY.
     */
    if (pc->num_phys > 1 && pc->addr_offset > 0) {
        *port += pc->addr_offset;
    }
    if (drv_name && pc->drv) {
        *drv_name = pc->drv->drv_name;
    }
#endif
    return CDK_E_NONE;
}

int 
bmd_phy_fw_base_set(int unit, int port, char *name, uint32_t fw_base)
{
    int rv = CDK_E_NONE;

#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, port);

    while (pc != NULL) {
        if (name && pc->drv && pc->drv->drv_name &&
            CDK_STRSTR(pc->drv->drv_name, name) == NULL) {
            pc = pc->next;
            continue;
        }
        rv = PHY_CONFIG_SET(pc, PhyConfig_RamBase, fw_base, NULL);
        if (rv == CDK_E_UNAVAIL) {
            rv = CDK_E_NONE;
        }
        break;
    }
#endif
    return rv;
}

int 
bmd_phy_default_ability_set(int unit, int port, uint32_t ability)
{
    int rv = CDK_E_NONE;

#if BMD_CONFIG_INCLUDE_PHY == 1
    uint32_t phy_abil = 0;
    
    if (BMD_PORT_PHY_CTRL(unit, port)) {        
        if (ability & BMD_PHY_ABIL_10MB_HD) {
            phy_abil |= PHY_ABIL_10MB_HD;
        }
        if (ability & BMD_PHY_ABIL_10MB_FD) {
            phy_abil |= PHY_ABIL_10MB_FD;
        }
        if (ability & BMD_PHY_ABIL_100MB_HD) {
            phy_abil |= PHY_ABIL_100MB_HD;
        }
        if (ability & BMD_PHY_ABIL_100MB_FD) {
            phy_abil |= PHY_ABIL_100MB_FD;
        }
        if (ability & BMD_PHY_ABIL_1000MB_HD) {
            phy_abil |= PHY_ABIL_1000MB_HD;
        }
        if (ability & BMD_PHY_ABIL_1000MB_FD) {
            phy_abil |= PHY_ABIL_1000MB_FD;
        }
        if (ability & BMD_PHY_ABIL_2500MB) {
            phy_abil |= PHY_ABIL_2500MB;
        }
        if (ability & BMD_PHY_ABIL_10GB) {
            phy_abil |= PHY_ABIL_10GB;
        }
        if (ability & BMD_PHY_ABIL_13GB) {
            phy_abil |= PHY_ABIL_13GB;
        }
        if (ability & BMD_PHY_ABIL_16GB) {
            phy_abil |= PHY_ABIL_16GB;
        }
        if (ability & BMD_PHY_ABIL_20GB) {
            phy_abil |= PHY_ABIL_20GB;
        }
        if (ability & BMD_PHY_ABIL_21GB) {
            phy_abil |= PHY_ABIL_21GB;
        }
        if (ability & BMD_PHY_ABIL_25GB) {
            phy_abil |= PHY_ABIL_25GB;
        }
        if (ability & BMD_PHY_ABIL_30GB) {
            phy_abil |= PHY_ABIL_30GB;
        }
        if (ability & BMD_PHY_ABIL_40GB) {
            phy_abil |= PHY_ABIL_40GB;
        }
        if (ability & BMD_PHY_ABIL_100GB) {
            phy_abil |= PHY_ABIL_100GB;
        }

        bmd_phy_default_ability[unit][port] = phy_abil;
    }
#endif
    return rv;
}

int
bmd_phy_set_slave(int unit, int port, int slave_port, int slave_idx)
{
#if BMD_CONFIG_INCLUDE_PHY == 1
    phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, port);
    phy_ctrl_t *pc_slave = BMD_PORT_PHY_CTRL(unit, slave_port);

    if (slave_idx < 0 || slave_idx >= 2) {
        /* Support max 2 slave PHYs for now */
        return CDK_E_PARAM;
    }

    if (pc) {
        pc->slave[slave_idx] = pc_slave;
    }
#endif
    return CDK_E_NONE;
}
