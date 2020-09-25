/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK abort handler.
 */

#include <cdk/cdk_types.h>
#include <cdk/cdk_printf.h>
#include <cdk/cdk_stdlib.h>

/*
 * Function:
 *	cdk_abort
 * Purpose:
 *	Default program abort routine.
 * Returns:
 *      Nothing.
 */
void
cdk_abort(void)
{
    CDK_PRINTF("\n*** Program aborted ***\n\n");
    CDK_PRINTF("It is recommended that you install your own abort handler.\n");
    CDK_PRINTF("Press Ctrl-C to exit the program completely.\n");

    do {/* loop forever */; } while (TRUE);
}
