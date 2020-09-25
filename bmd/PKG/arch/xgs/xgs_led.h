/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGS DMA definitions.
 */

#ifndef __XGS_LED_H__
#define __XGS_LED_H__

#include <bmd/bmd.h>

/*
 * LED Microcontroller Registers
 */
#define CMIC_LED_CTRL                   0x00001000
#define CMIC_LED_STATUS                 0x00001004
#define CMIC_LED_PROGRAM_RAM_BASE       0x00001800
#define CMIC_LED_DATA_RAM_BASE          0x00001c00
#define CMIC_LED_PROGRAM_RAM(_a)        (CMIC_LED_PROGRAM_RAM_BASE + 4 * (_a))
#define CMIC_LED_PROGRAM_RAM_SIZE       0x100
#define CMIC_LED_DATA_RAM(_a)           (CMIC_LED_DATA_RAM_BASE + 4 * (_a))
#define CMIC_LED_DATA_RAM_SIZE          0x100

#define LC_LED_ENABLE                   0x1     /* Enable */

#define LS_LED_INIT                     0x200   /* Initializing */
#define LS_LED_RUN                      0x100   /* Running */
#define LS_LED_PC                       0xff    /* Current PC */

/* Flags for xgs_led_update */
#define XGS_LED_LINK                    0x1
#define XGS_LED_TURBO                   0x2

extern int
xgs_led_prog(int unit, uint8_t *program, int size);

int
xgs_led_update(int unit, int offset, uint32_t flags);


#endif /* __XGS_LED_H__ */
