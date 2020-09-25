/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <cdk/cdk_shell.h>

static int
_xdigit2i(int digit)
{
    if (digit >= '0' && digit <= '9') return (digit - '0'     );
    if (digit >= 'a' && digit <= 'f') return (digit - 'a' + 10);
    if (digit >= 'A' && digit <= 'F') return (digit - 'A' + 10);
    return -1;
}

/*
 * Function:
 *	cdk_shell_parse_is_int
 * Purpose:
 *	Check if string contains a valid integer
 * Parameters:
 *	s - string to parse
 * Returns:
 *      TRUE if string is a valid integer, otherwise FALSE
 */
#include <cdk/cdk_printf.h>
int 
cdk_shell_parse_is_int(const char *s)
{
    int base;

    if (s == NULL || *s == 0) {
        return 0;
    }

    if (*s == '-') {
        s++;
    }

    if (*s == '0') {
        if (s[1] == 'b' || s[1] == 'B') {
            base = 2;
            s += 2;
        } else if (s[1] == 'x' || s[1] == 'X') {
            base = 16;
            s += 2;
        } else
            base = 8;
    } else {
        base = 10;
    }

    do {
        if ((_xdigit2i(*s) >= base) || (_xdigit2i(*s) < 0)) {
            return(0);
        }
    } while (*++s);

    return 1; 
}
