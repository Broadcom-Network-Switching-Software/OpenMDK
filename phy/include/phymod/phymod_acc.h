/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __PHYMOD_ACC_H__
#define __PHYMOD_ACC_H__

#include <phymod/phymod_system.h>
#include <phymod/phymod.h>

/*
 * Occasionally the phymod_bus_t changes to support new features. This define
 * allows applications to write backward compatible PHY bus drivers.
 */
#define PHYMOD_BUS_VERSION              1

/*
 * Extended PHY bus operation.
 *
 * If a PHY bus driver supports extended capabilities this must be
 * indicated via the flags field.  The access driver will then provide
 * additional information embedded in the function parameters.
 *
 * Read parameter encodings:
 *
 *   addr:  bits [31:16] - address extension
 *          bits [15:0]  - register address
 *
 *   data:  bits [31:16] - unused
 *          bits [15:0]  - register value
 * 
 * Write parameter encodings:
 *
 *   addr:  bits [31:16] - address extension
 *          bits [15:0]  - register address
 *
 *   data:  bits [31:16] - write mask
 *          bits [15:0]  - register value
 *
 * The address extension is hardware-sepcific, but typically it
 * contains the clause 45 DEVAD and a XAUI lane encoding.
 *
 * A write mask of zero will write all bits, otherwise only the
 * register value bits for which the corresponsing mask bit it set
 * will be written.
 */
 
/* Raw (unadjusted) PHY address */
#define PHYMOD_ACC_CACHE_INVAL(pa_) \
    PHYMOD_ACC_CACHE(pa_) = ~0
#define PHYMOD_ACC_CACHE_SYNC(pa_,data_) \
    PHYMOD_ACC_CACHE(pa_) = (data_)
#define PHYMOD_ACC_CACHE_IS_DIRTY(pa_,data_) \
    ((PHYMOD_ACC_FLAGS(pa_) & PHYMOD_ACC_F_USE_CACHE) == 0 || \
      PHYMOD_ACC_CACHE(pa_) != (data_))
#define PHYMOD_ACC_CACHE_ENABLE(pa_) do { \
        PHYMOD_ACC_FLAGS(pa_) |= PHYMOD_ACC_F_USE_CACHE; \
        PHYMOD_ACC_CACHE_INVAL(pa_); \
    } while (0)

/* PHY bus address (MDIO PHYAD) */
#define PHYMOD_ACC_BUS_ADDR(pa_) PHYMOD_ACC_ADDR(pa_)

/* Ensures that phymod_access_t structure is properly initialized */
#define PHYMOD_ACC_CHECK(pa_) \
    do { \
        if (phymod_acc_check(pa_) < 0) return PHYMOD_E_INTERNAL; \
    } while (0)

/* Access to PHY bus with instance adjustment (addr_offset) */
#define PHYMOD_BUS_READ(pa_,r_,v_) phymod_bus_read(pa_,r_,v_)
#define PHYMOD_BUS_WRITE(pa_,r_,v_) phymod_bus_write(pa_,r_,v_)
#define PHYMOD_IS_WRITE_DISABLED(pa_,v_) phymod_is_write_disabled(pa_,v_)

extern int
phymod_acc_check(const phymod_access_t *pa);

extern int
phymod_bus_read(const phymod_access_t *pa, uint32_t reg, uint32_t *data);

extern int
phymod_bus_write(const phymod_access_t *pa, uint32_t reg, uint32_t data);

extern int
phymod_is_write_disabled(const phymod_access_t *pa, uint32_t *data);

#endif /* __PHYMOD_ACC_H__ */
