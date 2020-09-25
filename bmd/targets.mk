# $Id$
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#
# BMD default targets.
#
# This file may be included from the application makefile as well.
#

BMD_DEFAULT_TARGETS = shell api pkgsrc shared

ifndef BMD_TARGETS
BMD_TARGETS = $(BMD_DEFAULT_TARGETS)
endif

BMD_LIBNAMES = $(addprefix libbmd,$(BMD_TARGETS))

ifndef BMD_LIBSUFFIX
BMD_LIBSUFFIX = a
endif

BMD_LIBS = $(addsuffix .$(BMD_LIBSUFFIX),$(BMD_LIBNAMES))
