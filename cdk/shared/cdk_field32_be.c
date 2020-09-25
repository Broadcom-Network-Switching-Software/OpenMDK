/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Utility functions used for extracting field values from 
 * registers and memories with big-endian word order.
 */

#include <cdk/cdk_types.h>
#include <cdk/cdk_assert.h>
#include <cdk/cdk_field.h>

/*
 * Function:
 *	cdk_field32_be_get
 * Purpose:
 *	Extract field value from multi-word register/memory.
 * Parameters:
 *	entbuf - current contents of register/memory (word array)
 *      wsize - size of entbuf counted in 32-bit words
 *      sbit - bit number of first bit of the field to extract
 *      ebit - bit number of last bit of the field to extract
 * Returns:
 *      Extracted field value.
 */
uint32_t
cdk_field32_be_get(const uint32_t *entbuf, int wsize,
                       int sbit, int ebit)
{
    uint32_t fval;

    cdk_field_be_get(entbuf, wsize, sbit, ebit, &fval);

    return fval;
}

/*
 * Function:
 *	cdk_field32_be_set
 * Purpose:
 *	Assign field value in multi-word register/memory.
 * Parameters:
 *	entbuf - current contents of register/memory (word array)
 *      wsize - size of entbuf counted in 32-bit words
 *      sbit - bit number of first bit of the field to extract
 *      ebit - bit number of last bit of the field to extract
 * Returns:
 *      Nothing.
 */
void
cdk_field32_be_set(uint32_t *entbuf, int wsize,
                       int sbit, int ebit, uint32_t fval)
{
    cdk_field_be_set(entbuf, wsize, sbit, ebit, &fval);
}
