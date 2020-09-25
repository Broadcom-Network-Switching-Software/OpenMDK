/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * XGSM LED controller definitions.
 */

#ifndef __XGSM_LED_H__
#define __XGSM_LED_H__

#include <bmd/bmd.h>

/*
 * LED Microcontroller Registers (LEDUP0)
 */
#define CMIC_LED_BASE                   0x00020000
#define CMIC_LED_CTRL                   (CMIC_LED_BASE + 0)
#define CMIC_LED_STATUS                 (CMIC_LED_BASE + 4)
#define CMIC_LED_PROGRAM_RAM_BASE       (CMIC_LED_BASE + 0x800)
#define CMIC_LED_DATA_RAM_BASE          (CMIC_LED_BASE + 0x400)
#define CMIC_LED_PROGRAM_RAM(_a)        (CMIC_LED_PROGRAM_RAM_BASE + 4 * (_a))
#define CMIC_LED_PROGRAM_RAM_SIZE       0x100
#define CMIC_LED_DATA_RAM(_a)           (CMIC_LED_DATA_RAM_BASE + 4 * (_a))
#define CMIC_LED_DATA_RAM_SIZE          0x100

#define LC_LED_ENABLE                   0x1     /* Enable */

#define LS_LED_INIT                     0x200   /* Initializing */
#define LS_LED_RUN                      0x100   /* Running */
#define LS_LED_PC                       0xff    /* Current PC */

/* Flags for xgsm_led_update */
#define XGSM_LED_LINK                    0x1
#define XGSM_LED_TURBO                   0x2

extern int
xgsm_led_prog(int unit, uint8_t *program, int size);

int
xgsm_led_update(int unit, int offset, uint32_t flags);


#endif /* __XGSM_LED_H__ */
