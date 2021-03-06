# Versioning.
m4_define([dc_version_major],[0])
m4_define([dc_version_minor],[0])
m4_define([dc_version_micro],[1])
m4_define([dc_version_suffix],[devel])
m4_define([dc_version],dc_version_major.dc_version_minor.dc_version_micro[]m4_ifset([dc_version_suffix],-[dc_version_suffix])) 
# Libtool versioning.
m4_define([dc_version_lt_current],[0])
m4_define([dc_version_lt_revision],[0])
m4_define([dc_version_lt_age],[0])

# Initialize autoconf
AC_PREREQ([2.60])
AC_INIT([domotica-engine],[dc_version])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_PREFIX_DEFAULT([/usr])
# AC_PROG_RANLIB

# Initialize automake.
AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability foreign])

# Fix for automake >= 1.12
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Initialize libtool.
LT_PREREQ([2.2.0])
LT_INIT()
LT_PROG_RC

# Tests
AC_ARG_ENABLE([tests],
	[AS_HELP_STRING([--enable-tests=@<:@yes/no@:>@],
		[Build self tests @<:@default=no@:>@])],
	[], [enable_tests=no])
AM_CONDITIONAL([ENABLE_TESTS], [test "x$enable_tests" = "xyes"])

AX_VALGRIND_CHECK

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99

# Check libs
AC_HAVE_LIBRARY(eu, ,AC_MSG_ERROR([libeu is required for building domotica-engine]))
AC_HAVE_LIBRARY(sqlite3, , AC_MSG_ERROR([libvsb is required for --enable-mod-eltako]))

# gcov coverage reporting

m4_include([m4/gcov.m4])
AC_TDD_GCOV
AC_SUBST(COVERAGE_CFLAGS)
AC_SUBST(COVERAGE_CXXFLAGS)
AC_SUBST(COVERAGE_LDFLAGS)

# Versioning.
AC_SUBST([DC_VERSION],[dc_version])
AC_SUBST([DC_VERSION_MAJOR],[dc_version_major])
AC_SUBST([DC_VERSION_MINOR],[dc_version_minor])
AC_SUBST([DC_VERSION_MICRO],[dc_version_micro])
AC_SUBST([DC_VERSION_LIBTOOL],[dc_version_lt_current:dc_version_lt_revision:dc_version_lt_age])

# Version suffix.
m4_ifset([dc_version_suffix],[
        AC_DEFINE(HAVE_VERSION_SUFFIX, [1], [Define if a version suffix is p    resent.])
])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	tests/Makefile
])

# Modules
AC_ARG_ENABLE(mod_soltrack,
AS_HELP_STRING([--enable-mod-soltrack],
               [Enable solar track module (needs libSolTrack)]),
[use_mod_soltrack=yes], [use_mod_soltrack=no])
AM_CONDITIONAL([HAVE_MOD_SOLTRACK], test "x$use_mod_soltrack" = "xyes")

if test "x$use_mod_soltrack" = "xyes"; then
	AC_DEFINE(ENABLE_MOD_SOLTRACK, 1, "Enable module soltrack")
	AC_HAVE_LIBRARY(SolTrack, LIBM="$LIBM -lm", AC_MSG_ERROR([libSolTrack is required for --enable-mod-soltrack]), -lm)
fi

AC_ARG_ENABLE(mod_eltako,
AS_HELP_STRING([--enable-mod-eltako],
               [Enable eltako module (needs libEltako)]),
[use_mod_eltako=yes], [use_mod_eltako=no])
AM_CONDITIONAL([HAVE_MOD_ELTAKO], test "x$use_mod_eltako" = "xyes")

if test "x$use_mod_eltako" = "xyes"; then
	AC_DEFINE(ENABLE_MOD_ELTAKO, 1, "Enable module eltako")
	AC_HAVE_LIBRARY(eltako, , AC_MSG_ERROR([libeltako is required for --enable-mod-eltako]))
	AC_HAVE_LIBRARY(vsb, , AC_MSG_ERROR([libvsb is required for --enable-mod-eltako]))
fi

AC_ARG_ENABLE(mod_musiccast,
AS_HELP_STRING([--enable-mod-musiccast],
               [Enable MusicCast module (Yamaha) (needs libCurl)]),
[use_mod_musiccast=yes], [use_mod_musiccast=no])
AM_CONDITIONAL([HAVE_MOD_MUSICCAST], test "x$use_mod_musiccast" = "xyes")

if test "x$use_mod_musiccast" = "xyes"; then
	AC_DEFINE(ENABLE_MOD_MUSICCAST, 1, "Enable module MusicCast")
	AC_HAVE_LIBRARY(curl, , AC_MSG_ERROR([libcurl is required for --enable-mod-musiccast]))
fi

AC_OUTPUT
