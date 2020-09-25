/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <phy/phy.h>

#define PHY_PORT_MAX    99

/*
 * Function:     
 *      phy_ctrl_change_inst
 * Purpose:    
 *      Modify phy_ctrl_t structure to allow access to another
 *      PHY instance within the same package.
 * Parameters:
 *      pc - PHY control structure
 *      new_inst - PHY instance of modified structure
 *      get_inst = optional function to supply default instance
 * Returns:    
 *      CDK_E_xxx.
 */
int
phy_ctrl_change_inst(phy_ctrl_t *pc, int new_inst, int (*get_inst)(phy_ctrl_t *))
{
    int inst;

    if (get_inst != NULL) {
        inst = get_inst(pc);
    } else {
        inst = PHY_CTRL_PHY_INST(pc);
    }
    if (inst < 0) {
        return CDK_E_NOT_FOUND;
    }

    pc->addr_offset = new_inst - inst;

    return CDK_E_NONE;
}
