AC_INIT([fast-repo], [0.0.1], [peter@remap.ucla.edu], [fast-repo], [http://github.com/remap/fast-repo])
AM_INIT_AUTOMAKE([1.10 -Wall no-define foreign subdir-objects])
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
AX_BOOST_FILESYSTEM
AX_BOOST_LOG

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
SAVED_LIBS=$LIBS
LIBS="$LIBS -lcnl-cpp"

AC_CHECK_HEADERS([cnl-cpp/namespace.hpp],,[AC_MSG_WARN([can't find namespace.hpp])])
AC_MSG_CHECKING([for CNL-CPP library...])

AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([
					#include <cnl-cpp/namespace.hpp>
				],[
					cnl_cpp::Namespace n(ndn::Name("/test"))
		])],[
			AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE_CNL_CPP], [], [Have CNL-CPP library])
			AM_CONDITIONAL([HAVE_CNL_CPP], [test "true" = true])
		],[
			AC_MSG_WARN([can't find CNL-CPP lib])
			LIBS=$SAVED_LIBS
		])
LIBS=$SAVED_LIBS

###############################
# Check for protobuf
AC_MSG_CHECKING([for Protobuf])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#include <google/protobuf/message.h>]]
    [[google::protobuf::Message *message;]])
], [
    AC_MSG_RESULT([yes])
    HAVE_PROTOBUF=1
], [
    AC_MSG_RESULT([no])
    HAVE_PROTOBUF=0
])
AC_DEFINE_UNQUOTED([HAVE_PROTOBUF], $HAVE_PROTOBUF, [1 if have Google Protobuf.])
if test "$HAVE_PROTOBUF" == "1" ; then
  AC_CHECK_LIB([protobuf], [main], [],
               [AC_MSG_FAILURE([have protobuf headers but no libprotobuf])])
  # libprotobuf needs libpthread.
  AC_CHECK_LIB([pthread], [main], [],
               [AC_MSG_FAILURE([have libprotobuf but no libpthread])])

  # See https://stackoverflow.com/questions/13939904/how-to-use-protocol-buffers-with-autoconf-automake
  AC_ARG_WITH([protoc],
      [AS_HELP_STRING([--with-protoc=/path/of/protoc],
          [Location of the protocol buffers compiler protoc. Defaults to looking on path.])],
      [PROTOC="$withval"],
      [ AS_IF([test "x${PROTOC}" == "x"],
          [AC_PATH_PROG([PROTOC], [protoc], [no])])
      ]
  )
  AS_IF([test "${PROTOC}" == "no"], [AC_MSG_ERROR([ProtoBuf compiler "protoc" not found.])])
fi

###############################
# Check for NDN-RTC
AC_CHECK_HEADERS([ndnrtc/interfaces.hpp],,[AC_MSG_FAILURE([can't find NDN-RTC])])
AC_CHECK_LIB([ndnrtc], [ndnrtc_getVersion])

AC_OUTPUT
