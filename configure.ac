dnl Autoconf file for building joystick codels library.
dnl
dnl Copyright (c) 2016 LAAS/CNRS
dnl All rights reserved.
dnl
dnl Redistribution and use  in source  and binary  forms,  with or without
dnl modification, are permitted provided that the following conditions are
dnl met:
dnl
dnl   1. Redistributions of  source  code must retain the  above copyright
dnl      notice and this list of conditions.
dnl   2. Redistributions in binary form must reproduce the above copyright
dnl      notice and  this list of  conditions in the  documentation and/or
dnl      other materials provided with the distribution.
dnl
dnl                                      Anthony Mallet on Thu Apr 14 2016
dnl

AC_PREREQ(2.59)

AC_INIT([joystick-genom3],[1.0],[openrobots@laas.fr])
AC_CONFIG_MACRO_DIR([autoconf])
AC_CONFIG_AUX_DIR([autoconf])
AC_CONFIG_HEADERS([autoconf/acjoystick.h])
AM_INIT_AUTOMAKE([foreign no-define])

dnl Compilers
dnl
LT_INIT
AC_PROG_CC


dnl External packages
PKG_CHECK_MODULES(requires, [
  genom3 >= 2.99.4
  openrobots-idl >= 1.2
  sdl2
])

AC_PATH_PROG(GENOM3, [genom3], AC_MSG_ERROR([genom3 tool not found], 2))


dnl --with-templates option
AG_OPT_TEMPLATES([$GENOM3 ],
    [$srcdir/joystick.gen])


dnl Output
AC_CONFIG_FILES([
	joystick-genom3.pc
	joystick-genom3-uninstalled.pc
	Makefile
	codels/Makefile
])
AC_OUTPUT
AG_OUTPUT_TEMPLATES
