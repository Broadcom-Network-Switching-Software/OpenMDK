/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __XGS_TEST_INTERRUPT_H__
#define __XGS_TEST_INTERRUPT_H__

#include <bmd/bmd.h>

extern int bmd_xgs_test_interrupt(int unit, int do_assert); 

#define bmd_xgs_test_interrupt_assert(_u)  bmd_xgs_test_interrupt(_u, 1)
#define bmd_xgs_test_interrupt_clear(_u)   bmd_xgs_test_interrupt(_u, 0)

#endif /* __XGS_TEST_INTERRUPT_H__ */
