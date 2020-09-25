/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * ROBO register access functions.
 */

#ifndef __ROBO_REG_H__
#define __ROBO_REG_H__

#include <cdk/cdk_util.h>

#include <cdk/arch/robo_chip.h>

extern int
cdk_robo_reg_read(int unit, uint32_t addr, void *entry_data, int size);

extern int
cdk_robo_reg_write(int unit, uint32_t addr, void *entry_data, int size);

extern int
cdk_robo_reg_port_read(int unit, int port, uint32_t addr, void *vptr, int size);

extern int
cdk_robo_reg_port_write(int unit, int port, uint32_t addr, void *vptr, int size);

#endif /* __ROBO_REG_H__ */
