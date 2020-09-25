/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __PHY_TSC_SBUS_H__
#define __PHY_TSC_SBUS_H__

#include <phy/phy_reg.h>

/*
 * Always set bit 15 in block address register 0x1f to make the
 * register contents more similar to the clause 45 address.
 */
#ifndef PHY_TSC_SBUS_DBG_BIT15
#define PHY_TSC_SBUS_DBG_BIT15  1
#endif

/* Special lane values for broadcast and dual-lane multicast */
#define PHY_TSC_SBUS_MCAST01    4
#define PHY_TSC_SBUS_MCAST23    5
#define PHY_TSC_SBUS_BCAST      6

/* Host/uC mailbox control */
#define TSC_UC_PROXY_WAIT_TIME  600000
#define TSC_UC_SYNC_CMD_REQ     (1L << 0)
#define TSC_UC_SYNC_CMD_RDY     (1L << 1)
#define TSC_UC_SYNC_CMD_BUSY    (1L << 2)
#define TSC_UC_SYNC_CMD_RAM     (1L << 3)
#define TSC_UC_SYNC_CMD_WR      (1L << 4)
#define TSC_UC_SYNC_CMD_DONE    (1L << 15)


extern int
phy_tsc_sbus_mdio_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);

extern int
phy_tsc_sbus_mdio_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

extern int
phy_tsc_sbus_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);

extern int
phy_tsc_sbus_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

#endif /* __PHY_TSC_SBUS_H__ */
