# $Id: d13e44d2dacf3ce6bd1fd421a11ea379e3fff30c $ Broadcom SDK $
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#
# LIBBDE top-level makefile
#

# CDK variable is required
ifndef CDK
$(warning setting $$CDK automatically)
export CDK=${MDK}/cdk
endif

# Set default LIBBDE directory
ifndef LIBBDE
LIBBDE = $(CURDIR)
export LIBBDE
endif

# Default build directory
ifndef LIBBDE_BLDDIR
LIBBDE_BLDDIR = $(LIBBDE)/build
endif

# Replace original CDK_BLDDIR with updated version
override MAKEOVERRIDES := $(subst CDK_BLDDIR,_BLDDIR,$(MAKEOVERRIDES))
override CDK_BLDDIR = $(LIBBDE_BLDDIR)
export CDK_BLDDIR

# Default include path
LIBBDE_INCLUDE_PATH = -I$(LIBBDE)/include

# Import preprocessor flags avoiding include duplicates
TMP_LIBBDE_CPPFLAGS := $(filter-out $(LIBBDE_INCLUDE_PATH),$(LIBBDE_CPPFLAGS))
TMP_LIBBDE_CPPFLAGS += $(LIBBDE_INCLUDE_PATH)

# Import compiler flags
TMP_LIBBDE_CFLAGS := $(LIBBDE_CFLAGS)

include $(CDK)/make/config.mk

# Replace original CDK_CPPFLAGS with updated version
override MAKEOVERRIDES := $(subst CDK_CPPFLAGS,_CPPFLAGS,$(MAKEOVERRIDES))
override CDK_CPPFLAGS := $(TMP_LIBBDE_CPPFLAGS)
export CDK_CPPFLAGS

# Replace original CDK_CFLAGS with updated version
override MAKEOVERRIDES := $(subst CDK_CFLAGS,_CFLAGS,$(MAKEOVERRIDES))
override CDK_CFLAGS := $(TMP_LIBBDE_CFLAGS)
export CDK_CFLAGS

include $(LIBBDE)/targets.mk

.PHONY: all clean $(LIBBDE_TARGETS)

ifeq ($(MAKECMDGOALS),clean)
LIBBDE_CLEAN = clean
endif

all:
	$(MAKE) bdelibs

bdelibs: $(LIBBDE_TARGETS)

$(LIBBDE_TARGETS):
	@${ECHO} Processing LIBBDE subdirectory $@
	$(MAKE) -C $@ $(LIBBDE_CLEAN)

clean:: $(LIBBDE_TARGETS)

