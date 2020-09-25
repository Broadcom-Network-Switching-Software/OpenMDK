/*
 *         
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <phymod/phymod.h>
#include "falcon_dpll.h"

/* NEED TO REVISIT: need to update these settings */
int falcon_dpll_phy_media_type_tx_get(const phymod_phy_access_t* phy, phymod_media_typed_t media, phymod_tx_t* tx)
{        
    switch (media) {
    case phymodMediaTypeChipToChip:
      tx->pre   = 0xc;
      tx->main  = 0x66;
      tx->post  = 0x0;
      tx->post2 = 0x0;
      tx->post3 = 0x0;
      tx->amp   = 0xc;
      break;
    case phymodMediaTypeShort:
      tx->pre   = 0xc;
      tx->main  = 0x66;
      tx->post  = 0x0;
      tx->post2 = 0x0;
      tx->post3 = 0x0;
      tx->amp   = 0xc;
      break;
    case phymodMediaTypeMid:
      tx->pre   = 0xc;
      tx->main  = 0x66;
      tx->post  = 0x0;
      tx->post2 = 0x0;
      tx->post3 = 0x0;
      tx->amp   = 0xc;
      break;
    case phymodMediaTypeLong:
      tx->pre   = 0xc;
      tx->main  = 0x66;
      tx->post  = 0x0;
      tx->post2 = 0x0;
      tx->post3 = 0x0;
      tx->amp   = 0xc;
      break;
    default:
      tx->pre   = 0xc;
      tx->main  = 0x66;
      tx->post  = 0x0;
      tx->post2 = 0x0;
      tx->post3 = 0x0;
      tx->amp   = 0xc;
      break;
    }

    return PHYMOD_E_NONE;
}

