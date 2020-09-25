#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53600_A0 == 1

/*
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * The ARL table has 4 bins per index.
 * The table index is a hash based on MAC address and VLAN ID.
 *
 * When writing the ARL table we look for an available
 * bin using the following priorities:
 *
 *   1. Key match
 *   2. Empty bin
 *   3. Bin with dynamic entry
 */

#include <bmd/bmd.h>

#include <bmdi/arch/robo_mac_util.h>

#include <cdk/chip/bcm53600_a0_defs.h>

#include <cdk/arch/robo_mem_regs.h>

#include <cdk/cdk_debug.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>

#include "bcm53600_a0_internal.h"
#include "bcm53600_a0_bmd.h"

#define NUM_BINS 4
#define MAX_POLL 20
#define MEM_OP_IDX_READ    0x03
#define MEM_OP_IDX_WRITE   0x04
#define VA_BITS       3

static int
_arl_op(int unit, int opcode)
{
    int ioerr = 0;
    int cnt;
    MEM_CTRLr_t mem_ctrl;

    ioerr += READ_MEM_CTRLr(unit, &mem_ctrl);
    MEM_CTRLr_OP_CMDf_SET(mem_ctrl, opcode);
    MEM_CTRLr_MEM_STDNf_SET(mem_ctrl, 1);
    ioerr += WRITE_MEM_CTRLr(unit, mem_ctrl);

    cnt = 0;
    while (cnt++ < MAX_POLL) {
        ioerr += READ_MEM_CTRLr(unit, &mem_ctrl);
        if (ioerr == 0 && 
            MEM_CTRLr_MEM_STDNf_GET(mem_ctrl) == 0) {
            return CDK_E_NONE;
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
}

int
bcm53600_a0_arl_write(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr)
{
    int ioerr = 0;
    int rv;
    MEM_INDEXr_t mem_index;
    MEM_KEY_0r_t mem_key_0;
    MEM_KEY_1r_t mem_key_1;
    uint32_t mac_val[2];
    uint32_t mac_cmp[2];
    L2_ARLm_t *l2_arl;
    MEM_DATAr_t mem_data[2];
    int idx, bin_no, dyn_bin, vlan_cmp;

    /* Convert MAC address to standard field value */
    robo_mac_to_field_val(mac_addr->b, mac_val);

    /* Let mem_data be an L2_ARL entry */
    l2_arl = (L2_ARLm_t *)mem_data;

    /* Select ARL table */
    ioerr += READ_MEM_INDEXr(unit, &mem_index);
    MEM_INDEXr_INDEXf_SET(mem_index, 0x01);
    ioerr += WRITE_MEM_INDEXr(unit, mem_index);

    /* Set search key using MACADDR[47:0] */
    MEM_KEY_0r_CLR(mem_key_0);
    MEM_KEY_0r_SET(mem_key_0, 0, mac_val[0]);
    MEM_KEY_0r_SET(mem_key_0, 1, mac_val[1]);
    ioerr += WRITE_MEM_KEY_0r(unit, mem_key_0);
    MEM_KEY_1r_CLR(mem_key_1);
    MEM_KEY_1r_SET(mem_key_1, 0, vlan);
    ioerr += WRITE_MEM_KEY_1r(unit, mem_key_1);
    rv = _arl_op(unit, MEM_OP_IDX_READ);

    /* Right-shift MAC address 12 bits to get MACADDR[47:12] */
    mac_val[0] >>= 12;
    mac_val[0] |= LSHIFT32(mac_val[1], (32 - 12));
    mac_val[1] >>= 12;

    /* Find matching/available bin */
    bin_no = -1;
    dyn_bin = -1;
    for (idx = 0; idx < NUM_BINS; idx++) {
        ioerr += READ_MEM_DATAr(unit, idx << 1, &mem_data[0]);
        ioerr += READ_MEM_DATAr(unit, (idx << 1) + 1, &mem_data[1]);
        if (L2_ARLm_VAf_GET(*l2_arl) == VA_BITS) {
            if (L2_ARLm_STATICf_GET(*l2_arl) == 0) {
                /* Track valid dynamic bins */
                CDK_VVERB(("bcm53600_a0_arl_write: dynamic bin %d\n", idx));
                dyn_bin = idx;
            }
            L2_ARLm_MACADDR_47_12f_GET(*l2_arl, mac_cmp);
            vlan_cmp = L2_ARLm_VIDf_GET(*l2_arl);
            if (CDK_MEMCMP(mac_val, mac_cmp, sizeof(mac_val)) == 0 &&
                vlan == vlan_cmp) {
                /* Found a matching key */
                CDK_VVERB(("bcm53600_a0_arl_write: matching bin %d\n", idx));
                bin_no = idx;
                break;
            }
        } else if (bin_no < 0) {
            /* First empty bin */
            CDK_VVERB(("bcm53600_a0_arl_write: empty bin %d\n", idx));
            bin_no = idx;
        }
    }

    if (bin_no < 0) {
        if (dyn_bin < 0) {
            return CDK_E_FULL;
        }
        /* Overwrite dynamic ARL entry */
        bin_no = dyn_bin;
    }

    /* Create ARL entry */
    CDK_MEMSET(mem_data, 0, sizeof(mem_data));
    L2_ARLm_MACADDR_47_12f_SET(*l2_arl, mac_val);
    L2_ARLm_VIDf_SET(*l2_arl, vlan);
    if (port >= 0) {
        L2_ARLm_PORTIDf_SET(*l2_arl, port);
        L2_ARLm_STATICf_SET(*l2_arl, 1);
        L2_ARLm_VAf_SET(*l2_arl, VA_BITS);
    }

    if (ioerr == 0 && CDK_SUCCESS(rv)) {
        /* Write new ARL entry to selected bin */
        ioerr += WRITE_MEM_DATAr(unit, bin_no << 1, mem_data[0]);
        ioerr += WRITE_MEM_DATAr(unit, (bin_no << 1) + 1, mem_data[1]);
        /* Write ARL entry */
        rv = _arl_op(unit, MEM_OP_IDX_WRITE);
    }

    return ioerr ? CDK_E_IO : rv;
}

int
bcm53600_a0_bmd_port_mac_addr_add(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr)
{
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    return bcm53600_a0_arl_write(unit, port, vlan, mac_addr);
}

#endif /* CDK_CONFIG_INCLUDE_BCM53600_A0 */
