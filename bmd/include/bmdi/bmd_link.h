/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __BMD_LINK_H__
#define __BMD_LINK_H__

#include <bmd_config.h>

extern int
bmd_link_update(int unit, int port, int *status_change);

#endif /* __BMD_LINK_H__ */
