/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Common XGS chip functions.
 */

#include <cdk/cdk_assert.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_chip.h>
#include <cdk/arch/xgsd_chip.h>

int
cdk_xgsd_port_number(int unit, int block, int port)
{
    /* 
     * Given a block number and physical port, return the 
     * logical port number.
     */
    int p; 
    cdk_xgsd_pblk_t pb; 
    int blktype;

    cdk_xgsd_block_type(unit, block, &blktype, NULL);

    for (p = 0; p < CDK_CONFIG_MAX_PORTS; p++) {
        if (cdk_xgsd_port_block(unit, p, &pb, blktype) >= 0) {
            if (pb.block == block && pb.bport == port) {
                return p; 
            }
        }
    }
    return -1; 
}
