/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * All Rights Reserved.$
 *
 * TDM core parsing functions
 */
#include "bcm56560_b0_tdm_core_top.h"


/**
@name: tdm_b0_abs
@param:

Returns absolute value of an integer
 */
int
tdm_b0_abs(int num)
{
    if (num < 0) {
        return (-num);
    }
    else {
        return num;
    }
}


/**
@name: tdm_b0_fac
@param:

Calculates factorial of an integer
 */
int
tdm_b0_fac(int num_signed)
{
    int i, product, num;
    num = tdm_b0_abs(num_signed);
    product=num;

    if (num==0) {
        return 1;
    }
    else {
        for (i=(num-1); i>0; i--) {
            product *= (num-i);
        }
        return product;
    }
}


/**
@name: tdm_b0_pow
@param:

Calculates unsigned power of an integer
 */
int
tdm_b0_pow(int num, int pow)
{
    int i, product=num;

    if (pow==0) {
        return 1;
    }
    else {
        for (i=1; i<pow; i++) {
            product *= num;
        }
        return product;
    }
}


/**
@name: tdm_b0_sqrt
@param:

Calculates approximate square root of an integer using Taylor series without float using Bakhshali Approximation
 */
int
tdm_b0_sqrt(int input_signed)
{
    int n, d=0, s=1, approx, input;
    input = tdm_b0_abs(input_signed);

    do {
        d=(input-tdm_b0_pow(s,2));
        if (d<0) {
            --s;
            break;
        }
        if ( ((1000*tdm_b0_abs(d))/tdm_b0_pow(s,2)) <= ((1000*tdm_b0_abs(d+1))/tdm_b0_pow((s+1),2)) ) {
            break;
        }
        s++;
    } while(s<input);
    d=(input-tdm_b0_pow(s,2));

    approx=s;
    if (d<(2*s)) {
        for (n=1; n<3; n++) {
            approx+=((tdm_b0_pow(-1,n)*tdm_b0_fac(2*n)*tdm_b0_pow(d,n))/(tdm_b0_pow(tdm_b0_fac(n),2)*tdm_b0_pow(4,n)*tdm_b0_pow(s,((2*n)-1))*(1-2*n)));
        }
    }

    return approx;
}


/**
@name: tdm_b0_PQ
@param:

Customizable partial quotient ceiling function
 */
/* int tdm_b0_PQ(float f) { return (int)((f < 0.0f) ? f - 0.5f : f + 0.5f); }*/
int tdm_b0_PQ(int f)
{
    return ( (int)( ( f+5 )/10) );
}


/**
@name: tdm_b0_math_div_ceil
@param:

Return (int)ceil(a/b)
 */
int tdm_b0_math_div_ceil(int a, int b)
{
    int result=0;

    if(a>0 && b>0){
        result = a/b + ((a%b)!=0);
    }

    return result;
}


/**
@name: tdm_b0_math_div_floor
@param:

Return (int)floor(a/b)
 */
int tdm_b0_math_div_floor(int a, int b)
{
    int result=0;

    if(a>0 && b>0){
        result = a/b;
    }

    return result;
}


/**
@name: tdm_b0_math_div_round_b0
@param:

Return (int)round(a/b)
 */
int tdm_b0_math_div_round_b0(int a, int b)
{
    int result=0;

    if(a>0 && b>0){
        result = (a*10+5)/(b*10);
    }

    return result;
}
