/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __PHY_REG_H__
#define __PHY_REG_H__

#include <phy/phy.h>

/*
 * Generic 32-bit PHY register layout:
 *
 * [31:28] - Access method
 * [27:24] - Flags for access method
 * [23:0]  - Register address
 *
 *
 * Access method 0 (Raw)
 *
 * For clause 22 access, register address is in bits [4:0]. For
 * clause 45 access, register address is in bits [15:0] and
 * DEVAD is in bits [20:16].
 *
 * 
 * Access method 1 (Broadcom Gigabit/Fast Ethernet shadow)
 *
 * If register is shadowed (register 18h, 1Ch, etc.) then shadow
 * number is in bits [15:8] and base register is in bits [4:0].
 * If register is an expansion register (registers 15h/17h), then
 * register address is in bits [23:8] and bits [4:0] is 15h.
 * If register is a clause 45 register (registers 0Dh/0Eh), then
 * clause 45 device type is in  bits [27:24], clause 45 register
 * address is in bits [23:8] and bits [4:0] is 0Eh.
 *
 *
 * Access method 2 (Broadcom Gigabit/1000BASE-X registers)
 *
 * Base address is in bits [4:0]. The 1000BASE-X registers will
 * be mapped into the IEEE space before register is accessed.
 *
 *
 * Access method 3 (Internal SerDes PHY block-based)
 *
 * Block address is in bits [23:8] (will be written to register.
 * 1Fh). Base address is in bits [4:0]. For clause 45 devices
 * the DEVAD is stored in the flags field (bits [26:24]).
 *
 *
 * Access method 4 (Internal XAUI/SerDes PHY with IEEE-map)
 *
 * Identical to access method 3, but supports the following
 * access flag instead of clause 45 DEVAD:
 *
 * Bit 3 - If set, map SerDes registers before reading IEEE 
 *         register. otherwise map XAUI registers before 
 *         reading IEEE register.
 *         
 *
 * Access method 5 (Internal SerDes PHY clause 45 with AER)
 *
 * Clause 45 address is in bits [15:0] and DEVAD in bits [19:16].
 * If a register is not duplicated for each XAUI lane, the actual
 * number of copies (1 or 2) will be stored in bits [22:20].
 * If clause 45 access is not supported the access will be done
 * using clause 22 block-mapped access.
 *
 * This method supports the following flags:
 *
 * Bit 3 - If set, force write to lane specified in flag
 *         bits [2:0].
 *
 * Bit 2 - If set, broadcast writes to all lanes if not in
 *         independent lane mode (aka. serdes mode).
 *         Ignored if flag bit 3 is set (force lane).
 *
 * In addition to the standard flags, bit 23 is used as a
 * write-only flag.
 *
 *
 * Access method 6 (Broadcom 10-Gigabit registers)
 *
 * Standard clause 45 access with additional support for Gigabit
 * shadow and extension register. Also supports multiple PHYs
 * (each with multiple caluse 45 device types) for a single PHY
 * address.
 * If register is a standard clause 45 register, then clause 45
 * device type is in bits [20:16] and register address is in
 * bits [15:0].
 * If register is shadowed (register 18h, 1Ch, etc.) then shadow
 * number is in bits [15:8] and base register is in bits [4:0].
 * If register is an expansion register (registers 15h/17h), then
 * register address is in bits [23:8] and bits [4:0] is 15h.
 *
 * This method supports the following flags:
 *
 * Bit 3 - Use shadow/extension register access.
 *         
 * Flag bits [2:0] may be used to select an alternative PHY device.
 *
 *
 * Access method 7 (Internal TSC-based SerDes PHY with IBLK access)
 *
 * Clause 45 address is in bits [15:0] and DEVAD in bits [19:16].
 * If a register is not duplicated for each XAUI lane, the actual
 * number of copies (1 or 2) will be stored in bits [22:20].
 * Bit 23 is used as a write-only flag.
 *
 * Bits [2:0] of the flags field may be used to specify a different
 * lane.
 *
 *
 * Access method 8 (Broadcom Gigabit shadowed registers)
 *
 * Base address is in bits [15:0].
 *
 *
 * Access method 9 (Internal TSC-based SerDes PHY with SBUS access)
 *
 * Clause 45 address is in bits [15:0] and DEVAD in bits [19:16].
 * If a register is not duplicated for each XAUI lane, the actual
 * number of copies (1 or 2) will be stored in bits [22:20].
 * Bit 23 is used as a write-only flag.
 *
 * Bits [1:0] of the flags field may be used to specify a different
 * PLL domain.
 * Access method 7 (TSC registers)
 *
 * This method supports the following flags:
 *
  * Bit 27 - If set, force write to lane specified in flag bits [26:24].
 * Bit 26 - If set, Broadcast to all lanes if multiple copies.
 * Bit 26:24 - If bit 27 been set, force write to lane specified.
 * Bit 25:24 - PLL index.
 * Bit 24 - If set, setting pll_index in flag bits [25:24].
 * Bit 23 - write-only flag.
 * Bit 22:20 - reg_copies
 * Bit 19:16 - devad
 */

/* Register layout */
#define PHY_REG_ACCESS_METHOD_SHIFT     28
#define PHY_REG_ACCESS_FLAGS_SHIFT      24

/* Access methods */
#define PHY_REG_ACC_RAW                 LSHIFT32(0, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_BRCM_SHADOW         LSHIFT32(1, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_BRCM_1000X          LSHIFT32(2, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_XGS_IBLK            LSHIFT32(3, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_XAUI_IBLK           LSHIFT32(4, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_AER_IBLK            LSHIFT32(5, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_BRCM_XE             LSHIFT32(6, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_TSC_IBLK            LSHIFT32(7, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_BRCM_RDB            LSHIFT32(8, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_TSC_SBUS            LSHIFT32(9, PHY_REG_ACCESS_METHOD_SHIFT)

#define PHY_REG_ACCESS_METHOD(_r)       ((_r) & LSHIFT32(0xf, PHY_REG_ACCESS_METHOD_SHIFT))

#define PHY_REG_IBLK_DEVAD(_r)          (((_r) >> PHY_REG_ACCESS_FLAGS_SHIFT) & 0x7)

/* Spacing between elements in register arrays */
#define PHY_REG_ACC_XGS_IBLK_STEP       0x1000
#define PHY_REG_ACC_XAUI_IBLK_STEP      0x1000
#define PHY_REG_ACC_AER_IBLK_STEP       0x10

/* Flags for PHY_REG_ACC_XAUI_IBLK method */
#define PHY_REG_ACC_XAUI_IBLK_CL22      LSHIFT32(0x8, PHY_REG_ACCESS_FLAGS_SHIFT)

/* Flags for PHY_REG_ACC_AER_IBLK method */
#define PHY_REG_ACC_AER_IBLK_FORCE_LANE LSHIFT32(0x8, PHY_REG_ACCESS_FLAGS_SHIFT)
#define PHY_REG_ACC_AER_IBLK_BCAST      LSHIFT32(0x4, PHY_REG_ACCESS_FLAGS_SHIFT)
#define PHY_REG_ACC_AER_IBLK_WR_ONLY    LSHIFT32(1, 23)

/* Flags for PHY_REG_ACC_BRCM_XE method */
#define PHY_REG_ACC_BRCM_XE_SHADOW      LSHIFT32(0x8, PHY_REG_ACCESS_FLAGS_SHIFT)

/* Flags for PHY_REG_ACC_TSC_IBLK method */
#define PHY_REG_ACC_TSC_IBLK_FORCE_LANE LSHIFT32(0x8, PHY_REG_ACCESS_FLAGS_SHIFT)
#define PHY_REG_ACC_TSC_IBLK_BCAST      LSHIFT32(0x4, PHY_REG_ACCESS_FLAGS_SHIFT)
#define PHY_REG_ACC_TSC_IBLK_PLL        LSHIFT32(0x3, PHY_REG_ACCESS_FLAGS_SHIFT)
#define PHY_REG_ACC_TSC_IBLK_WR_ONLY    LSHIFT32(1, 23)
#define PHY_REG_ACC_TSC_IBLK_PLL_SHIFT  24
#define PHY_REG_ACC_TSC_IBLK_PLL_IDX(_r)     (((_r) >> 24) & 0x3)

/* Flags for PHY_REG_ACC_TSC_SBUS method */
#define PHY_REG_ACC_TSC_SBUS_WR_ONLY    LSHIFT32(1, 23)
#define PHY_REG_ACC_TSC_SBUS_PLL(_r)    (((_r) >> PHY_REG_ACCESS_FLAGS_SHIFT) & 0x3)

extern int
phy_reg_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data);

extern int
phy_reg_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data);

#endif /* __PHY_REG_H__ */
