# $Id$
# This software is governed by the Broadcom Switch APIs license.
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenMDK/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#
# PHY Library default targets.
#
# This file may be included from the application makefile as well.
#

PHY_DEFAULT_TARGETS = pkgsrc generic util sym

ifndef PHY_TARGETS
PHY_TARGETS = $(PHY_DEFAULT_TARGETS)
endif

PHY_LIBNAMES = $(addprefix libphy,$(PHY_TARGETS))

ifndef PHY_LIBSUFFIX
PHY_LIBSUFFIX = a
endif

PHY_LIBS = $(addsuffix .$(PHY_LIBSUFFIX),$(PHY_LIBNAMES))
