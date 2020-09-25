# $Id: ea7196a92e2c4166a104e21f896b30353634cc25 $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#

LOCALDIR = sym
LIBNAME = libphysym

ifndef PHY_ALLSYMS
PHY_ALLSYMS = 1
endif

ifeq ($(PHY_ALLSYMS),1)
# Allow compiler to merge duplicate strings
VSRCS = phy_allsyms.c
else
VSRCS = $(wildcard $(PHY)/$(LOCALDIR)/chip/*.c)
endif

LSRCS = $(notdir $(VSRCS))
LOBJS = $(addsuffix .$(OBJSUFFIX), $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))

include $(CDK)/make/lib.mk

