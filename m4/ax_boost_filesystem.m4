##### http://autoconf-archive.cryp.to/ax_boost_regex.html
# ATTENTION: original file modified for PET by Peter Adolphs
#
# SYNOPSIS
#
#   AX_BOOST_FILESYSTEM([action-if-found], [action-if-not-found])
#
# DESCRIPTION
#
#   Test for Program.Options library from the Boost C++ libraries. The macro
#   requires a preceding call to AX_BOOST_BASE. Further documentation
#   is available at <http://randspringer.de/boost/index.html>.
#
#   This macro calls:
#
#     AC_SUBST(BOOST_FILESYSTEM_LIBS)
#
#   And sets:
#
#     HAVE_BOOST_FILESYSTEM
#
# LAST MODIFICATION
#
#   2010-01-28 by Peter Adolphs
#              - test alternative library names
#   2008-11-03 by Peter Adolphs
#              - simplified header and library checks using standard
#                autoconf macros
#   2008-02-25 by Peter Adolphs
#              - fixed broken indentation (tabs/spaces)
#              - added ACTION-IF-FOUND and ACTION-IF-NOT-FOUND
#              - notice instead of error when boost was not found
#              - changed help message
#
# COPYLEFT
#
#   Copyright (c) 2007 Thomas Porschberg <thomas@randspringer.de>
#   Copyright (c) 2007 Michael Tindal
#   Copyright (c) 2008 Peter Adolphs
#
#   Copying and distribution of this file, with or without
#   modification, are permitted in any medium without royalty provided
#   the copyright notice and this notice are preserved.

AC_DEFUN([AX_BOOST_FILESYSTEM],
[
  ax_boost_filesystem_default_libnames="boost_filesystem boost_filesystem-mt"
  AC_ARG_WITH([boost-filesystem],
    [ AS_HELP_STRING(
        [--with-boost-filesystem@<:@=ARG@:>@],
        [use standard Boost.Filesystem library (ARG=yes),
         use specific Boost.Filesystem library (ARG=<name>, e.g. ARG=boost_filesystem-gcc-mt-d-1_33_1),
         or disable it (ARG=no)
         @<:@ARG=yes@:>@ ]) ],
    [
      if test "$withval" = "no"; then
        ax_boost_filesystem_wanted="no"
      elif test "$withval" = "yes"; then
        ax_boost_filesystem_wanted="yes"
        ax_boost_filesystem_libnames="${ax_boost_filesystem_default_libnames}"
      else
        ax_boost_filesystem_wanted="yes"
        ax_boost_filesystem_libnames="$withval"
      fi
    ],
    [ax_boost_filesystem_wanted="yes"
     ax_boost_filesystem_libnames="${ax_boost_filesystem_default_libnames}"]
  )
  
  if test "x$ax_boost_filesystem_wanted" = "xyes"; then
    AC_REQUIRE([AX_BOOST_BASE])
    CPPFLAGS_SAVED="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
    export CPPFLAGS
    
    LDFLAGS_SAVED="$LDFLAGS"
    LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
    export LDFLAGS
    
    AC_LANG_PUSH([C++])
    AC_CHECK_HEADER([boost/filesystem.hpp],
                    [ax_boost_filesystem_headers=yes],
                    [ax_boost_filesystem_headers=no
                     AC_MSG_WARN([Could not find header files for Boost.Filesystem])])
    if test "x${ax_boost_filesystem_headers}" = "xyes"; then
      for ax_boost_filesystem_libname in ${ax_boost_filesystem_libnames} ; do
        AC_CHECK_LIB([$ax_boost_filesystem_libname], [main],
                     [ax_boost_filesystem_links=yes
                      BOOST_FILESYSTEM_LIBS=-l${ax_boost_filesystem_libname}
                      break
                     ],
                     [ax_boost_filesystem_links=no])
      done
      test $ax_boost_filesystem_links == "no" && AC_MSG_WARN([Could not link Boost.Filesystem])
    fi
    AC_LANG_POP([C++])
    
    if test "x$ax_boost_filesystem_headers" = "xyes" -a "x$ax_boost_filesystem_links" = "xyes"; then
      AC_DEFINE(HAVE_BOOST_FILESYSTEM,,[define if the Boost.Filesystem library is available])
      AC_SUBST([BOOST_FILESYSTEM_LIBS])
      # execute ACTION-IF-FOUND (if present):
      ifelse([$1], , :, [$1])
    else
      # execute ACTION-IF-NOT-FOUND (if present):
      ifelse([$2], , :, [$2])
    fi
    
    CPPFLAGS="$CPPFLAGS_SAVED"
    LDFLAGS="$LDFLAGS_SAVED"
  fi
])
