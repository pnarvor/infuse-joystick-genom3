#
# Copyright (c) 2016 LAAS/CNRS
# All rights reserved.
#
# Redistribution and use  in source  and binary  forms,  with or without
# modification, are permitted provided that the following conditions are
# met:
#
#   1. Redistributions of  source  code must retain the  above copyright
#      notice and this list of conditions.
#   2. Redistributions in binary form must reproduce the above copyright
#      notice and  this list of  conditions in the  documentation and/or
#      other materials provided with the distribution.
#
#                                      Anthony Mallet on Thu Apr 14 2016
#
lib_LTLIBRARIES = libjoystick_codels.la

libjoystick_codels_la_SOURCES=	joystick_c_types.h
libjoystick_codels_la_SOURCES+=	joystick_codels.c
libjoystick_codels_la_SOURCES+=	joystick_publish_codels.c

libjoystick_codels_la_CPPFLAGS=	$(requires_CFLAGS)
libjoystick_codels_la_LDFLAGS=	$(requires_LIBS)
libjoystick_codels_la_LDFLAGS+=	-release $(PACKAGE_VERSION)


# idl  mappings
BUILT_SOURCES=	joystick_c_types.h
CLEANFILES=	${BUILT_SOURCES}
joystick_c_types.h: ${top_srcdir}/joystick.gen
	${GENOM3}  mappings \
	  -MD -MF .deps/$@.d -MT $@ --signature -l c $< >$@

-include .deps/joystick_c_types.h.d
