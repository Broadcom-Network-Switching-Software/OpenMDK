#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM56340_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <bmd/bmd.h>

#include <bmdi/arch/xgsm_mac_util.h>

#include <cdk/chip/bcm56340_a0_defs.h>
#include <cdk/arch/xgsm_schan.h>
#include <cdk/cdk_debug.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>

#include "bcm56340_a0_bmd.h"
#include "bcm56340_a0_internal.h"

static int
l2x_insert(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr)
{
    int lport;
    int ipipe_blk;
    L2_ENTRY_1m_t l2x;
    uint32_t fval[2];
    schan_msg_t schan_msg;
    lport = P2L(unit, port);

    L2_ENTRY_1m_CLR(l2x);
    xgsm_mac_to_field_val(mac_addr->b, fval);
    L2_ENTRY_1m_L2_MAC_ADDRf_SET(l2x, fval);
    L2_ENTRY_1m_L2_VLAN_IDf_SET(l2x, vlan);
    L2_ENTRY_1m_L2_PORT_NUMf_SET(l2x, lport);
    L2_ENTRY_1m_L2_LIMIT_COUNTEDf_SET(l2x, 1);
    L2_ENTRY_1m_STATIC_BITf_SET(l2x, 1);
    L2_ENTRY_1m_VALIDf_SET(l2x, 1);

    if ((ipipe_blk = cdk_xgsm_block_number(unit, BLKTYPE_ISM, 0)) < 0) {
        return CDK_E_INTERNAL;
    }

    /* Write message to S-Channel */
    SCHAN_MSG_CLEAR(&schan_msg);
    SCMH_OPCODE_SET(schan_msg.gencmd.header, TABLE_INSERT_CMD_MSG);
    SCMH_DSTBLK_SET(schan_msg.gencmd.header, ipipe_blk); 
    SCMH_DATALEN_SET(schan_msg.gencmd.header, 16); 
    CDK_MEMCPY(schan_msg.gencmd.data, &l2x, sizeof(l2x));
    schan_msg.gencmd.address = L2_ENTRY_1m;

    /* Write header word + L2X entry */
    return cdk_xgsm_schan_op(unit, &schan_msg, 6, 0);
}

int
bcm56340_a0_bmd_port_mac_addr_add(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr)
{
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    return l2x_insert(unit, port, vlan, mac_addr);
}

#endif /* CDK_CONFIG_INCLUDE_BCM56340_A0 */
