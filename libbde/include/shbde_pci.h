/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __SHBDE_PCI_H__
#define __SHBDE_PCI_H__

#include <shbde.h>

extern unsigned int
shbde_pci_pcie_cap(shbde_hal_t *shbde, void *pci_dev);

extern int
shbde_pci_is_pcie(shbde_hal_t *shbde, void *pci_dev);

extern int
shbde_pci_is_iproc(shbde_hal_t *shbde, void *pci_dev, int *cmic_bar);

extern int
shbde_pci_max_payload_set(shbde_hal_t *shbde, void *pci_dev, int maxpayload);

#endif /* __SHBDE_PCI_H__ */
