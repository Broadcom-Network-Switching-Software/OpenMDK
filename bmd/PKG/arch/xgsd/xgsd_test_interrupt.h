/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __XGSD_TEST_INTERRUPT_H__
#define __XGSD_TEST_INTERRUPT_H__

#include <bmd/bmd.h>

extern int bmd_xgsd_test_interrupt(int unit, int do_assert); 

#define bmd_xgsd_test_interrupt_assert(_u)  bmd_xgsd_test_interrupt(_u, 1)
#define bmd_xgsd_test_interrupt_clear(_u)   bmd_xgsd_test_interrupt(_u, 0)

#endif /* __XGSD_TEST_INTERRUPT_H__ */
