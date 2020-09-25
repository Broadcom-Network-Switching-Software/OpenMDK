/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * MIIM access.
 */

#ifndef __XGSD_MIIM_H__
#define __XGSD_MIIM_H__

/* 
 * Definitions for 'phy_addr' parameter.
 *
 * Note 1:
 * CDK_XGSD_MIIM_BUS_2 overrides CDK_XGSD_MIIM_BUS_1.
 *
 * Note 2:
 * If neither CDK_XGSD_MIIM_BUS_1 or CDK_XGSD_MIIM_BUS_2 is set
 * then external MII bus #0 is used.
 *
 * Note 3:
 * External FE and Gigait PHYs usually reside on external bus #0
 * and use clause 22 access.
 *
 * Note 4:
 * External HiGig/XE PHYs usually reside on external bus #1 (or
 * #2 if supported) and use clause 45 access method.
 *
 * Note 5:
 * Second generation register layout replaces CDK_XGSD_MIIM_BUS_1,
 * CDK_XGSD_MIIM_BUS_2 and CDK_XGSD_MIIM_INTERNAL with a 3-bit
 * bus ID and a new bit for selecting between internal and
 * external MII buses.
 */
#define CDK_XGSD_MIIM_BUS_2      0x00000100 /* Select external MII bus #2 */
#define CDK_XGSD_MIIM_INTERNAL   0x00000080 /* Select internal SerDes MII bus */
#define CDK_XGSD_MIIM_BUS_1      0x00000040 /* Select external MII bus #1 */
#define CDK_XGSD_MIIM_CLAUSE45   0x00000020 /* Select clause 45 access method */
#define CDK_XGSD_MIIM_PHY_ID     0x0000001f /* PHY address in MII bus */
#define CDK_XGSD_MIIM_IBUS(_b)   (((_b) << 6) | 0x200)
#define CDK_XGSD_MIIM_EBUS(_b)   ((_b) << 6)
#define CDK_XGSD_MIIM_IBUS_NUM(_a)  ((_a & ~0x200) >> 6)

/*
 * Definitions for 'reg' parameter.
 *
 * Note 1:
 * For clause 22 access, the register address is 5 bits.
 *
 * Note 2:
 * For clause 45 access, the register address is 16 bits and
 * the device address is 5 bits.
 *
 * Note 3:
 * For internal SerDes registers, bits [23:8] are used for selecting
 * the block number for registers 0x10-0x1f. The block select value
 * will be written unmodified to register 0x1f.
 */
#define CDK_XGSD_MIIM_REG_ADDR   0x0000001f /* Clause 22 register address */
#define CDK_XGSD_MIIM_R45_ADDR   0x0000ffff /* Clause 45 register address */
#define CDK_XGSD_MIIM_DEV_ADDR   0x001f0000 /* Clause 45 device address */
#define CDK_XGSD_MIIM_BLK_ADDR   0x00ffff00 /* SerDes block mapping (reg 0x1f) */

/* Transform register address from software API to datasheet format */
#define CDK_XGSD_IBLK_TO_C45(_a) \
    (((_a) & 0xf) | (((_a) >> 8) & 0x7ff0) | (((_a) << 11) & 0x8000));

/* Transform register address from datasheet to software API format */
#define CDK_XGSD_C45_TO_IBLK(_a) \
    ((((_a) & 0x7ff0) << 8) | (((_a) & 0x8000) >> 11) | ((_a) & 0xf))

/* MII write access. */
extern int 
cdk_xgsd_miim_write(int unit, uint32_t phy_addr, uint32_t reg, uint32_t val); 

/* MII read access. */
extern int
cdk_xgsd_miim_read(int unit, uint32_t phy_addr, uint32_t reg, uint32_t *val); 

/* MII write access to internal SerDes PHY registers. */
extern int 
cdk_xgsd_miim_iblk_write(int unit, uint32_t phy_addr, uint32_t reg, uint32_t val); 

/* MII read access to internal SerDes PHY registers. */
extern int
cdk_xgsd_miim_iblk_read(int unit, uint32_t phy_addr, uint32_t reg, uint32_t *val); 

#endif /* __XGSD_MIIM_H__ */
