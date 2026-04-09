#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([Turbiine],
        [1.1-pre2+],
        [https://github.com/dkosmari/Turbiine/issues],
        [turbiine],
        [https://github.com/dkosmari/Turbiine])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AC_CANONICAL_HOST

DEVKITPRO_WUT_INIT

AM_INIT_AUTOMAKE([foreign subdir-objects])

LT_INIT([disable-shared])

AC_PROG_CXX
AC_LANG([C++])
AX_APPEND_COMPILE_FLAGS([-std=c++23], [CXX])

WIIU_ENV_SETUP_WUPS([external/libwupsxx/external/WiiUPluginSystem])
WIIU_ENV_SETUP_LIBBUTTONCOMBO([external/libwupsxx/external/libbuttoncombo])
WIIU_ENV_SETUP_LIBNOTIFICATIONS([external/libwupsxx/external/libnotifications])

AC_CONFIG_FILES([Makefile])

AX_SUBDIRS_CONFIGURE([external/libwupsxx],
                     [[--enable-notify],[--enable-shortcuts]])

AC_OUTPUT
