/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS interupt test functions.
 */

#include <bmd/bmd.h>

#ifdef CDK_CONFIG_ARCH_XGS_INSTALLED

#include <bmdi/arch/xgs_test_interrupt.h>

#include <cdk/arch/xgs_cmic.h>

int
bmd_xgs_test_interrupt(int unit, int do_assert)
{
    int ioerr = 0;
    CMIC_CONFIGr_t cmic_config;

    BMD_CHECK_UNIT(unit);

    ioerr = READ_CMIC_CONFIGr(unit, &cmic_config);
    CMIC_CONFIGr_ACT_LOW_INTf_SET(cmic_config, do_assert ? 0 : 1);
    ioerr = WRITE_CMIC_CONFIGr(unit, cmic_config);

    return ioerr ? CDK_E_IO : CDK_E_NONE; 
}

#endif /* CDK_CONFIG_ARCH_XGS_INSTALLED */
