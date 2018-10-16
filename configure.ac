AC_INIT([fast-repo], [0.0.1], [peter@remap.ucla.edu], [fast-repo], [http://github.com/remap/fast-repo])
AM_INIT_AUTOMAKE([1.10 -Wall -Werror no-define foreign subdir-objects])
AC_CONFIG_MACRO_DIR([m4])
AC_PREREQ([2.69])

AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])

AC_PREFIX_DEFAULT([/usr/local])

AC_CONFIG_FILES([Makefile])

AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([src/config.hpp])


AC_PROG_CXX
AC_PROG_CC
AC_LANG([C++])

AX_CXX_COMPILE_STDCXX_11(, mandatory)
AC_CHECK_PROGS([TAR], [tar])

###############################
# Check for Boost
AX_BOOST_BASE([1.54])

AX_BOOST_SYSTEM
AX_BOOST_ASIO
AX_BOOST_THREAD
AX_BOOST_TIMER

###############################
# Check for RocksDB
AC_CHECK_HEADERS([rocksdb/db.h],,[AC_MSG_FAILURE([can't find RocksDB])])
AC_CHECK_LIB([rocksdb], [rocksdb_restore_options_create])

###############################
# Check for NDN-CPP
AC_CHECK_HEADERS([ndn-cpp/interest.hpp],,[AC_MSG_FAILURE([can't find interest.hpp])])
AC_CHECK_LIB([ndn-cpp], [ndn_getErrorString], [],
             [AC_MSG_FAILURE([can't find the ndn-cpp library])])

###############################
# Check for CNL
AC_CHECK_HEADERS([cnl-cpp/namespace.hpp],,[AC_MSG_FAILURE([can't find namespace.hpp])])
AC_MSG_CHECKING([for CNL-CPP library...])

SAVED_LIBS=$LIBS
LIBS="$LIBS -lcnl-cpp"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <cnl-cpp/namespace.hpp>],
        [cnl_cpp::Namespace n(ndn::Name("/test"))])],
        [AC_MSG_RESULT([yes])],
        [AC_MSG_FAILURE([can't find CNL-CPP lib])]
)
LIBS=$SAVED_LIBS

AC_OUTPUT

