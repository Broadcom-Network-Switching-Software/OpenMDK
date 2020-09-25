/*
 *         
 * 
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <phymod/phymod.h>
#include <phymod/phymod_acc.h>

int phymod_osr_mode_to_actual_os(phymod_osr_mode_t osr_mode, uint32_t* os_int, uint32_t* os_remainder)
{
    if(PHYMOD_E_OK != phymod_osr_mode_t_validate(osr_mode)) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("osr_mode validation failed")));
    }

    if(os_int == NULL) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("os_int NULL parameter")));
    }
    if(os_remainder == NULL) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("os_remainder NULL parameter")));
    }

    *os_int = 0;
    *os_remainder = 0;

    switch (osr_mode) {
        case phymodOversampleMode1: 
            *os_int = 1;
            break;
        case phymodOversampleMode2: 
            *os_int = 2; 
            break;
        case phymodOversampleMode3: 
            *os_int = 3; 
            break;
        case phymodOversampleMode3P3: 
            *os_int = 3;
            *os_remainder = 300;
            break;
        case phymodOversampleMode4: 
            *os_int = 4; 
            break;
        case phymodOversampleMode5: 
            *os_int = 5; 
            break;
        case phymodOversampleMode7P5: 
            *os_int = 7;
            *os_remainder = 500;
            break;
        case phymodOversampleMode8: 
            *os_int = 8; 
            break;
        case phymodOversampleMode8P25:
            *os_int = 8;
            *os_remainder = 250;
            break;
        case phymodOversampleMode10: 
            *os_int = 10; 
            break;
        case phymodOversampleMode16P5:
            *os_int = 16;
            *os_remainder = 500;
            break;
        case phymodOversampleMode20P625:
            *os_int = 20;
            *os_remainder = 625;
            break;
        default:
            PHYMOD_RETURN_WITH_ERR(PHYMOD_E_INTERNAL, (_PHYMOD_MSG("OS mode not supported")));
    }
         
    return PHYMOD_E_NONE;
}

int phymod_osr_mode_t_validate(phymod_osr_mode_t phymod_osr_mode)
{
        
    if(phymod_osr_mode >= phymodOversampleModeCount) {
        PHYMOD_RETURN_WITH_ERR(PHYMOD_E_PARAM, (_PHYMOD_MSG("Parameter is out of range")));
    }
        
    return PHYMOD_E_NONE;
    
}





