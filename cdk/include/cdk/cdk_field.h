/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

/* Little endian word order (normal) */
extern uint32_t *cdk_field_get(const uint32_t *entbuf, 
                               int sbit, int ebit, uint32_t *fbuf);
extern void cdk_field_set(uint32_t *entbuf, 
                          int sbit, int ebit, uint32_t *fbuf);
extern uint32_t cdk_field32_get(const uint32_t *entbuf, 
                                int sbit, int ebit);
extern void cdk_field32_set(uint32_t *entbuf, 
                            int sbit, int ebit, uint32_t fval);

/* Big endian word order (some exceptions) */
extern uint32_t *cdk_field_be_get(const uint32_t *entbuf, int wsize,
                                  int sbit, int ebit, uint32_t *fbuf);
extern void cdk_field_be_set(uint32_t *entbuf, int wsize,
                             int sbit, int ebit, uint32_t *fbuf);
extern uint32_t cdk_field32_be_get(const uint32_t *entbuf, int wsize,
                                   int sbit, int ebit);
extern void cdk_field32_be_set(uint32_t *entbuf, int wsize,
                               int sbit, int ebit, uint32_t fval);
