/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BMD_PORT_MODE_H__
#define __BMD_PORT_MODE_H__

#include <bmd_config.h>

extern int
bmd_port_mode_from_speed_duplex(uint32_t speed, int duplex, bmd_port_mode_t *mode);

extern int
bmd_port_mode_from_phy(int unit, int port,
                       bmd_port_mode_t *mode, uint32_t *flags);

extern int
bmd_port_mode_to_phy(int unit, int port, bmd_port_mode_t mode,
                     uint32_t flags, uint32_t speed, int duplex);

#endif /* __BMD_PORT_MODE_H__ */
