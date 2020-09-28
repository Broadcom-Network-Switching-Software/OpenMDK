/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <shbde_pci.h>

/* PCIe capabilities */
#ifndef PCI_CAPABILITY_LIST
#define PCI_CAPABILITY_LIST     0x34 
#endif
#ifndef PCI_CAP_ID_EXP
#define PCI_CAP_ID_EXP          0x10
#endif
#ifndef PCI_EXP_DEVCAP
#define PCI_EXP_DEVCAP          4
#endif
#ifndef PCI_EXP_DEVCTL
#define PCI_EXP_DEVCTL          8
#endif
#ifndef PCI_EXT_CAP_START
#define PCI_EXT_CAP_START       0x100
#endif
#ifndef PCI_EXT_CAP_ID
#define PCI_EXT_CAP_ID(_hdr)    (_hdr & 0x0000ffff)
#endif
#ifndef PCI_EXT_CAP_VER
#define PCI_EXT_CAP_VER(_hdr)   ((_hdr >> 16) & 0xf)
#endif
#ifndef PCI_EXT_CAP_NEXT
#define PCI_EXT_CAP_NEXT(_hdr)  ((_hdr >> 20) & 0xffc)
#endif
#ifndef PCI_EXT_CAP_ID_VNDR
#define PCI_EXT_CAP_ID_VNDR     0x0b
#endif

#define LOG_OUT(_shbde, _lvl, _str, _prm)             \
    if ((_shbde)->log_func) {                         \
        (_shbde)->log_func(_lvl, _str, _prm);         \
    }
#define LOG_ERR(_shbde, _str, _prm)     LOG_OUT(_shbde, SHBDE_ERR, _str, _prm)
#define LOG_WARN(_shbde, _str, _prm)    LOG_OUT(_shbde, SHBDE_WARN, _str, _prm)
#define LOG_DBG(_shbde, _str, _prm)     LOG_OUT(_shbde, SHBDE_DBG, _str, _prm)

/*
 * Warpper functions with null-pointer checks.
 */
static unsigned int
pcic16_read(shbde_hal_t *shbde, void *pci_dev,
            unsigned int addr)
{
    if (!shbde || !shbde->pcic16_read) {
        return 0;
    }
    return shbde->pcic16_read(pci_dev, addr);
}

static void
pcic16_write(shbde_hal_t *shbde, void *pci_dev,
             unsigned int addr, unsigned int data)
{
    if (!shbde || !shbde->pcic16_write) {
        return;
    }
    shbde->pcic16_write(pci_dev, addr, data);
}

static unsigned int
pcic32_read(shbde_hal_t *shbde, void *pci_dev,
            unsigned int addr)
{
    if (!shbde || !shbde->pcic32_read) {
        return 0;
    }
    return shbde->pcic32_read(pci_dev, addr);
}

/*
 * Function:
 *      shbde_pci_pcie_cap
 * Purpose:
 *      Return offset of PCIe capabilities in PCI configuration space
 * Parameters:
 *      shbde - pointer to initialized hardware abstraction module
 *      dev - PCI device handle (passed back to PCI HAL functions)
 * Returns:
 *      PCI_CAP_ID_EXP offset in PCI configuration space if PCIe, otherwise 0
 */
unsigned int
shbde_pci_pcie_cap(shbde_hal_t *shbde, void *pci_dev)
{
    unsigned int cap_base, rval;

    cap_base = pcic16_read(shbde, pci_dev, PCI_CAPABILITY_LIST);
    while (cap_base) {
        rval = pcic16_read(shbde, pci_dev, cap_base);
        if ((rval & 0xff) == PCI_CAP_ID_EXP) {
            break;
        }
        cap_base = (rval >> 8) & 0xff;
    }

    return cap_base;
}

/*
 * Function:
 *      shbde_pci_is_pcie
 * Purpose:
 *      Check if PCI device is PCIe device
 * Parameters:
 *      shbde - pointer to initialized hardware abstraction module
 *      dev - PCI device handle (passed back to PCI HAL functions)
 * Returns:
 *      1 if PCIe, otherwise 0
 */
int
shbde_pci_is_pcie(shbde_hal_t *shbde, void *pci_dev)
{
    return shbde_pci_pcie_cap(shbde, pci_dev) ? 1 : 0;
}

/*
 * Function:
 *      shbde_pci_is_iproc
 * Purpose:
 *      Check if PCI device is iProc-based
 * Parameters:
 *      shbde - pointer to initialized hardware abstraction module
 *      dev - PCI device handle (passed back to PCI HAL functions)
 *      cmic_bar - (OUT) PCI BAR which contains switch CMIC registers
 * Returns:
 *      1 if iProc-based, otherwise 0
 */
int
shbde_pci_is_iproc(shbde_hal_t *shbde, void *pci_dev, int *cmic_bar)
{
    unsigned int cap_base, rval;

    if (!shbde_pci_is_pcie(shbde, pci_dev)) {
        return 0;
    }

    /* Look for PCIe vendor-specific extended capability (VSEC) */
    cap_base = PCI_EXT_CAP_START;
    while (cap_base) {
        rval = pcic32_read(shbde, pci_dev, cap_base);
        if (rval == 0xffffffff) { 
           /* Assume PCI HW read error */ 
           return 0; 
        } 

        if (PCI_EXT_CAP_ID(rval) == PCI_EXT_CAP_ID_VNDR) {
            break;
        }
        cap_base = PCI_EXT_CAP_NEXT(rval);
    }
    if (cap_base) {
        /*
         * VSEC layout:
         *
         * 0x00: PCI Express Extended Capability Header
         * 0x04: Vendor-Specific Header
         * 0x08: Vendor-Specific Register 1
         * 0x0c: Vendor-Specific Register 2
         *     ...
         * 0x24: Vendor-Specific Register 8
         */
        rval = pcic32_read(shbde, pci_dev, cap_base + 8);
        LOG_DBG(shbde, "Found VSEC", rval);

        /* Determine PCI BAR of CMIC */
        *cmic_bar = 0;
        if ((rval == 0x101) || (rval == 0x100)) { 
            *cmic_bar = 2;
        }
        /* Assume iProc device */
        return 1;
    }

    return 0;
}

/*
 * Function:
 *      shbde_pci_max_payload_set
 * Purpose:
 *      Set PCIe maximum payload
 * Parameters:
 *      shbde - pointer to initialized hardware abstraction module
 *      dev - PCI device handle (passed back to PCI HAL functions)
 *      maxpayload - maximum payload (in byte)
 * Returns:
 *      -1 if error, otherwise 0
 * Notes:
 *      If not PCIe device, set the PCI retry count to infinte instead.
 */
int
shbde_pci_max_payload_set(shbde_hal_t *shbde, void *pci_dev, int maxpayload)
{
    unsigned int cap_base;
    unsigned int devcap, devctl;
    int max_val, max_cap;

    cap_base = shbde_pci_pcie_cap(shbde, pci_dev);

    if (cap_base == 0) {
        /* Not PCIe */
        return 0;
    }

    /* Get current device control settings */
    devctl = pcic16_read(shbde, pci_dev, cap_base + PCI_EXP_DEVCTL);

    /* Get current max payload setting */
    max_val = (devctl >> 5) & 0x7;

    if (maxpayload) {
        /* Get encoding from byte value */
        max_val = 0;
        while ((1 << (max_val + 7)) < maxpayload) {
            max_val++;
        }
        LOG_DBG(shbde, "Set max payload size", maxpayload);
        LOG_DBG(shbde, "Set max payload val", max_val);

        /* Get max supported payload size */
        devcap = pcic16_read(shbde, pci_dev, cap_base + PCI_EXP_DEVCAP);
        max_cap = (devcap & 0x7);

        /* Do not exceed device capabilities */
        if (max_val > max_cap) {
            max_val = max_cap;
            LOG_WARN(shbde,
                     "Payload size exceeds device capability",
                     maxpayload);
        }

        /* Update max payload size */
        devctl &= ~(0x7 << 5);
        devctl |= (max_val) << 5;

        /* Update max request size */
        devctl &= ~(0x7 << 12);
        devctl |= (max_val << 12);
    }

    /* Always disable relaxed ordering */
    devctl &= ~(1 << 4);

    /* Update device control settings */
    pcic16_write(shbde, pci_dev, cap_base + PCI_EXP_DEVCTL, devctl);

    /* Warn if non-default setting is used */
    if (max_val > 0) {
        LOG_WARN(shbde,
                 "Selected payload size may not be supported by all "
                 "PCIe bridges by default.",
                 (1 << (max_val + 7)));
    }

    return 0;
}
