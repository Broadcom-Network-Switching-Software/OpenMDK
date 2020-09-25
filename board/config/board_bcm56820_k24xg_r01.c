/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 */

/*
 * This port configuration is intended for testing Scorpion in the
 * default 10G mode (BCM956820K24XG board).
 */

#include <board/board_config.h>
#include <cdk/cdk_string.h>
#include <phy/phy_buslist.h>

static int 
_phy_reset_cb(phy_ctrl_t *pc)
{
    int rv = CDK_E_NONE;
    int port;
    phy_ctrl_t *lpc = pc;
    uint32_t tx_pol;

    while (lpc) {
        if (lpc->drv == NULL || lpc->drv->drv_name == NULL) {
            return CDK_E_INTERNAL;
        }
        if (CDK_STRSTR(lpc->drv->drv_name, "unicore16g_xgxs") != NULL) {
            port = PHY_CTRL_PORT(lpc);
            /* Invert Tx polarity on lane 3 of port 17 */
            if (port == 17) {
                tx_pol = 0x1000; /* Invert lane 3 */
                rv = PHY_CONFIG_SET(lpc, PhyConfig_XauiTxPolInvert,
                                    tx_pol, NULL);
                PHY_VERB(lpc, ("Flip Tx pol (0x%04"PRIx32")\n", tx_pol));
            }
        }
        lpc = lpc->next;
    }

    return rv;
}

static int 
_phy_init_cb(phy_ctrl_t *pc)
{
    return CDK_E_NONE;
}

board_config_t board_bcm56820_k24xg_r01 = {
    "bcm56820_k24xg_r01",
    NULL,
    &_phy_reset_cb,
    &_phy_init_cb,
};
