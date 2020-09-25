/*
 * This software is governed by the Broadcom Switch APIs license.
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Shell symbol flag utilities.
 */

#include <cdk/cdk_shell.h>
#include <cdk/cdk_string.h>

typedef struct sym_flags_s {
    const char *name; 
    uint32_t value; 
} sym_flags_map_t; 


static sym_flags_map_t _sym_flags_table[] = 
    {   
        {"register",    CDK_SYMBOL_FLAG_REGISTER    }, 
        {"port",        CDK_SYMBOL_FLAG_PORT        }, 
        {"counter",     CDK_SYMBOL_FLAG_COUNTER     }, 
        {"memory",      CDK_SYMBOL_FLAG_MEMORY      }, 
        {"r64",         CDK_SYMBOL_FLAG_R64         }, 
	{"big-endian",  CDK_SYMBOL_FLAG_BIG_ENDIAN  },  
        {"soft-port",   CDK_SYMBOL_FLAG_SOFT_PORT   },
        {"memmapped",   CDK_SYMBOL_FLAG_MEMMAPPED   },
	{NULL, 0                                    }
    }; 


const char *
cdk_shell_symflag_type2name(uint32_t flag)
{
    sym_flags_map_t *s = _sym_flags_table; 
    for (; s->name; s++) {
        if (s->value == flag) {
            return s->name; 
        }
    }
    return NULL;
}

uint32_t 
cdk_shell_symflag_name2type(const char *name)
{
    sym_flags_map_t *sft = _sym_flags_table; 

    for (; sft->name; sft++) {
        if (CDK_STRCMP(sft->name, name) == 0) {
            return sft->value; 
        }
    }
    return 0;
}
