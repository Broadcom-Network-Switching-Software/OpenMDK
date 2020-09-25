/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK simulator hook definitions.
 *
 * These generic access functions support reading and writing an
 * arbitrary number of bytes at a 64-bit address.
 *
 * The hooks are intended for simple software simulators that
 * do not implement indirect access protocols such as the XGS
 * S-channel message protocol.
 *
 * Note that the software simulator itself must be provided by 
 * the application.
 */

#include <cdk/cdk_simhook.h>

int (*cdk_simhook_read)(int unit, uint32_t addrx, uint32_t addr,
                        void *vptr, int size);
int (*cdk_simhook_write)(int unit, uint32_t addrx, uint32_t addr,
                         void *vptr, int size);
