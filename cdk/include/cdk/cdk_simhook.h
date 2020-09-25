/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK simulator hook definitions.
 */

#ifndef __CDK_SIMHOOK_H__
#define __CDK_SIMHOOK_H__

#include <cdk/cdk_types.h>

extern int (*cdk_simhook_read)(int unit, uint32_t addrx, uint32_t addr,
                               void *vptr, int size);
extern int (*cdk_simhook_write)(int unit, uint32_t addrx, uint32_t addr,
                                void *vptr, int size);

#endif /* __CDK_SIMHOOK_H__ */
