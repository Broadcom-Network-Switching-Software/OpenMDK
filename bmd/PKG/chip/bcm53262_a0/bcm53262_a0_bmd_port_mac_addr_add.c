#include <bmd_config.h>
#if CDK_CONFIG_INCLUDE_BCM53262_A0 == 1

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

#include <cdk/chip/bcm53262_a0_defs.h>

#include <cdk/arch/robo_mem_regs.h>

#include <cdk/cdk_debug.h>
#include <cdk/cdk_device.h>
#include <cdk/cdk_error.h>

#include "bcm53262_a0_internal.h"
#include "bcm53262_a0_bmd.h"

#define NUM_BINS 4
#define MAX_POLL 20

static int
_arl_op(int unit, int opcode)
{
    int ioerr = 0;
    int cnt;
    ARLA_RWCTLr_t arla_rwctl;

    ARLA_RWCTLr_CLR(arla_rwctl);
    ARLA_RWCTLr_ARLA_RWCTL_RSRV0f_SET(arla_rwctl, opcode);
    ARLA_RWCTLr_ARL_STRTDNf_SET(arla_rwctl, 1);
    ioerr += WRITE_ARLA_RWCTLr(unit, arla_rwctl);

    cnt = 0;
    while (cnt++ < MAX_POLL) {
        ioerr += READ_ARLA_RWCTLr(unit, &arla_rwctl);
        if (ioerr == 0 && 
            ARLA_RWCTLr_ARL_STRTDNf_GET(arla_rwctl) == 0) {
            return CDK_E_NONE;
        }
    }

    return ioerr ? CDK_E_IO : CDK_E_TIMEOUT;
}

int
bcm53262_a0_arl_write(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr)
{
    int ioerr = 0;
    int rv;
    ARLA_MACr_t arla_mac;
    ARLA_VIDr_t arla_vid; 
    ARLA_VID_ENTRY_0r_t arla_vid_entry, arla_vid_entry_chk;
    ARLA_ENTRY_0r_t arla_fwd, fwd_chk;
    uint32_t vid_base = ARLA_VID_ENTRY_0r;
    uint32_t vid_inc = ARLA_VID_ENTRY_1r - ARLA_VID_ENTRY_0r;
    int vid_size = ARLA_VID_ENTRY_0r_SIZE;
    uint32_t fwd_base = ARLA_ENTRY_0r;
    uint32_t fwd_inc = ARLA_ENTRY_1r - ARLA_ENTRY_0r;
    int fwd_size = ARLA_ENTRY_0r_SIZE;
    uint32_t reg;
    uint32_t fval[2];
    int idx, bin_no, dyn_bin;
    /* Convert MAC address to standard field value */
    robo_mac_to_field_val(mac_addr->b, fval);

    /* Create ARL entry */
    ARLA_VID_ENTRY_0r_CLR(arla_vid_entry);
    ARLA_VID_ENTRY_0r_ARLA_VIDf_SET(arla_vid_entry, vlan);
    
    ARLA_ENTRY_0r_CLR(arla_fwd);
    /* If port is negative, then clear the entry */
    if (port >= 0) {
        ARLA_ENTRY_0r_ARL_PIDf_SET(arla_fwd, port+24);
        ARLA_ENTRY_0r_ARL_MACADDRf_SET(arla_fwd, fval);        
        ARLA_ENTRY_0r_ARL_STATICf_SET(arla_fwd, 1);
        ARLA_ENTRY_0r_ARL_VALIDf_SET(arla_fwd, 1);
        ARLA_ENTRY_0r_ARL_MACADDRf_GET(arla_fwd,fval);
    }

    /* Set up MAC and VLAN for hash index calculation */
    ARLA_MACr_CLR(arla_mac);
    ARLA_MACr_MAC_ADDR_INDXf_SET(arla_mac, fval);
    ioerr += WRITE_ARLA_MACr(unit, arla_mac);

    ARLA_VIDr_CLR(arla_vid);
    ARLA_VIDr_ARLA_VIDTAB_INDXf_SET(arla_vid, vlan);
    ioerr += WRITE_ARLA_VIDr(unit, arla_vid);
    /* Read ARL entry */
    rv = _arl_op(unit, ROBO_MEM_OP_READ);

    /* Find matching/available bin */
    bin_no = -1;
    dyn_bin = -1;
    for (idx = 0; idx < NUM_BINS; idx++) {
        reg = fwd_base + (idx * fwd_inc);
        ioerr += cdk_robo_reg_read(unit, reg, &fwd_chk, fwd_size);
        if (ARLA_ENTRY_0r_ARL_VALIDf_GET(fwd_chk)) {
            if (ARLA_ENTRY_0r_ARL_STATICf_GET(fwd_chk) == 0) {
                /* Track valid dynamic bins */
                CDK_VVERB(("bcm53262_a0_arl_write: dynamic bin %d\n", idx));
                dyn_bin = idx;
            }
            reg = vid_base + (idx * vid_inc);
           ioerr += cdk_robo_reg_read(unit, reg, &arla_vid_entry_chk, vid_size);
          if (CDK_MEMCMP(&arla_vid_entry_chk, &arla_vid_entry, sizeof(&arla_vid_entry_chk)) == 0) {
                /* Found a matching key */
                CDK_VVERB(("bcm53262_a0_arl_write: matching bin %d\n", idx));
                bin_no = idx;
                break;
            }
        } else if (bin_no < 0) {
            /* First empty bin */
            CDK_VVERB(("bcm53262_a0_arl_write: empty bin %d\n", idx));
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

    /* Check for errors */
    if (ioerr) {
        return CDK_E_IO;
    }

    if (CDK_SUCCESS(rv)) {
        /* Write new ARL entry to selected bin */
        reg = vid_base + (bin_no * vid_inc);
        ioerr += cdk_robo_reg_write(unit, reg, &arla_vid_entry, vid_size);
        reg = fwd_base + (bin_no * fwd_inc);
        ioerr += cdk_robo_reg_write(unit, reg, &arla_fwd, fwd_size);
        /* Write ARL entry */
        rv = _arl_op(unit, ROBO_MEM_OP_WRITE);
    }

    return ioerr ? CDK_E_IO : rv;
}

int
bcm53262_a0_bmd_port_mac_addr_add(int unit, int port, int vlan, const bmd_mac_addr_t *mac_addr)
{
    BMD_CHECK_UNIT(unit);
    BMD_CHECK_VLAN(unit, vlan);
    BMD_CHECK_PORT(unit, port);

    return bcm53262_a0_arl_write(unit, port, vlan, mac_addr);
}

#endif /* CDK_CONFIG_INCLUDE_BCM53262_A0 */
