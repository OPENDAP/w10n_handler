# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.69)
AC_INIT(w10n_handler, 1.0.0, opendap-tech@opendap.org)
AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([conf])

AM_INIT_AUTOMAKE([foreign subdir-objects check-news])

AM_CONDITIONAL([DAP_MODULES], [false])

# The handler is a shared object library, so we don't normally need
# the static lib. Override this default by specifying `--enable-static',
# which can make running unit tests with valgrind far more pleasant. 
AM_DISABLE_STATIC

LT_INIT

PACKAGE_MAJOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\1@'`
PACKAGE_MINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\2@'`
PACKAGE_SUBMINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\3@'`
AC_MSG_NOTICE(Package Major Version:     $PACKAGE_MAJOR_VERSION)
AC_MSG_NOTICE(Package Minor Version:     $PACKAGE_MINOR_VERSION)
AC_MSG_NOTICE(Package SubMinor Version:  $PACKAGE_SUBMINOR_VERSION)
AC_SUBST(PACKAGE_MAJOR_VERSION)
AC_SUBST(PACKAGE_MINOR_VERSION)
AC_SUBST(PACKAGE_SUBMINOR_VERSION)

AC_CONFIG_SRCDIR([w10n_utils.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_CHECK_FUNCS([strchr])

# Support for large files?
AC_SYS_LARGEFILE

dnl Checks for specific libraries
AC_CHECK_LIBDAP([3.13.0],
 [
  LIBS="$LIBS $DAP_SERVER_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])
 

AC_CHECK_BES([3.13.0],
[
],
[ AC_MSG_ERROR([Cannot find bes])
])

# Is there CPPUNIT?
AM_PATH_CPPUNIT(1.12.0,
	[AM_CONDITIONAL([CPPUNIT], [true])],
	[AM_CONDITIONAL([CPPUNIT], [false])])


AC_MSG_NOTICE([NC_LDFLAGS is $NC_LDFLAGS])
NC_BIN=`echo $NC_LDFLAGS | sed 's@^-L\(.*\)/lib@\1/bin@g'`
AC_MSG_NOTICE([NC_BIN is $NC_BIN])
AC_SUBST(NC_BIN_PATH, $NC_BIN)

## This sets up the special symbols used to build the bes.conf file for
## the tests. Previously, this was done in the Makefile.am down in the
## test directory, but building the bes.conf here works better with
## autoconf overall and esp. with the 'modules' build. jhrg 11/19/14
modulesdir=$BES_MODULE_DIR
AC_SUBST(modulesdir)

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([unit-tests/Makefile unit-tests/test_config.h])
AC_CONFIG_FILES([bes-testsuite/Makefile bes-testsuite/atlocal])
AC_CONFIG_FILES([bes-testsuite/bes.conf:bes-testsuite/bes.conf.in])

AC_CONFIG_TESTDIR(bes-testsuite)

AC_OUTPUT
