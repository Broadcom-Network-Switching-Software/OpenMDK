# $Id$
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#
# LIBBDE default targets.
#
# This file may be included from the application makefile as well.
#

LIBBDE_DEFAULT_TARGETS = shared

ifndef LIBBDE_TARGETS
LIBBDE_TARGETS = $(LIBBDE_DEFAULT_TARGETS)
endif

LIBBDE_LIBNAMES = $(addprefix libbde,$(LIBBDE_TARGETS))

ifndef LIBBDE_LIBSUFFIX
LIBBDE_LIBSUFFIX = a
endif

LIBBDE_LIBS = $(addsuffix .$(LIBBDE_LIBSUFFIX),$(LIBBDE_LIBNAMES))
