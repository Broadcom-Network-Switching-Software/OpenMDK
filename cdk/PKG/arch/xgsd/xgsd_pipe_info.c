/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGSd pipe access control.
 */

#include <cdk/cdk_device.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_debug.h>

#include <cdk/arch/xgsd_chip.h>
#include <cdk/arch/xgsd_reg.h>

/*******************************************************************************
 *
 * Provide information about pipe-based access,
 *
 * The function will work in two modes depending on the baseidx
 * parameter:
 *
 * If baseidx is negative, then the function will return the number
 * of logical instances of this register/memory in bits [15:8] as
 * well as a bitmap of the addresable physical instances in bits [7:0]
 *
 * If baseidx is valid, then the function will return a bitmap of the
 * valid physical instances for this base index.
 *
 * If a regsiter/memory does not use pipe control, then function will
 * return 0.
 */

uint32_t
cdk_xgsd_pipe_info(int unit, uint32_t addr, int acctype,
                   int blktype, int baseidx)
{
    if (CDK_XGSD_INFO(unit)->pipe_info) {
        return CDK_XGSD_INFO(unit)->pipe_info(addr, acctype, blktype, baseidx);
    }
    
    return 0;
}
