/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * CDK readline implmentation.
 */

#ifndef __CDK_READLINE_H__
#define __CDK_READLINE_H__

/* Simple readline function with basic edit and history */
extern char *
cdk_readline(int (*getchar_func)(void), int (*putchar_func)(int),
             const char *prompt, char *str, int maxlen);

#endif /* __CDK_READLINE_H__ */
