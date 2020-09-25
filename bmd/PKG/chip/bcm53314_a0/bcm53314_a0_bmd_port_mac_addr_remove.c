#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53314_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/xgs_mac_util.h>

#include <cdk/chip/bcm53314_a0_defs.h>
#include <cdk/arch/xgs_schan.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>

#include "bcm53314_a0_bmd.h"

static int
l2x_delete(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr)
{
    int ipipe_blk;
    L2Xm_t l2x;
    uint32_t fval[2];
    schan_msg_t schan_msg;

    L2Xm_CLR(l2x);
    xgs_mac_to_field_val(mac_addr->b, fval);
    L2Xm_MAC_ADDRf_SET(l2x, fval);
    L2Xm_VLAN_IDf_SET(l2x, vlan);
    L2Xm_PORT_TGIDf_SET(l2x, port);
    L2Xm_VALIDf_SET(l2x, 1);

    if ((ipipe_blk = cdk_xgs_block_number(unit, BLKTYPE_IPIPE, 0)) < 0) {
        return CDK_E_INTERNAL;
    }

    /* Write message to S-Channel */
    SCHAN_MSG_CLEAR(&schan_msg);
    SCMH_OPCODE_SET(schan_msg.arldel.header, ARL_DELETE_CMD_MSG);
    SCMH_SRCBLK_SET(schan_msg.arldel.header, CDK_XGS_CMIC_BLOCK(unit)); 
    SCMH_DSTBLK_SET(schan_msg.arldel.header, ipipe_blk); 
    SCMH_DATALEN_SET(schan_msg.arldel.header, 12); 
    CDK_MEMCPY(schan_msg.arldel.data, &l2x, sizeof(l2x));

    /* Write header word + L2X entry */
    return cdk_xgs_schan_op(unit, &schan_msg, 4, 0);
}

int
bcm53314_a0_bmd_port_mac_addr_remove(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr)
{
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    return l2x_delete(unit, port, vlan, mac_addr);
}

#endif /* CDK_CONFIG_INCLUDE_BCM53314_A0 */
