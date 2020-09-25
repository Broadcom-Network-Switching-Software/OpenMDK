# $Id: 6683dbf52722b4f638f30d86358446adca5f8aa3 $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#

LOCALDIR = sym
LIBNAME = libcdksym

ifndef CDK_MAKE_ALLSYMS
CDK_MAKE_ALLSYMS = 1
endif

ifeq ($(CDK_MAKE_ALLSYMS),1)

# Allow compiler to merge duplicate strings
VSRCS = cdk_allsyms.c

else

# Compiler individual symbol files
VSRCS = $(wildcard chip/*.c); 
VPATH += $(CDK)/$(LOCALDIR)/chip

endif

# Add local/static sym files
VSRCS += $(wildcard $(CDK)/$(LOCALDIR)/*sym.c)

LSRCS = $(notdir $(VSRCS))
LOBJS = $(addsuffix .$(OBJSUFFIX), $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))

include $(CDK)/make/lib.mk

