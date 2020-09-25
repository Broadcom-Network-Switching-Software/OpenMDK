/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53570_A0 == 1

#include <bmd/bmd.h>
#include <bmdi/arch/xgsd_mac_util.h>

#include <cdk/arch/xgsd_schan.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>
#include <cdk/chip/bcm53570_a0_defs.h>

#include "bcm53570_a0_bmd.h"
#include "bcm53570_a0_internal.h"

static int
l2x_delete(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr)
{
    int lport;
    int ipipe_blk;
    L2Xm_t l2x;
    uint32_t fval[2];
    schan_msg_t schan_msg;

    lport = P2L(unit, port);

    L2Xm_CLR(l2x);
    xgsd_mac_to_field_val(mac_addr->b, fval);
    L2Xm_L2_MAC_ADDRf_SET(l2x, fval);
    L2Xm_L2_VLAN_IDf_SET(l2x, vlan);
    L2Xm_L2_PORT_NUMf_SET(l2x, lport);
    L2Xm_VALIDf_SET(l2x, 1);

    if ((ipipe_blk = cdk_xgsd_block_number(unit, BLKTYPE_IPIPE, 0)) < 0) {
        return CDK_E_INTERNAL;
    }

    /* Write message to S-Channel */
    SCHAN_MSG_CLEAR(&schan_msg);
    SCMH_OPCODE_SET(schan_msg.gencmd.header, TABLE_DELETE_CMD_MSG);
    SCMH_DSTBLK_SET(schan_msg.gencmd.header, ipipe_blk); 
    SCMH_DATALEN_SET(schan_msg.gencmd.header, 16); 
    CDK_MEMCPY(schan_msg.gencmd.data, &l2x, sizeof(l2x));
    schan_msg.gencmd.address =  L2Xm;

    /* Write header word + L2X entry */
    return cdk_xgsd_schan_op(unit, &schan_msg, 4, 0);
}

int
bcm53570_a0_bmd_port_mac_addr_remove(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr)
{
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    return l2x_delete(unit, port, vlan, mac_addr);
}

#endif /* CDK_CONFIG_INCLUDE_BCM53570_A0 */
