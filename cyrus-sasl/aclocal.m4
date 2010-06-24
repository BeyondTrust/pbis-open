# generated automatically by aclocal 1.7.9 -*- Autoconf -*-

# Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002
# Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# Do all the work for Automake.                            -*- Autoconf -*-

# This macro actually does too much some checks are only needed if
# your package does certain things.  But this isn't really a big deal.

# Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
# Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 10

AC_PREREQ([2.54])

# Autoconf 2.50 wants to disallow AM_ names.  We explicitly allow
# the ones we care about.
m4_pattern_allow([^AM_[A-Z]+FLAGS$])dnl

# AM_INIT_AUTOMAKE(PACKAGE, VERSION, [NO-DEFINE])
# AM_INIT_AUTOMAKE([OPTIONS])
# -----------------------------------------------
# The call with PACKAGE and VERSION arguments is the old style
# call (pre autoconf-2.50), which is being phased out.  PACKAGE
# and VERSION should now be passed to AC_INIT and removed from
# the call to AM_INIT_AUTOMAKE.
# We support both call styles for the transition.  After
# the next Automake release, Autoconf can make the AC_INIT
# arguments mandatory, and then we can depend on a new Autoconf
# release and drop the old call support.
AC_DEFUN([AM_INIT_AUTOMAKE],
[AC_REQUIRE([AM_SET_CURRENT_AUTOMAKE_VERSION])dnl
 AC_REQUIRE([AC_PROG_INSTALL])dnl
# test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" &&
   test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi

# test whether we have cygpath
if test -z "$CYGPATH_W"; then
  if (cygpath --version) >/dev/null 2>/dev/null; then
    CYGPATH_W='cygpath -w'
  else
    CYGPATH_W=echo
  fi
fi
AC_SUBST([CYGPATH_W])

# Define the identity of the package.
dnl Distinguish between old-style and new-style calls.
m4_ifval([$2],
[m4_ifval([$3], [_AM_SET_OPTION([no-define])])dnl
 AC_SUBST([PACKAGE], [$1])dnl
 AC_SUBST([VERSION], [$2])],
[_AM_SET_OPTIONS([$1])dnl
 AC_SUBST([PACKAGE], ['AC_PACKAGE_TARNAME'])dnl
 AC_SUBST([VERSION], ['AC_PACKAGE_VERSION'])])dnl

_AM_IF_OPTION([no-define],,
[AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
 AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package])])dnl

# Some tools Automake needs.
AC_REQUIRE([AM_SANITY_CHECK])dnl
AC_REQUIRE([AC_ARG_PROGRAM])dnl
AM_MISSING_PROG(ACLOCAL, aclocal-${am__api_version})
AM_MISSING_PROG(AUTOCONF, autoconf)
AM_MISSING_PROG(AUTOMAKE, automake-${am__api_version})
AM_MISSING_PROG(AUTOHEADER, autoheader)
AM_MISSING_PROG(MAKEINFO, makeinfo)
AM_MISSING_PROG(AMTAR, tar)
AM_PROG_INSTALL_SH
AM_PROG_INSTALL_STRIP
# We need awk for the "check" target.  The system "awk" is bad on
# some platforms.
AC_REQUIRE([AC_PROG_AWK])dnl
AC_REQUIRE([AC_PROG_MAKE_SET])dnl
AC_REQUIRE([AM_SET_LEADING_DOT])dnl

_AM_IF_OPTION([no-dependencies],,
[AC_PROVIDE_IFELSE([AC_PROG_CC],
                  [_AM_DEPENDENCIES(CC)],
                  [define([AC_PROG_CC],
                          defn([AC_PROG_CC])[_AM_DEPENDENCIES(CC)])])dnl
AC_PROVIDE_IFELSE([AC_PROG_CXX],
                  [_AM_DEPENDENCIES(CXX)],
                  [define([AC_PROG_CXX],
                          defn([AC_PROG_CXX])[_AM_DEPENDENCIES(CXX)])])dnl
])
])


# When config.status generates a header, we must update the stamp-h file.
# This file resides in the same directory as the config header
# that is generated.  The stamp files are numbered to have different names.

# Autoconf calls _AC_AM_CONFIG_HEADER_HOOK (when defined) in the
# loop where config.status creates the headers, so we can generate
# our stamp files there.
AC_DEFUN([_AC_AM_CONFIG_HEADER_HOOK],
[# Compute $1's index in $config_headers.
_am_stamp_count=1
for _am_header in $config_headers :; do
  case $_am_header in
    $1 | $1:* )
      break ;;
    * )
      _am_stamp_count=`expr $_am_stamp_count + 1` ;;
  esac
done
echo "timestamp for $1" >`AS_DIRNAME([$1])`/stamp-h[]$_am_stamp_count])

# Copyright 2002  Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA

# AM_AUTOMAKE_VERSION(VERSION)
# ----------------------------
# Automake X.Y traces this macro to ensure aclocal.m4 has been
# generated from the m4 files accompanying Automake X.Y.
AC_DEFUN([AM_AUTOMAKE_VERSION],[am__api_version="1.7"])

# AM_SET_CURRENT_AUTOMAKE_VERSION
# -------------------------------
# Call AM_AUTOMAKE_VERSION so it can be traced.
# This function is AC_REQUIREd by AC_INIT_AUTOMAKE.
AC_DEFUN([AM_SET_CURRENT_AUTOMAKE_VERSION],
	 [AM_AUTOMAKE_VERSION([1.7.9])])

# Helper functions for option handling.                    -*- Autoconf -*-

# Copyright 2001, 2002  Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 2

# _AM_MANGLE_OPTION(NAME)
# -----------------------
AC_DEFUN([_AM_MANGLE_OPTION],
[[_AM_OPTION_]m4_bpatsubst($1, [[^a-zA-Z0-9_]], [_])])

# _AM_SET_OPTION(NAME)
# ------------------------------
# Set option NAME.  Presently that only means defining a flag for this option.
AC_DEFUN([_AM_SET_OPTION],
[m4_define(_AM_MANGLE_OPTION([$1]), 1)])

# _AM_SET_OPTIONS(OPTIONS)
# ----------------------------------
# OPTIONS is a space-separated list of Automake options.
AC_DEFUN([_AM_SET_OPTIONS],
[AC_FOREACH([_AM_Option], [$1], [_AM_SET_OPTION(_AM_Option)])])

# _AM_IF_OPTION(OPTION, IF-SET, [IF-NOT-SET])
# -------------------------------------------
# Execute IF-SET if OPTION is set, IF-NOT-SET otherwise.
AC_DEFUN([_AM_IF_OPTION],
[m4_ifset(_AM_MANGLE_OPTION([$1]), [$2], [$3])])

#
# Check to make sure that the build environment is sane.
#

# Copyright 1996, 1997, 2000, 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 3

# AM_SANITY_CHECK
# ---------------
AC_DEFUN([AM_SANITY_CHECK],
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftest.file
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftest.file 2> /dev/null`
   if test "$[*]" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftest.file`
   fi
   rm -f conftest.file
   if test "$[*]" != "X $srcdir/configure conftest.file" \
      && test "$[*]" != "X conftest.file $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "$[2]" = conftest.file
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
AC_MSG_RESULT(yes)])

#  -*- Autoconf -*-


# Copyright 1997, 1999, 2000, 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 3

# AM_MISSING_PROG(NAME, PROGRAM)
# ------------------------------
AC_DEFUN([AM_MISSING_PROG],
[AC_REQUIRE([AM_MISSING_HAS_RUN])
$1=${$1-"${am_missing_run}$2"}
AC_SUBST($1)])


# AM_MISSING_HAS_RUN
# ------------------
# Define MISSING if not defined so far and test if it supports --run.
# If it does, set am_missing_run to use it, otherwise, to nothing.
AC_DEFUN([AM_MISSING_HAS_RUN],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
test x"${MISSING+set}" = xset || MISSING="\${SHELL} $am_aux_dir/missing"
# Use eval to expand $SHELL
if eval "$MISSING --run true"; then
  am_missing_run="$MISSING --run "
else
  am_missing_run=
  AC_MSG_WARN([`missing' script is too old or missing])
fi
])

# AM_AUX_DIR_EXPAND

# Copyright 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# For projects using AC_CONFIG_AUX_DIR([foo]), Autoconf sets
# $ac_aux_dir to `$srcdir/foo'.  In other projects, it is set to
# `$srcdir', `$srcdir/..', or `$srcdir/../..'.
#
# Of course, Automake must honor this variable whenever it calls a
# tool from the auxiliary directory.  The problem is that $srcdir (and
# therefore $ac_aux_dir as well) can be either absolute or relative,
# depending on how configure is run.  This is pretty annoying, since
# it makes $ac_aux_dir quite unusable in subdirectories: in the top
# source directory, any form will work fine, but in subdirectories a
# relative path needs to be adjusted first.
#
# $ac_aux_dir/missing
#    fails when called from a subdirectory if $ac_aux_dir is relative
# $top_srcdir/$ac_aux_dir/missing
#    fails if $ac_aux_dir is absolute,
#    fails when called from a subdirectory in a VPATH build with
#          a relative $ac_aux_dir
#
# The reason of the latter failure is that $top_srcdir and $ac_aux_dir
# are both prefixed by $srcdir.  In an in-source build this is usually
# harmless because $srcdir is `.', but things will broke when you
# start a VPATH build or use an absolute $srcdir.
#
# So we could use something similar to $top_srcdir/$ac_aux_dir/missing,
# iff we strip the leading $srcdir from $ac_aux_dir.  That would be:
#   am_aux_dir='\$(top_srcdir)/'`expr "$ac_aux_dir" : "$srcdir//*\(.*\)"`
# and then we would define $MISSING as
#   MISSING="\${SHELL} $am_aux_dir/missing"
# This will work as long as MISSING is not called from configure, because
# unfortunately $(top_srcdir) has no meaning in configure.
# However there are other variables, like CC, which are often used in
# configure, and could therefore not use this "fixed" $ac_aux_dir.
#
# Another solution, used here, is to always expand $ac_aux_dir to an
# absolute PATH.  The drawback is that using absolute paths prevent a
# configured tree to be moved without reconfiguration.

# Rely on autoconf to set up CDPATH properly.
AC_PREREQ([2.50])

AC_DEFUN([AM_AUX_DIR_EXPAND], [
# expand $ac_aux_dir to an absolute path
am_aux_dir=`cd $ac_aux_dir && pwd`
])

# AM_PROG_INSTALL_SH
# ------------------
# Define $install_sh.

# Copyright 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

AC_DEFUN([AM_PROG_INSTALL_SH],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
install_sh=${install_sh-"$am_aux_dir/install-sh"}
AC_SUBST(install_sh)])

# AM_PROG_INSTALL_STRIP

# Copyright 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# One issue with vendor `install' (even GNU) is that you can't
# specify the program used to strip binaries.  This is especially
# annoying in cross-compiling environments, where the build's strip
# is unlikely to handle the host's binaries.
# Fortunately install-sh will honor a STRIPPROG variable, so we
# always use install-sh in `make install-strip', and initialize
# STRIPPROG with the value of the STRIP variable (set by the user).
AC_DEFUN([AM_PROG_INSTALL_STRIP],
[AC_REQUIRE([AM_PROG_INSTALL_SH])dnl
# Installed binaries are usually stripped using `strip' when the user
# run `make install-strip'.  However `strip' might not be the right
# tool to use in cross-compilation environments, therefore Automake
# will honor the `STRIP' environment variable to overrule this program.
dnl Don't test for $cross_compiling = yes, because it might be `maybe'.
if test "$cross_compiling" != no; then
  AC_CHECK_TOOL([STRIP], [strip], :)
fi
INSTALL_STRIP_PROGRAM="\${SHELL} \$(install_sh) -c -s"
AC_SUBST([INSTALL_STRIP_PROGRAM])])

#                                                          -*- Autoconf -*-
# Copyright (C) 2003  Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 1

# Check whether the underlying file-system supports filenames
# with a leading dot.  For instance MS-DOS doesn't.
AC_DEFUN([AM_SET_LEADING_DOT],
[rm -rf .tst 2>/dev/null
mkdir .tst 2>/dev/null
if test -d .tst; then
  am__leading_dot=.
else
  am__leading_dot=_
fi
rmdir .tst 2>/dev/null
AC_SUBST([am__leading_dot])])

# serial 5						-*- Autoconf -*-

# Copyright (C) 1999, 2000, 2001, 2002, 2003  Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.


# There are a few dirty hacks below to avoid letting `AC_PROG_CC' be
# written in clear, in which case automake, when reading aclocal.m4,
# will think it sees a *use*, and therefore will trigger all it's
# C support machinery.  Also note that it means that autoscan, seeing
# CC etc. in the Makefile, will ask for an AC_PROG_CC use...



# _AM_DEPENDENCIES(NAME)
# ----------------------
# See how the compiler implements dependency checking.
# NAME is "CC", "CXX", "GCJ", or "OBJC".
# We try a few techniques and use that to set a single cache variable.
#
# We don't AC_REQUIRE the corresponding AC_PROG_CC since the latter was
# modified to invoke _AM_DEPENDENCIES(CC); we would have a circular
# dependency, and given that the user is not expected to run this macro,
# just rely on AC_PROG_CC.
AC_DEFUN([_AM_DEPENDENCIES],
[AC_REQUIRE([AM_SET_DEPDIR])dnl
AC_REQUIRE([AM_OUTPUT_DEPENDENCY_COMMANDS])dnl
AC_REQUIRE([AM_MAKE_INCLUDE])dnl
AC_REQUIRE([AM_DEP_TRACK])dnl

ifelse([$1], CC,   [depcc="$CC"   am_compiler_list=],
       [$1], CXX,  [depcc="$CXX"  am_compiler_list=],
       [$1], OBJC, [depcc="$OBJC" am_compiler_list='gcc3 gcc'],
       [$1], GCJ,  [depcc="$GCJ"  am_compiler_list='gcc3 gcc'],
                   [depcc="$$1"   am_compiler_list=])

AC_CACHE_CHECK([dependency style of $depcc],
               [am_cv_$1_dependencies_compiler_type],
[if test -z "$AMDEP_TRUE" && test -f "$am_depcomp"; then
  # We make a subdir and do the tests there.  Otherwise we can end up
  # making bogus files that we don't know about and never remove.  For
  # instance it was reported that on HP-UX the gcc test will end up
  # making a dummy file named `D' -- because `-MD' means `put the output
  # in D'.
  mkdir conftest.dir
  # Copy depcomp to subdir because otherwise we won't find it if we're
  # using a relative directory.
  cp "$am_depcomp" conftest.dir
  cd conftest.dir
  # We will build objects and dependencies in a subdirectory because
  # it helps to detect inapplicable dependency modes.  For instance
  # both Tru64's cc and ICC support -MD to output dependencies as a
  # side effect of compilation, but ICC will put the dependencies in
  # the current directory while Tru64 will put them in the object
  # directory.
  mkdir sub

  am_cv_$1_dependencies_compiler_type=none
  if test "$am_compiler_list" = ""; then
     am_compiler_list=`sed -n ['s/^#*\([a-zA-Z0-9]*\))$/\1/p'] < ./depcomp`
  fi
  for depmode in $am_compiler_list; do
    # Setup a source with many dependencies, because some compilers
    # like to wrap large dependency lists on column 80 (with \), and
    # we should not choose a depcomp mode which is confused by this.
    #
    # We need to recreate these files for each test, as the compiler may
    # overwrite some of them when testing with obscure command lines.
    # This happens at least with the AIX C compiler.
    : > sub/conftest.c
    for i in 1 2 3 4 5 6; do
      echo '#include "conftst'$i'.h"' >> sub/conftest.c
      : > sub/conftst$i.h
    done
    echo "${am__include} ${am__quote}sub/conftest.Po${am__quote}" > confmf

    case $depmode in
    nosideeffect)
      # after this tag, mechanisms are not by side-effect, so they'll
      # only be used when explicitly requested
      if test "x$enable_dependency_tracking" = xyes; then
	continue
      else
	break
      fi
      ;;
    none) break ;;
    esac
    # We check with `-c' and `-o' for the sake of the "dashmstdout"
    # mode.  It turns out that the SunPro C++ compiler does not properly
    # handle `-M -o', and we need to detect this.
    if depmode=$depmode \
       source=sub/conftest.c object=sub/conftest.${OBJEXT-o} \
       depfile=sub/conftest.Po tmpdepfile=sub/conftest.TPo \
       $SHELL ./depcomp $depcc -c -o sub/conftest.${OBJEXT-o} sub/conftest.c \
         >/dev/null 2>conftest.err &&
       grep sub/conftst6.h sub/conftest.Po > /dev/null 2>&1 &&
       grep sub/conftest.${OBJEXT-o} sub/conftest.Po > /dev/null 2>&1 &&
       ${MAKE-make} -s -f confmf > /dev/null 2>&1; then
      # icc doesn't choke on unknown options, it will just issue warnings
      # (even with -Werror).  So we grep stderr for any message
      # that says an option was ignored.
      if grep 'ignoring option' conftest.err >/dev/null 2>&1; then :; else
        am_cv_$1_dependencies_compiler_type=$depmode
        break
      fi
    fi
  done

  cd ..
  rm -rf conftest.dir
else
  am_cv_$1_dependencies_compiler_type=none
fi
])
AC_SUBST([$1DEPMODE], [depmode=$am_cv_$1_dependencies_compiler_type])
AM_CONDITIONAL([am__fastdep$1], [
  test "x$enable_dependency_tracking" != xno \
  && test "$am_cv_$1_dependencies_compiler_type" = gcc3])
])


# AM_SET_DEPDIR
# -------------
# Choose a directory name for dependency files.
# This macro is AC_REQUIREd in _AM_DEPENDENCIES
AC_DEFUN([AM_SET_DEPDIR],
[AC_REQUIRE([AM_SET_LEADING_DOT])dnl
AC_SUBST([DEPDIR], ["${am__leading_dot}deps"])dnl
])


# AM_DEP_TRACK
# ------------
AC_DEFUN([AM_DEP_TRACK],
[AC_ARG_ENABLE(dependency-tracking,
[  --disable-dependency-tracking Speeds up one-time builds
  --enable-dependency-tracking  Do not reject slow dependency extractors])
if test "x$enable_dependency_tracking" != xno; then
  am_depcomp="$ac_aux_dir/depcomp"
  AMDEPBACKSLASH='\'
fi
AM_CONDITIONAL([AMDEP], [test "x$enable_dependency_tracking" != xno])
AC_SUBST([AMDEPBACKSLASH])
])

# Generate code to set up dependency tracking.   -*- Autoconf -*-

# Copyright 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

#serial 2

# _AM_OUTPUT_DEPENDENCY_COMMANDS
# ------------------------------
AC_DEFUN([_AM_OUTPUT_DEPENDENCY_COMMANDS],
[for mf in $CONFIG_FILES; do
  # Strip MF so we end up with the name of the file.
  mf=`echo "$mf" | sed -e 's/:.*$//'`
  # Check whether this is an Automake generated Makefile or not.
  # We used to match only the files named `Makefile.in', but
  # some people rename them; so instead we look at the file content.
  # Grep'ing the first line is not enough: some people post-process
  # each Makefile.in and add a new line on top of each file to say so.
  # So let's grep whole file.
  if grep '^#.*generated by automake' $mf > /dev/null 2>&1; then
    dirpart=`AS_DIRNAME("$mf")`
  else
    continue
  fi
  grep '^DEP_FILES *= *[[^ @%:@]]' < "$mf" > /dev/null || continue
  # Extract the definition of DEP_FILES from the Makefile without
  # running `make'.
  DEPDIR=`sed -n -e '/^DEPDIR = / s///p' < "$mf"`
  test -z "$DEPDIR" && continue
  # When using ansi2knr, U may be empty or an underscore; expand it
  U=`sed -n -e '/^U = / s///p' < "$mf"`
  test -d "$dirpart/$DEPDIR" || mkdir "$dirpart/$DEPDIR"
  # We invoke sed twice because it is the simplest approach to
  # changing $(DEPDIR) to its actual value in the expansion.
  for file in `sed -n -e '
    /^DEP_FILES = .*\\\\$/ {
      s/^DEP_FILES = //
      :loop
	s/\\\\$//
	p
	n
	/\\\\$/ b loop
      p
    }
    /^DEP_FILES = / s/^DEP_FILES = //p' < "$mf" | \
       sed -e 's/\$(DEPDIR)/'"$DEPDIR"'/g' -e 's/\$U/'"$U"'/g'`; do
    # Make sure the directory exists.
    test -f "$dirpart/$file" && continue
    fdir=`AS_DIRNAME(["$file"])`
    AS_MKDIR_P([$dirpart/$fdir])
    # echo "creating $dirpart/$file"
    echo '# dummy' > "$dirpart/$file"
  done
done
])# _AM_OUTPUT_DEPENDENCY_COMMANDS


# AM_OUTPUT_DEPENDENCY_COMMANDS
# -----------------------------
# This macro should only be invoked once -- use via AC_REQUIRE.
#
# This code is only required when automatic dependency tracking
# is enabled.  FIXME.  This creates each `.P' file that we will
# need in order to bootstrap the dependency handling code.
AC_DEFUN([AM_OUTPUT_DEPENDENCY_COMMANDS],
[AC_CONFIG_COMMANDS([depfiles],
     [test x"$AMDEP_TRUE" != x"" || _AM_OUTPUT_DEPENDENCY_COMMANDS],
     [AMDEP_TRUE="$AMDEP_TRUE" ac_aux_dir="$ac_aux_dir"])
])

# Check to see how 'make' treats includes.	-*- Autoconf -*-

# Copyright (C) 2001, 2002, 2003 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 2

# AM_MAKE_INCLUDE()
# -----------------
# Check to see how make treats includes.
AC_DEFUN([AM_MAKE_INCLUDE],
[am_make=${MAKE-make}
cat > confinc << 'END'
am__doit:
	@echo done
.PHONY: am__doit
END
# If we don't find an include directive, just comment out the code.
AC_MSG_CHECKING([for style of include used by $am_make])
am__include="#"
am__quote=
_am_result=none
# First try GNU make style include.
echo "include confinc" > confmf
# We grep out `Entering directory' and `Leaving directory'
# messages which can occur if `w' ends up in MAKEFLAGS.
# In particular we don't look at `^make:' because GNU make might
# be invoked under some other name (usually "gmake"), in which
# case it prints its new name instead of `make'.
if test "`$am_make -s -f confmf 2> /dev/null | grep -v 'ing directory'`" = "done"; then
   am__include=include
   am__quote=
   _am_result=GNU
fi
# Now try BSD make style include.
if test "$am__include" = "#"; then
   echo '.include "confinc"' > confmf
   if test "`$am_make -s -f confmf 2> /dev/null`" = "done"; then
      am__include=.include
      am__quote="\""
      _am_result=BSD
   fi
fi
AC_SUBST([am__include])
AC_SUBST([am__quote])
AC_MSG_RESULT([$_am_result])
rm -f confinc confmf
])

# AM_CONDITIONAL                                              -*- Autoconf -*-

# Copyright 1997, 2000, 2001 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# serial 5

AC_PREREQ(2.52)

# AM_CONDITIONAL(NAME, SHELL-CONDITION)
# -------------------------------------
# Define a conditional.
AC_DEFUN([AM_CONDITIONAL],
[ifelse([$1], [TRUE],  [AC_FATAL([$0: invalid condition: $1])],
        [$1], [FALSE], [AC_FATAL([$0: invalid condition: $1])])dnl
AC_SUBST([$1_TRUE])
AC_SUBST([$1_FALSE])
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi
AC_CONFIG_COMMANDS_PRE(
[if test -z "${$1_TRUE}" && test -z "${$1_FALSE}"; then
  AC_MSG_ERROR([conditional "$1" was never defined.
Usually this means the macro was only invoked conditionally.])
fi])])

dnl init_automake.m4--cmulocal automake setup macro
dnl Rob Earhart
dnl $Id: init_automake.m4,v 1.4 2003/10/08 20:35:24 rjs3 Exp $

AC_DEFUN([CMU_INIT_AUTOMAKE], [
	AC_REQUIRE([AM_INIT_AUTOMAKE])
	ACLOCAL="$ACLOCAL -I \$(top_srcdir)/cmulocal"
	])

dnl
dnl $Id: c-attribute.m4,v 1.3 2003/10/08 20:35:24 rjs3 Exp $
dnl

dnl
dnl Test for __attribute__
dnl

AC_DEFUN([CMU_C___ATTRIBUTE__], [
AC_MSG_CHECKING(for __attribute__)
AC_CACHE_VAL(ac_cv___attribute__, [
AC_TRY_COMPILE([
#include <stdlib.h>
],
[
static void foo(void) __attribute__ ((noreturn));

static void
foo(void)
{
  exit(1);
}
],
ac_cv___attribute__=yes,
ac_cv___attribute__=no)])
if test "$ac_cv___attribute__" = "yes"; then
  AC_DEFINE(HAVE___ATTRIBUTE__, 1, [define if your compiler has __attribute__])
fi
AC_MSG_RESULT($ac_cv___attribute__)
])


dnl
dnl Additional macros for configure.in packaged up for easier theft.
dnl $Id: cyrus.m4,v 1.4 2003/10/08 20:35:24 rjs3 Exp $
dnl tjs@andrew.cmu.edu 6-may-1998
dnl

dnl It would be good if ANDREW_ADD_LIBPATH could detect if something was
dnl already there and not redundantly add it if it is.

dnl add -L(arg), and possibly (runpath switch)(arg), to LDFLAGS
dnl (so the runpath for shared libraries is set).
AC_DEFUN([CMU_ADD_LIBPATH], [
  # this is CMU ADD LIBPATH
  if test "$andrew_runpath_switch" = "none" ; then
	LDFLAGS="-L$1 ${LDFLAGS}"
  else
	LDFLAGS="-L$1 $andrew_runpath_switch$1 ${LDFLAGS}"
  fi
])

dnl add -L(1st arg), and possibly (runpath switch)(1st arg), to (2nd arg)
dnl (so the runpath for shared libraries is set).
AC_DEFUN([CMU_ADD_LIBPATH_TO], [
  # this is CMU ADD LIBPATH TO
  if test "$andrew_runpath_switch" = "none" ; then
	$2="-L$1 ${$2}"
  else
	$2="-L$1 ${$2} $andrew_runpath_switch$1"
  fi
])

dnl runpath initialization
AC_DEFUN([CMU_GUESS_RUNPATH_SWITCH], [
   # CMU GUESS RUNPATH SWITCH
  AC_CACHE_CHECK(for runpath switch, andrew_runpath_switch, [
    # first, try -R
    SAVE_LDFLAGS="${LDFLAGS}"
    LDFLAGS="-R /usr/lib"
    AC_TRY_LINK([],[],[andrew_runpath_switch="-R"], [
  	LDFLAGS="-Wl,-rpath,/usr/lib"
    AC_TRY_LINK([],[],[andrew_runpath_switch="-Wl,-rpath,"],
    [andrew_runpath_switch="none"])
    ])
  LDFLAGS="${SAVE_LDFLAGS}"
  ])])


# serial 40 AC_PROG_LIBTOOL
AC_DEFUN([AC_PROG_LIBTOOL],
[AC_REQUIRE([AC_LIBTOOL_SETUP])dnl

# Save cache, so that ltconfig can load it
AC_CACHE_SAVE

# Actually configure libtool.  ac_aux_dir is where install-sh is found.
CC="$CC" CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" \
LD="$LD" LDFLAGS="$LDFLAGS" LIBS="$LIBS" \
LN_S="$LN_S" NM="$NM" RANLIB="$RANLIB" \
DLLTOOL="$DLLTOOL" AS="$AS" OBJDUMP="$OBJDUMP" \
${CONFIG_SHELL-/bin/sh} $ac_aux_dir/ltconfig --no-reexec \
$libtool_flags --no-verify $ac_aux_dir/ltmain.sh $lt_target \
|| AC_MSG_ERROR([libtool configure failed])

# Reload cache, that may have been modified by ltconfig
AC_CACHE_LOAD

# This can be used to rebuild libtool when needed
LIBTOOL_DEPS="$ac_aux_dir/ltconfig $ac_aux_dir/ltmain.sh"

# Always use our own libtool.
LIBTOOL='$(SHELL) $(top_builddir)/libtool'
AC_SUBST(LIBTOOL)dnl

# Redirect the config.log output again, so that the ltconfig log is not
# clobbered by the next message.
exec 5>>./config.log
])

AC_DEFUN([AC_LIBTOOL_SETUP],
[AC_PREREQ(2.13)dnl
AC_REQUIRE([AC_ENABLE_SHARED])dnl
AC_REQUIRE([AC_ENABLE_STATIC])dnl
AC_REQUIRE([AC_ENABLE_FAST_INSTALL])dnl
AC_REQUIRE([AC_CANONICAL_HOST])dnl
AC_REQUIRE([AC_CANONICAL_BUILD])dnl
AC_REQUIRE([AC_PROG_RANLIB])dnl
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_LD])dnl
AC_REQUIRE([AC_PROG_NM])dnl
AC_REQUIRE([AC_PROG_LN_S])dnl
dnl

case "$target" in
NONE) lt_target="$host" ;;
*) lt_target="$target" ;;
esac

# Check for any special flags to pass to ltconfig.
libtool_flags="--cache-file=$cache_file"
test "$enable_shared" = no && libtool_flags="$libtool_flags --disable-shared"
test "$enable_static" = no && libtool_flags="$libtool_flags --disable-static"
test "$enable_fast_install" = no && libtool_flags="$libtool_flags --disable-fast-install"
test "$ac_cv_prog_gcc" = yes && libtool_flags="$libtool_flags --with-gcc"
test "$ac_cv_prog_gnu_ld" = yes && libtool_flags="$libtool_flags --with-gnu-ld"
ifdef([AC_PROVIDE_AC_LIBTOOL_DLOPEN],
[libtool_flags="$libtool_flags --enable-dlopen"])
ifdef([AC_PROVIDE_AC_LIBTOOL_WIN32_DLL],
[libtool_flags="$libtool_flags --enable-win32-dll"])
AC_ARG_ENABLE(libtool-lock,
  [  --disable-libtool-lock  avoid locking (might break parallel builds)])
test "x$enable_libtool_lock" = xno && libtool_flags="$libtool_flags --disable-lock"
test x"$silent" = xyes && libtool_flags="$libtool_flags --silent"

# Some flags need to be propagated to the compiler or linker for good
# libtool support.
case "$lt_target" in
*-*-irix6*)
  # Find out which ABI we are using.
  echo '[#]line __oline__ "configure"' > conftest.$ac_ext
  if AC_TRY_EVAL(ac_compile); then
    case "`/usr/bin/file conftest.o`" in
    *32-bit*)
      LD="${LD-ld} -32"
      ;;
    *N32*)
      LD="${LD-ld} -n32"
      ;;
    *64-bit*)
      LD="${LD-ld} -64"
      ;;
    esac
  fi
  rm -rf conftest*
  ;;

*-*-sco3.2v5*)
  # On SCO OpenServer 5, we need -belf to get full-featured binaries.
  SAVE_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -belf"
  AC_CACHE_CHECK([whether the C compiler needs -belf], lt_cv_cc_needs_belf,
    [AC_TRY_LINK([],[],[lt_cv_cc_needs_belf=yes],[lt_cv_cc_needs_belf=no])])
  if test x"$lt_cv_cc_needs_belf" != x"yes"; then
    # this is probably gcc 2.8.0, egcs 1.0 or newer; no need for -belf
    CFLAGS="$SAVE_CFLAGS"
  fi
  ;;

ifdef([AC_PROVIDE_AC_LIBTOOL_WIN32_DLL],
[*-*-cygwin* | *-*-mingw*)
  AC_CHECK_TOOL(DLLTOOL, dlltool, false)
  AC_CHECK_TOOL(AS, as, false)
  AC_CHECK_TOOL(OBJDUMP, objdump, false)
  ;;
])
esac
])

# AC_LIBTOOL_DLOPEN - enable checks for dlopen support
AC_DEFUN([AC_LIBTOOL_DLOPEN], [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])])

# AC_LIBTOOL_WIN32_DLL - declare package support for building win32 dll's
AC_DEFUN([AC_LIBTOOL_WIN32_DLL], [AC_BEFORE([$0], [AC_LIBTOOL_SETUP])])

# AC_ENABLE_SHARED - implement the --enable-shared flag
# Usage: AC_ENABLE_SHARED[(DEFAULT)]
#   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
#   `yes'.
AC_DEFUN([AC_ENABLE_SHARED], [dnl
define([AC_ENABLE_SHARED_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(shared,
changequote(<<, >>)dnl
<<  --enable-shared[=PKGS]  build shared libraries [default=>>AC_ENABLE_SHARED_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case "$enableval" in
yes) enable_shared=yes ;;
no) enable_shared=no ;;
*)
  enable_shared=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_shared=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_shared=AC_ENABLE_SHARED_DEFAULT)dnl
])

# AC_DISABLE_SHARED - set the default shared flag to --disable-shared
AC_DEFUN([AC_DISABLE_SHARED], [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
AC_ENABLE_SHARED(no)])

# AC_ENABLE_STATIC - implement the --enable-static flag
# Usage: AC_ENABLE_STATIC[(DEFAULT)]
#   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
#   `yes'.
AC_DEFUN([AC_ENABLE_STATIC], [dnl
define([AC_ENABLE_STATIC_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(static,
changequote(<<, >>)dnl
<<  --enable-static[=PKGS]  build static libraries [default=>>AC_ENABLE_STATIC_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case "$enableval" in
yes) enable_static=yes ;;
no) enable_static=no ;;
*)
  enable_static=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_static=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_static=AC_ENABLE_STATIC_DEFAULT)dnl
])

# AC_DISABLE_STATIC - set the default static flag to --disable-static
AC_DEFUN([AC_DISABLE_STATIC], [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
AC_ENABLE_STATIC(no)])


# AC_ENABLE_FAST_INSTALL - implement the --enable-fast-install flag
# Usage: AC_ENABLE_FAST_INSTALL[(DEFAULT)]
#   Where DEFAULT is either `yes' or `no'.  If omitted, it defaults to
#   `yes'.
AC_DEFUN([AC_ENABLE_FAST_INSTALL], [dnl
define([AC_ENABLE_FAST_INSTALL_DEFAULT], ifelse($1, no, no, yes))dnl
AC_ARG_ENABLE(fast-install,
changequote(<<, >>)dnl
<<  --enable-fast-install[=PKGS]  optimize for fast installation [default=>>AC_ENABLE_FAST_INSTALL_DEFAULT],
changequote([, ])dnl
[p=${PACKAGE-default}
case "$enableval" in
yes) enable_fast_install=yes ;;
no) enable_fast_install=no ;;
*)
  enable_fast_install=no
  # Look at the argument we got.  We use all the common list separators.
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}:,"
  for pkg in $enableval; do
    if test "X$pkg" = "X$p"; then
      enable_fast_install=yes
    fi
  done
  IFS="$ac_save_ifs"
  ;;
esac],
enable_fast_install=AC_ENABLE_FAST_INSTALL_DEFAULT)dnl
])

# AC_ENABLE_FAST_INSTALL - set the default to --disable-fast-install
AC_DEFUN([AC_DISABLE_FAST_INSTALL], [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
AC_ENABLE_FAST_INSTALL(no)])

# AC_PROG_LD - find the path to the GNU or non-GNU linker
AC_DEFUN([AC_PROG_LD],
[AC_ARG_WITH(gnu-ld,
[  --with-gnu-ld           assume the C compiler uses GNU ld [default=no]],
test "$withval" = no || with_gnu_ld=yes, with_gnu_ld=no)
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_CANONICAL_HOST])dnl
AC_REQUIRE([AC_CANONICAL_BUILD])dnl
ac_prog=ld
if test "$ac_cv_prog_gcc" = yes; then
  # Check if gcc -print-prog-name=ld gives a path.
  AC_MSG_CHECKING([for ld used by GCC])
  ac_prog=`($CC -print-prog-name=ld) 2>&5`
  case "$ac_prog" in
    # Accept absolute paths.
changequote(,)dnl
    [\\/]* | [A-Za-z]:[\\/]*)
      re_direlt='/[^/][^/]*/\.\./'
changequote([,])dnl
      # Canonicalize the path of ld
      ac_prog=`echo $ac_prog| sed 's%\\\\%/%g'`
      while echo $ac_prog | grep "$re_direlt" > /dev/null 2>&1; do
	ac_prog=`echo $ac_prog| sed "s%$re_direlt%/%"`
      done
      test -z "$LD" && LD="$ac_prog"
      ;;
  "")
    # If it fails, then pretend we aren't using GCC.
    ac_prog=ld
    ;;
  *)
    # If it is relative, then search for the first ld in PATH.
    with_gnu_ld=unknown
    ;;
  esac
elif test "$with_gnu_ld" = yes; then
  AC_MSG_CHECKING([for GNU ld])
else
  AC_MSG_CHECKING([for non-GNU ld])
fi
AC_CACHE_VAL(ac_cv_path_LD,
[if test -z "$LD"; then
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}${PATH_SEPARATOR-:}"
  for ac_dir in $PATH; do
    test -z "$ac_dir" && ac_dir=.
    if test -f "$ac_dir/$ac_prog" || test -f "$ac_dir/$ac_prog$ac_exeext"; then
      ac_cv_path_LD="$ac_dir/$ac_prog"
      # Check to see if the program is GNU ld.  I'd rather use --version,
      # but apparently some GNU ld's only accept -v.
      # Break only if it was the GNU/non-GNU ld that we prefer.
      if "$ac_cv_path_LD" -v 2>&1 < /dev/null | egrep '(GNU|with BFD)' > /dev/null; then
	test "$with_gnu_ld" != no && break
      else
	test "$with_gnu_ld" != yes && break
      fi
    fi
  done
  IFS="$ac_save_ifs"
else
  ac_cv_path_LD="$LD" # Let the user override the test with a path.
fi])
LD="$ac_cv_path_LD"
if test -n "$LD"; then
  AC_MSG_RESULT($LD)
else
  AC_MSG_RESULT(no)
fi
test -z "$LD" && AC_MSG_ERROR([no acceptable ld found in \$PATH])
AC_PROG_LD_GNU
])

AC_DEFUN([AC_PROG_LD_GNU],
[AC_CACHE_CHECK([if the linker ($LD) is GNU ld], ac_cv_prog_gnu_ld,
[# I'd rather use --version here, but apparently some GNU ld's only accept -v.
if $LD -v 2>&1 </dev/null | egrep '(GNU|with BFD)' 1>&5; then
  ac_cv_prog_gnu_ld=yes
else
  ac_cv_prog_gnu_ld=no
fi])
])

# AC_PROG_NM - find the path to a BSD-compatible name lister
AC_DEFUN([AC_PROG_NM],
[AC_MSG_CHECKING([for BSD-compatible nm])
AC_CACHE_VAL(ac_cv_path_NM,
[if test -n "$NM"; then
  # Let the user override the test.
  ac_cv_path_NM="$NM"
else
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}${PATH_SEPARATOR-:}"
  for ac_dir in $PATH /usr/ccs/bin /usr/ucb /bin; do
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/nm || test -f $ac_dir/nm$ac_exeext ; then
      # Check to see if the nm accepts a BSD-compat flag.
      # Adding the `sed 1q' prevents false positives on HP-UX, which says:
      #   nm: unknown option "B" ignored
      if ($ac_dir/nm -B /dev/null 2>&1 | sed '1q'; exit 0) | egrep /dev/null >/dev/null; then
	ac_cv_path_NM="$ac_dir/nm -B"
	break
      elif ($ac_dir/nm -p /dev/null 2>&1 | sed '1q'; exit 0) | egrep /dev/null >/dev/null; then
	ac_cv_path_NM="$ac_dir/nm -p"
	break
      else
	ac_cv_path_NM=${ac_cv_path_NM="$ac_dir/nm"} # keep the first match, but
	continue # so that we can try to find one that supports BSD flags
      fi
    fi
  done
  IFS="$ac_save_ifs"
  test -z "$ac_cv_path_NM" && ac_cv_path_NM=nm
fi])
NM="$ac_cv_path_NM"
AC_MSG_RESULT([$NM])
])

# AC_CHECK_LIBM - check for math library
AC_DEFUN([AC_CHECK_LIBM],
[AC_REQUIRE([AC_CANONICAL_HOST])dnl
LIBM=
case "$lt_target" in
*-*-beos* | *-*-cygwin*)
  # These system don't have libm
  ;;
*-ncr-sysv4.3*)
  AC_CHECK_LIB(mw, _mwvalidcheckl, LIBM="-lmw")
  AC_CHECK_LIB(m, main, LIBM="$LIBM -lm")
  ;;
*)
  AC_CHECK_LIB(m, main, LIBM="-lm")
  ;;
esac
])

# AC_LIBLTDL_CONVENIENCE[(dir)] - sets LIBLTDL to the link flags for
# the libltdl convenience library, adds --enable-ltdl-convenience to
# the configure arguments.  Note that LIBLTDL is not AC_SUBSTed, nor
# is AC_CONFIG_SUBDIRS called.  If DIR is not provided, it is assumed
# to be `${top_builddir}/libltdl'.  Make sure you start DIR with
# '${top_builddir}/' (note the single quotes!) if your package is not
# flat, and, if you're not using automake, define top_builddir as
# appropriate in the Makefiles.
AC_DEFUN([AC_LIBLTDL_CONVENIENCE], [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
  case "$enable_ltdl_convenience" in
  no) AC_MSG_ERROR([this package needs a convenience libltdl]) ;;
  "") enable_ltdl_convenience=yes
      ac_configure_args="$ac_configure_args --enable-ltdl-convenience" ;;
  esac
  LIBLTDL=ifelse($#,1,$1,['${top_builddir}/libltdl'])/libltdlc.la
  INCLTDL=ifelse($#,1,-I$1,['-I${top_builddir}/libltdl'])
])

# AC_LIBLTDL_INSTALLABLE[(dir)] - sets LIBLTDL to the link flags for
# the libltdl installable library, and adds --enable-ltdl-install to
# the configure arguments.  Note that LIBLTDL is not AC_SUBSTed, nor
# is AC_CONFIG_SUBDIRS called.  If DIR is not provided, it is assumed
# to be `${top_builddir}/libltdl'.  Make sure you start DIR with
# '${top_builddir}/' (note the single quotes!) if your package is not
# flat, and, if you're not using automake, define top_builddir as
# appropriate in the Makefiles.
# In the future, this macro may have to be called after AC_PROG_LIBTOOL.
AC_DEFUN([AC_LIBLTDL_INSTALLABLE], [AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
  AC_CHECK_LIB(ltdl, main,
  [test x"$enable_ltdl_install" != xyes && enable_ltdl_install=no],
  [if test x"$enable_ltdl_install" = xno; then
     AC_MSG_WARN([libltdl not installed, but installation disabled])
   else
     enable_ltdl_install=yes
   fi
  ])
  if test x"$enable_ltdl_install" = x"yes"; then
    ac_configure_args="$ac_configure_args --enable-ltdl-install"
    LIBLTDL=ifelse($#,1,$1,['${top_builddir}/libltdl'])/libltdl.la
    INCLTDL=ifelse($#,1,-I$1,['-I${top_builddir}/libltdl'])
  else
    ac_configure_args="$ac_configure_args --enable-ltdl-install=no"
    LIBLTDL="-lltdl"
    INCLTDL=
  fi
])

dnl old names
AC_DEFUN([AM_PROG_LIBTOOL], [indir([AC_PROG_LIBTOOL])])dnl
AC_DEFUN([AM_ENABLE_SHARED], [indir([AC_ENABLE_SHARED], $@)])dnl
AC_DEFUN([AM_ENABLE_STATIC], [indir([AC_ENABLE_STATIC], $@)])dnl
AC_DEFUN([AM_DISABLE_SHARED], [indir([AC_DISABLE_SHARED], $@)])dnl
AC_DEFUN([AM_DISABLE_STATIC], [indir([AC_DISABLE_STATIC], $@)])dnl
AC_DEFUN([AM_PROG_LD], [indir([AC_PROG_LD])])dnl
AC_DEFUN([AM_PROG_NM], [indir([AC_PROG_NM])])dnl

dnl This is just to silence aclocal about the macro not being used
ifelse([AC_DISABLE_FAST_INSTALL])dnl

dnl bsd_sockets.m4--which socket libraries do we need? 
dnl Derrick Brashear
dnl from Zephyr
dnl $Id: bsd_sockets.m4,v 1.10 2005/04/26 19:14:07 shadow Exp $

dnl Hacked on by Rob Earhart to not just toss stuff in LIBS
dnl It now puts everything required for sockets into LIB_SOCKET

AC_DEFUN([CMU_SOCKETS], [
	save_LIBS="$LIBS"
	LIB_SOCKET=""
	AC_CHECK_FUNC(connect, :,
		AC_CHECK_LIB(nsl, gethostbyname,
			     LIB_SOCKET="-lnsl $LIB_SOCKET")
		AC_CHECK_LIB(socket, connect,
			     LIB_SOCKET="-lsocket $LIB_SOCKET")
	)
	LIBS="$LIB_SOCKET $save_LIBS"
	AC_CHECK_FUNC(res_search, :,
		LIBS="-lresolv $LIB_SOCKET $save_LIBS"
		AC_TRY_LINK([[
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#ifdef HAVE_ARPA_NAMESER_COMPAT_H
#include <arpa/nameser_compat.h>
#endif
#include <resolv.h>]],[[
const char host[12]="openafs.org";
u_char ans[1024];
res_search( host, C_IN, T_MX, (u_char *)&ans, sizeof(ans));
return 0;
]], LIB_SOCKET="-lresolv $LIB_SOCKET")
        )
	LIBS="$LIB_SOCKET $save_LIBS"
	AC_CHECK_FUNCS(dn_expand dns_lookup)
	LIBS="$save_LIBS"
	AC_SUBST(LIB_SOCKET)
	])

dnl Functions to check what database to use for libsasldb

dnl Berkeley DB specific checks first..

dnl Figure out what database type we're using
AC_DEFUN([SASL_DB_CHECK], [
cmu_save_LIBS="$LIBS"
AC_ARG_WITH(dblib, [  --with-dblib=DBLIB      set the DB library to use [berkeley] ],
  dblib=$withval,
  dblib=auto_detect)

CYRUS_BERKELEY_DB_OPTS()

SASL_DB_LIB=""

case "$dblib" in
dnl this is unbelievably painful due to confusion over what db-3 should be
dnl named.  arg.
  berkeley)
	CYRUS_BERKELEY_DB_CHK()
	CPPFLAGS="${CPPFLAGS} ${BDB_INCADD}"
	SASL_DB_INC=$BDB_INCADD
	SASL_DB_LIB="${BDB_LIBADD}"
	;;
  gdbm)
	AC_ARG_WITH(gdbm,[  --with-gdbm=PATH        use gdbm from PATH],
                    with_gdbm="${withval}")

        case "$with_gdbm" in
           ""|yes)
               AC_CHECK_HEADER(gdbm.h, [
			AC_CHECK_LIB(gdbm, gdbm_open, SASL_DB_LIB="-lgdbm",
                                           dblib="no")],
			dblib="no")
               ;;
           *)
               if test -d $with_gdbm; then
                 CPPFLAGS="${CPPFLAGS} -I${with_gdbm}/include"
                 LDFLAGS="${LDFLAGS} -L${with_gdbm}/lib"
                 SASL_DB_LIB="-lgdbm" 
               else
                 with_gdbm="no"
               fi
       esac
	;;
  ndbm)
	dnl We want to attempt to use -lndbm if we can, just in case
	dnl there's some version of it installed and overriding libc
	AC_CHECK_HEADER(ndbm.h, [
			AC_CHECK_LIB(ndbm, dbm_open, SASL_DB_LIB="-lndbm", [
				AC_CHECK_FUNC(dbm_open,,dblib="no")])],
				dblib="no")
	;;
  auto_detect)
        dnl How about berkeley db?
	CYRUS_BERKELEY_DB_CHK()
	if test "$dblib" = no; then
	  dnl How about ndbm?
	  AC_CHECK_HEADER(ndbm.h, [
		AC_CHECK_LIB(ndbm, dbm_open,
			     dblib="ndbm"; SASL_DB_LIB="-lndbm",
		   	     dblib="weird")],
		   dblib="no")
	  if test "$dblib" = "weird"; then
	    dnl Is ndbm in the standard library?
            AC_CHECK_FUNC(dbm_open, dblib="ndbm", dblib="no")
	  fi

	  if test "$dblib" = no; then
            dnl Can we use gdbm?
   	    AC_CHECK_HEADER(gdbm.h, [
		AC_CHECK_LIB(gdbm, gdbm_open, dblib="gdbm";
					     SASL_DB_LIB="-lgdbm", dblib="no")],
  			     dblib="no")
	  fi
	else
	  dnl we took Berkeley
	  CPPFLAGS="${CPPFLAGS} ${BDB_INCADD}"
	  SASL_DB_INC=$BDB_INCADD
          SASL_DB_LIB="${BDB_LIBADD}"
	fi
	;;
  none)
	;;
  no)
	;;
  *)
	AC_MSG_WARN([Bad DB library implementation specified;])
	AC_ERROR([Use either \"berkeley\", \"gdbm\", \"ndbm\" or \"none\"])
	dblib=no
	;;
esac
LIBS="$cmu_save_LIBS"

AC_MSG_CHECKING(DB library to use)
AC_MSG_RESULT($dblib)

SASL_DB_BACKEND="db_${dblib}.lo"
SASL_DB_BACKEND_STATIC="db_${dblib}.o allockey.o"
SASL_DB_BACKEND_STATIC_SRCS="../sasldb/db_${dblib}.c ../sasldb/allockey.c"
SASL_DB_UTILS="saslpasswd2 sasldblistusers2"
SASL_DB_MANS="saslpasswd2.8 sasldblistusers2.8"

case "$dblib" in
  gdbm) 
    SASL_MECHS="$SASL_MECHS libsasldb.la"
    AC_DEFINE(SASL_GDBM,[],[Use GDBM for SASLdb])
    ;;
  ndbm)
    SASL_MECHS="$SASL_MECHS libsasldb.la"
    AC_DEFINE(SASL_NDBM,[],[Use NDBM for SASLdb])
    ;;
  berkeley)
    SASL_MECHS="$SASL_MECHS libsasldb.la"
    AC_DEFINE(SASL_BERKELEYDB,[],[Use BerkeleyDB for SASLdb])
    ;;
  *)
    AC_MSG_WARN([Disabling SASL authentication database support])
    dnl note that we do not add libsasldb.la to SASL_MECHS, since it
    dnl will just fail to load anyway.
    SASL_DB_BACKEND="db_none.lo"
    SASL_DB_BACKEND_STATIC="db_none.o"
    SASL_DB_BACKEND_STATIC_SRCS="../sasldb/db_none.c"
    SASL_DB_UTILS=""
    SASL_DB_MANS=""
    SASL_DB_LIB=""
    ;;
esac

if test "$enable_static" = yes; then
    if test "$dblib" != "none"; then
      SASL_STATIC_SRCS="$SASL_STATIC_SRCS ../plugins/sasldb.c $SASL_DB_BACKEND_STATIC_SRCS"
      SASL_STATIC_OBJS="$SASL_STATIC_OBJS sasldb.o $SASL_DB_BACKEND_STATIC"
      AC_DEFINE(STATIC_SASLDB,[],[Link SASLdb Staticly])
    else
      SASL_STATIC_OBJS="$SASL_STATIC_OBJS $SASL_DB_BACKEND_STATIC"
      SASL_STATIC_SRCS="$SASL_STATIC_SRCS $SASL_DB_BACKEND_STATIC_SRCS"
    fi
fi

AC_SUBST(SASL_DB_UTILS)
AC_SUBST(SASL_DB_MANS)
AC_SUBST(SASL_DB_BACKEND)
AC_SUBST(SASL_DB_BACKEND_STATIC)
AC_SUBST(SASL_DB_INC)
AC_SUBST(SASL_DB_LIB)
])

dnl Figure out what database path we're using
AC_DEFUN([SASL_DB_PATH_CHECK], [
AC_ARG_WITH(dbpath, [  --with-dbpath=PATH      set the DB path to use [/etc/sasldb2] ],
  dbpath=$withval,
  dbpath=/etc/sasldb2)
AC_MSG_CHECKING(DB path to use)
AC_MSG_RESULT($dbpath)
AC_DEFINE_UNQUOTED(SASL_DB_PATH, "$dbpath", [Path to default SASLdb database])])

dnl $Id: berkdb.m4,v 1.20 2005/04/26 19:14:07 shadow Exp $

AC_DEFUN([CMU_DB_INC_WHERE1], [
saved_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$saved_CPPFLAGS -I$1"
AC_TRY_COMPILE([#include <db.h>],
[DB *db;
db_create(&db, NULL, 0);
db->open(db, "foo.db", NULL, DB_UNKNOWN, DB_RDONLY, 0644);],
ac_cv_found_db_inc=yes,
ac_cv_found_db_inc=no)
CPPFLAGS=$saved_CPPFLAGS
])

AC_DEFUN([CMU_DB_INC_WHERE], [
   for i in $1; do
      AC_MSG_CHECKING(for db headers in $i)
      CMU_DB_INC_WHERE1($i)
      CMU_TEST_INCPATH($i, db)
      if test "$ac_cv_found_db_inc" = "yes"; then
        ac_cv_db_where_inc=$i
        AC_MSG_RESULT(found)
        break
      else
        AC_MSG_RESULT(not found)
      fi
    done
])

#
# Test for lib files
#

AC_DEFUN([CMU_DB3_LIB_WHERE1], [
AC_REQUIRE([CMU_AFS])
AC_REQUIRE([CMU_KRB4])
saved_LIBS=$LIBS
  LIBS="$saved_LIBS -L$1 -ldb-3"
AC_TRY_LINK([#include <db.h>],
[db_env_create(NULL, 0);],
[ac_cv_found_db_3_lib=yes],
ac_cv_found_db_3_lib=no)
LIBS=$saved_LIBS
])
AC_DEFUN([CMU_DB4_LIB_WHERE1], [
AC_REQUIRE([CMU_AFS])
AC_REQUIRE([CMU_KRB4])
saved_LIBS=$LIBS
LIBS="$saved_LIBS -L$1 -ldb-4"
AC_TRY_LINK([#include <db.h>],
[db_env_create(NULL, 0);],
[ac_cv_found_db_4_lib=yes],
ac_cv_found_db_4_lib=no)
LIBS=$saved_LIBS
])

AC_DEFUN([CMU_DB_LIB_WHERE], [
   for i in $1; do
      AC_MSG_CHECKING(for db libraries in $i)
if test "$enable_db4" = "yes"; then
      CMU_DB4_LIB_WHERE1($i)
      CMU_TEST_LIBPATH($i, [db-4])
      ac_cv_found_db_lib=$ac_cv_found_db_4_lib
else
      CMU_DB3_LIB_WHERE1($i)
      CMU_TEST_LIBPATH($i, [db-3])
      ac_cv_found_db_lib=$ac_cv_found_db_3_lib
fi
      if test "$ac_cv_found_db_lib" = "yes" ; then
        ac_cv_db_where_lib=$i
        AC_MSG_RESULT(found)
        break
      else
        AC_MSG_RESULT(not found)
      fi
    done
])

AC_DEFUN([CMU_USE_DB], [
AC_REQUIRE([CMU_FIND_LIB_SUBDIR])
AC_ARG_WITH(db,
	[  --with-db=PREFIX      Compile with db support],
	[if test "X$with_db" = "X"; then
		with_db=yes
	fi])
AC_ARG_WITH(db-lib,
	[  --with-db-lib=dir     use db libraries in dir],
	[if test "$withval" = "yes" -o "$withval" = "no"; then
		AC_MSG_ERROR([No argument for --with-db-lib])
	fi])
AC_ARG_WITH(db-include,
	[  --with-db-include=dir use db headers in dir],
	[if test "$withval" = "yes" -o "$withval" = "no"; then
		AC_MSG_ERROR([No argument for --with-db-include])
	fi])
AC_ARG_ENABLE(db4,
	[  --enable-db4          use db 4.x libraries])
	
	if test "X$with_db" != "X"; then
	  if test "$with_db" != "yes"; then
	    ac_cv_db_where_lib=$with_db/$CMU_LIB_SUBDIR
	    ac_cv_db_where_inc=$with_db/include
	  fi
	fi

	if test "X$with_db_lib" != "X"; then
	  ac_cv_db_where_lib=$with_db_lib
	fi
	if test "X$ac_cv_db_where_lib" = "X"; then
	  CMU_DB_LIB_WHERE(/usr/athena/$CMU_LIB_SUBDIR /usr/$CMU_LIB_SUBDIR /usr/local/$CMU_LIB_SUBDIR)
	fi

	if test "X$with_db_include" != "X"; then
	  ac_cv_db_where_inc=$with_db_include
	fi
	if test "X$ac_cv_db_where_inc" = "X"; then
	  CMU_DB_INC_WHERE(/usr/athena/include /usr/local/include)
	fi

	AC_MSG_CHECKING(whether to include db)
	if test "X$ac_cv_db_where_lib" = "X" -o "X$ac_cv_db_where_inc" = "X"; then
	  ac_cv_found_db=no
	  AC_MSG_RESULT(no)
	else
	  ac_cv_found_db=yes
	  AC_MSG_RESULT(yes)
	  DB_INC_DIR=$ac_cv_db_where_inc
	  DB_LIB_DIR=$ac_cv_db_where_lib
	  DB_INC_FLAGS="-I${DB_INC_DIR}"
          if test "$enable_db4" = "yes"; then
	     DB_LIB_FLAGS="-L${DB_LIB_DIR} -ldb-4"
          else
	     DB_LIB_FLAGS="-L${DB_LIB_DIR} -ldb-3"
          fi
          dnl Do not force configure.in to put these in CFLAGS and LIBS unconditionally
          dnl Allow makefile substitutions....
          AC_SUBST(DB_INC_FLAGS)
          AC_SUBST(DB_LIB_FLAGS)
	  if test "X$RPATH" = "X"; then
		RPATH=""
	  fi
	  case "${host}" in
	    *-*-linux*)
	      if test "X$RPATH" = "X"; then
	        RPATH="-Wl,-rpath,${DB_LIB_DIR}"
	      else 
		RPATH="${RPATH}:${DB_LIB_DIR}"
	      fi
	      ;;
	    *-*-hpux*)
	      if test "X$RPATH" = "X"; then
	        RPATH="-Wl,+b${DB_LIB_DIR}"
	      else 
		RPATH="${RPATH}:${DB_LIB_DIR}"
	      fi
	      ;;
	    *-*-irix*)
	      if test "X$RPATH" = "X"; then
	        RPATH="-Wl,-rpath,${DB_LIB_DIR}"
	      else 
		RPATH="${RPATH}:${DB_LIB_DIR}"
	      fi
	      ;;
	    *-*-solaris2*)
	      if test "$ac_cv_prog_gcc" = yes; then
		if test "X$RPATH" = "X"; then
		  RPATH="-Wl,-R${DB_LIB_DIR}"
		else 
		  RPATH="${RPATH}:${DB_LIB_DIR}"
		fi
	      else
	        RPATH="${RPATH} -R${DB_LIB_DIR}"
	      fi
	      ;;
	  esac
	  AC_SUBST(RPATH)
	fi
	])



dnl ---- CUT HERE ---

dnl These are the Cyrus Berkeley DB macros.  In an ideal world these would be
dnl identical to the above.

dnl They are here so that they can be shared between Cyrus IMAPd
dnl and Cyrus SASL with relative ease.

dnl The big difference between this and the ones above is that we don't assume
dnl that we know the name of the library, and we try a lot of permutations
dnl instead.  We also assume that DB4 is acceptable.

dnl When we're done, there will be a BDB_LIBADD and a BDB_INCADD which should
dnl be used when necessary.  We should probably be smarter about our RPATH
dnl handling.

dnl Call these with BERKELEY_DB_CHK.

dnl We will also set $dblib to "berkeley" if we are successful, "no" otherwise.

dnl this is unbelievably painful due to confusion over what db-3 should be
dnl named and where the db-3 header file is located.  arg.
AC_DEFUN([CYRUS_BERKELEY_DB_CHK_LIB],
[
	BDB_SAVE_LDFLAGS=$LDFLAGS

	if test -d $with_bdb_lib; then
	    CMU_ADD_LIBPATH_TO($with_bdb_lib, LDFLAGS)
	    CMU_ADD_LIBPATH_TO($with_bdb_lib, BDB_LIBADD)
	else
	    BDB_LIBADD=""
	fi

	saved_LIBS=$LIBS
        for dbname in db-4.4 db4.4 db44 db-4.3 db4.3 db43 db-4.2 db4.2 db42 db-4.1 db4.1 db41 db-4.0 db4.0 db-4 db40 db4 db-3.3 db3.3 db33 db-3.2 db3.2 db32 db-3.1 db3.1 db31 db-3 db30 db3 db
          do
	    LIBS="$saved_LIBS -l$dbname"
	    AC_TRY_LINK([#include <db.h>],
	    [db_create(NULL, NULL, 0);],
	    BDB_LIBADD="$BDB_LIBADD -l$dbname"; dblib="berkeley"; dbname=db,
            dblib="no")
	    if test "$dblib" = "berkeley"; then break; fi
          done
        if test "$dblib" = "no"; then
	    LIBS="$saved_LIBS -ldb"
	    AC_TRY_LINK([#include <db.h>],
	    [db_open(NULL, 0, 0, 0, NULL, NULL, NULL);],
	    BDB_LIBADD="$BDB_LIBADD -ldb"; dblib="berkeley"; dbname=db,
            dblib="no")
        fi
	LIBS=$saved_LIBS

	LDFLAGS=$BDB_SAVE_LDFLAGS
])

AC_DEFUN([CYRUS_BERKELEY_DB_OPTS],
[
AC_ARG_WITH(bdb-libdir,
	[  --with-bdb-libdir=DIR   Berkeley DB lib files are in DIR],
	with_bdb_lib=$withval,
	[ test "${with_bdb_lib+set}" = set || with_bdb_lib=none])
AC_ARG_WITH(bdb-incdir,
	[  --with-bdb-incdir=DIR   Berkeley DB include files are in DIR],
	with_bdb_inc=$withval,
	[ test "${with_bdb_inc+set}" = set || with_bdb_inc=none ])
])

AC_DEFUN([CYRUS_BERKELEY_DB_CHK],
[
	AC_REQUIRE([CYRUS_BERKELEY_DB_OPTS])

	cmu_save_CPPFLAGS=$CPPFLAGS

	if test -d $with_bdb_inc; then
	    CPPFLAGS="$CPPFLAGS -I$with_bdb_inc"
	    BDB_INCADD="-I$with_bdb_inc"
	else
	    BDB_INCADD=""
	fi

	dnl Note that FreeBSD puts it in a wierd place
        dnl (but they should use with-bdb-incdir)
        AC_CHECK_HEADER(db.h,
                        [CYRUS_BERKELEY_DB_CHK_LIB()],
                        dblib="no")

	CPPFLAGS=$cmu_save_CPPFLAGS
])

dnl $Id: common.m4,v 1.13 2006/02/25 18:29:46 cg2v Exp $

AC_DEFUN([CMU_TEST_LIBPATH], [
changequote(<<, >>)
define(<<CMU_AC_CV_FOUND>>, translit(ac_cv_found_$2_lib, <<- *>>, <<__p>>))
changequote([, ])
if test "$CMU_AC_CV_FOUND" = "yes"; then
  if test \! -r "$1/lib$2.a" -a \! -r "$1/lib$2.so" -a \! -r "$1/lib$2.sl" -a \! -r "$1/lib$2.dylib"; then
    CMU_AC_CV_FOUND=no
  fi
fi
])

AC_DEFUN([CMU_TEST_INCPATH], [
changequote(<<, >>)
define(<<CMU_AC_CV_FOUND>>, translit(ac_cv_found_$2_inc, [ *], [_p]))
changequote([, ])
if test "$CMU_AC_CV_FOUND" = "yes"; then
  if test \! -r "$1/$2.h"; then
    CMU_AC_CV_FOUND=no
  fi
fi
])

dnl CMU_CHECK_HEADER_NOCACHE(HEADER-FILE, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN([CMU_CHECK_HEADER_NOCACHE],
[dnl Do the transliteration at runtime so arg 1 can be a shell variable.
ac_safe=`echo "$1" | sed 'y%./+-%__p_%'`
AC_MSG_CHECKING([for $1])
AC_TRY_CPP([#include <$1>], eval "ac_cv_header_$ac_safe=yes",
  eval "ac_cv_header_$ac_safe=no")
if eval "test \"`echo '$ac_cv_header_'$ac_safe`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
ifelse([$3], , , [$3
])dnl
fi
])

AC_DEFUN([CMU_FIND_LIB_SUBDIR],
[dnl
AC_ARG_WITH([lib-subdir], AC_HELP_STRING([--with-lib-subdir=DIR],[Find libraries in DIR instead of lib]))
AC_CHECK_SIZEOF(long)
AC_CACHE_CHECK([what directory libraries are found in], [ac_cv_cmu_lib_subdir],
[test "X$with_lib_subdir" = "Xyes" && with_lib_subdir=
test "X$with_lib_subdir" = "Xno" && with_lib_subdir=
if test "X$with_lib_subdir" = "X" ; then
  ac_cv_cmu_lib_subdir=lib
  if test $ac_cv_sizeof_long -eq 4 ; then
    test -d /usr/lib32 && ac_cv_cmu_lib_subdir=lib32
  fi
  if test $ac_cv_sizeof_long -eq 8 ; then
    test -d /usr/lib64 && ac_cv_cmu_lib_subdir=lib64
  fi
else
  ac_cv_cmu_lib_subdir=$with_lib_subdir
fi])
AC_SUBST(CMU_LIB_SUBDIR, $ac_cv_cmu_lib_subdir)
])

dnl afs.m4--AFS libraries, includes, and dependencies
dnl $Id: afs.m4,v 1.29 2005/04/26 19:14:07 shadow Exp $
dnl Chaskiel Grundman
dnl based on kerberos_v4.m4
dnl Derrick Brashear
dnl from KTH krb and Arla

AC_DEFUN([CMU_AFS_INC_WHERE1], [
cmu_save_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$cmu_save_CPPFLAGS -I$1"
AC_TRY_COMPILE([#include <afs/param.h>],
[#ifndef SYS_NAME
choke me
#endif
int foo;],
ac_cv_found_afs_inc=yes,
ac_cv_found_afs_inc=no)
CPPFLAGS=$cmu_save_CPPFLAGS
])

AC_DEFUN([CMU_AFS_LIB_WHERE1], [
save_LIBS="$LIBS"
save_LDFLAGS="$LDFLAGS"

LIBS="-lauth $1/afs/util.a $LIB_SOCKET $LIBS"
LDFLAGS="-L$1 -L$1/afs $LDFLAGS"
dnl suppress caching
AC_TRY_LINK([],[afsconf_Open();], ac_cv_found_afs_lib=yes, ac_cv_found_afs_lib=no)
LIBS="$save_LIBS"
LDFLAGS="$save_LDFLAGS"
])

AC_DEFUN([CMU_AFS_WHERE], [
AC_REQUIRE([CMU_FIND_LIB_SUBDIR])
   for i in $1; do
      AC_MSG_CHECKING(for AFS in $i)
      CMU_AFS_INC_WHERE1("$i/include")
      ac_cv_found_lwp_inc=$ac_cv_found_afs_inc
      CMU_TEST_INCPATH($i/include, lwp) 
      ac_cv_found_afs_inc=$ac_cv_found_lwp_inc
      if test "$ac_cv_found_afs_inc" = "yes"; then
        CMU_AFS_LIB_WHERE1("$i/$CMU_LIB_SUBDIR")
        if test "$ac_cv_found_afs_lib" = "yes"; then
          ac_cv_afs_where=$i
          AC_MSG_RESULT(found)
          break
        else
          AC_MSG_RESULT(not found)
        fi
      else
        AC_MSG_RESULT(not found)
      fi
    done
])

AC_DEFUN([CMU_AFS], [
AC_REQUIRE([CMU_FIND_LIB_SUBDIR])
AC_REQUIRE([CMU_SOCKETS])
AC_REQUIRE([CMU_LIBSSL])
AC_ARG_WITH(AFS,
	[  --with-afs=PREFIX      Compile with AFS support],
	[if test "X$with_AFS" = "X"; then
		with_AFS=yes
	fi])

	if test "X$with_AFS" != "X"; then
	  ac_cv_afs_where=$with_AFS
	fi
	if test "X$ac_cv_afs_where" = "X"; then
	  CMU_AFS_WHERE(/usr/afsws /usr/local /usr/athena /Library/OpenAFS/Tools)
	fi

	AC_MSG_CHECKING(whether to include AFS)
	if test "X$ac_cv_afs_where" = "Xno" -o "X$ac_cv_afs_where" = "X"; then
	  ac_cv_found_afs=no
	  AC_MSG_RESULT(no)
	else
	  ac_cv_found_afs=yes
	  AC_MSG_RESULT(yes)
	  AFS_INC_DIR="$ac_cv_afs_where/include"
	  AFS_LIB_DIR="$ac_cv_afs_where/$CMU_LIB_SUBDIR"
	  AFS_TOP_DIR="$ac_cv_afs_where"
	  AFS_INC_FLAGS="-I${AFS_INC_DIR}"
          AFS_LIB_FLAGS="-L${AFS_LIB_DIR} -L${AFS_LIB_DIR}/afs"
          cmu_save_LIBS="$LIBS"
          cmu_save_CPPFLAGS="$CPPFLAGS"
          CPPFLAGS="$CPPFLAGS ${AFS_INC_FLAGS}"
	  cmu_save_LDFLAGS="$LDFLAGS"
 	  LDFLAGS="$cmu_save_LDFLAGS ${AFS_LIB_FLAGS}"
                        
          AC_CHECK_HEADERS(afs/stds.h)

          AC_MSG_CHECKING([if libdes is needed])
          AC_TRY_LINK([],[des_quad_cksum();],AFS_DES_LIB="",AFS_DES_LIB="maybe")
          if test "X$AFS_DES_LIB" != "X"; then
              LIBS="$cmu_save_LIBS -ldes"
              AC_TRY_LINK([], [des_quad_cksum();],AFS_DES_LIB="yes")
              if test "X$AFS_DES_LIB" = "Xyes"; then
                  AC_MSG_RESULT([yes])
    	          AFS_LIBDES="-ldes"
    	          AFS_LIBDESA="${AFS_LIB_DIR}/libdes.a"
    	      else
   	          LIBS="$cmu_save_LIBS $LIBSSL_LIB_FLAGS"
 	          AC_TRY_LINK([],
	          [des_quad_cksum();],AFS_DES_LIB="libcrypto")
	          if test "X$AFS_DES_LIB" = "Xlibcrypto"; then
	              AC_MSG_RESULT([libcrypto])
		      AFS_LIBDES="$LIBSSL_LIB_FLAGS"
	              AFS_LIBDESA="$LIBSSL_LIB_FLAGS"
    	          else
   	              LIBS="$cmu_save_LIBS -L$LIBSSL_LIB_DIR -ldescompat $LIBSSL_LIB_FLAGS"
 	              AC_TRY_LINK([],
	              [des_quad_cksum();],AFS_DES_LIB="libcrypto+descompat")
	              if test "X$AFS_DES_LIB" = "Xlibcrypto+descompat"; then
	                  AC_MSG_RESULT([libcrypto+descompat])
		          AFS_LIBDES="-L$LIBSSL_LIB_DIR -ldescompat $LIBSSL_LIB_FLAGS"
	                  AFS_LIBDESA="-L$LIBSSL_LIB_DIR -ldescompat $LIBSSL_LIB_FLAGS"
	              else
         	          AC_MSG_RESULT([unknown])
	                  AC_MSG_ERROR([Could not use -ldes])
	              fi 
	          fi 
	      fi 
	  else
             AC_MSG_RESULT([no])
          fi


	  AFS_CLIENT_LIBS_STATIC="${AFS_LIB_DIR}/afs/libvolser.a ${AFS_LIB_DIR}/afs/libvldb.a ${AFS_LIB_DIR}/afs/libkauth.a ${AFS_LIB_DIR}/afs/libprot.a ${AFS_LIB_DIR}/libubik.a ${AFS_LIB_DIR}/afs/libauth.a ${AFS_LIB_DIR}/librxkad.a ${AFS_LIB_DIR}/librx.a ${AFS_LIB_DIR}/afs/libsys.a ${AFS_LIB_DIR}/librx.a ${AFS_LIB_DIR}/liblwp.a ${AFS_LIBDESA} ${AFS_LIB_DIR}/afs/libcmd.a ${AFS_LIB_DIR}/afs/libcom_err.a ${AFS_LIB_DIR}/afs/util.a"
          AFS_KTC_LIBS_STATIC="${AFS_LIB_DIR}/afs/libauth.a ${AFS_LIB_DIR}/afs/libsys.a ${AFS_LIB_DIR}/librx.a ${AFS_LIB_DIR}/liblwp.a ${AFS_LIBDESA} ${AFS_LIB_DIR}/afs/libcom_err.a ${AFS_LIB_DIR}/afs/util.a"
	  AFS_CLIENT_LIBS="-lvolser -lvldb -lkauth -lprot -lubik -lauth -lrxkad -lrx ${AFS_LIB_DIR}/afs/libsys.a -lrx -llwp ${AFS_LIBDES} -lcmd -lcom_err ${AFS_LIB_DIR}/afs/util.a"
	  AFS_RX_LIBS="-lauth -lrxkad -lrx ${AFS_LIB_DIR}/afs/libsys.a -lrx -llwp ${AFS_LIBDES} -lcmd -lcom_err ${AFS_LIB_DIR}/afs/util.a"
          AFS_KTC_LIBS="-lauth ${AFS_LIB_DIR}/afs/libsys.a -lrx -llwp ${AFS_LIBDES} -lcom_err ${AFS_LIB_DIR}/afs/util.a"

          LIBS="$cmu_save_LIBS $AFS_CLIENT_LIBS ${LIB_SOCKET}"
          AC_CHECK_FUNC(des_pcbc_init)
          if test "X$ac_cv_func_des_pcbc_init" != "Xyes"; then
           AC_CHECK_LIB(descompat, des_pcbc_init, AFS_DESCOMPAT_LIB="-ldescompat")
           if test "X$AFS_DESCOMPAT_LIB" != "X" ; then
                AFS_CLIENT_LIBS_STATIC="$AFS_CLIENT_LIBS_STATIC $AFS_DESCOMPAT_LIB"
                AFS_KTC_LIBS_STATIC="$AFS_KTC_LIBS_STATIC $AFS_DESCOMPAT_LIB"
                AFS_CLIENT_LIBS="$AFS_CLIENT_LIBS $AFS_DESCOMPAT_LIB"
                AFS_KTC_LIBS="$AFS_KTC_LIBS $AFS_DESCOMPAT_LIB"
           else

           AC_MSG_CHECKING([if rxkad needs des_pcbc_init])
           AC_TRY_LINK(,[tkt_DecodeTicket();],RXKAD_PROBLEM=no,RXKAD_PROBLEM=maybe)
            if test "$RXKAD_PROBLEM" = "maybe"; then
              AC_TRY_LINK([int des_pcbc_init() { return 0;}],
              [tkt_DecodeTicket();],RXKAD_PROBLEM=yes,RXKAD_PROBLEM=error)
              if test "$RXKAD_PROBLEM" = "yes"; then
                    AC_MSG_RESULT([yes])
                    AC_MSG_ERROR([cannot use rxkad])
              else
                    AC_MSG_RESULT([unknown])        
                    AC_MSG_ERROR([Unknown error testing rxkad])
              fi
            else
              AC_MSG_RESULT([no])
            fi
           fi
          fi

          LIBS="$cmu_save_LIBS"
          AC_CHECK_FUNC(flock)
          LIBS="$cmu_save_LIBS ${AFS_CLIENT_LIBS} ${LIB_SOCKET}"
          if test "X$ac_cv_func_flock" != "Xyes"; then
             AC_MSG_CHECKING([if AFS needs flock])
             AC_TRY_LINK([#include <afs/param.h>
#ifdef HAVE_AFS_STDS_H
#include <afs/stds.h>
#endif
#include <ubik.h>
#include <afs/cellconfig.h>
#include <afs/auth.h>
#include <afs/volser.h>
struct ubik_client * cstruct;
int sigvec() {return 0;}
extern int UV_SetSecurity();],
             [vsu_ClientInit(1,"","",0,
                             &cstruct,UV_SetSecurity)],
             AFS_FLOCK=no,AFS_FLOCK=yes)
             if test $AFS_FLOCK = "no"; then
                AC_MSG_RESULT([no])
             else
               AC_MSG_RESULT([yes])
               LDFLAGS="$LDFLAGS -L/usr/ucblib"
               AC_CHECK_LIB(ucb, flock,:, [AC_CHECK_LIB(BSD, flock)])
             fi
          fi
          LIBS="$cmu_save_LIBS"
          AC_CHECK_FUNC(sigvec)
          LIBS="$cmu_save_LIBS ${AFS_CLIENT_LIBS} ${LIB_SOCKET}"
          if test "X$ac_cv_func_sigvec" != "Xyes"; then
             AC_MSG_CHECKING([if AFS needs sigvec])
             AC_TRY_LINK([#include <afs/param.h>
#ifdef HAVE_AFS_STDS_H
#include <afs/stds.h>
#endif
#include <ubik.h>
#include <afs/cellconfig.h>
#include <afs/auth.h>
#include <afs/volser.h>
struct ubik_client * cstruct;
int flock() {return 0;}
extern int UV_SetSecurity();],
             [vsu_ClientInit(1,"","",0,
                             &cstruct,UV_SetSecurity)],
             AFS_SIGVEC=no,AFS_SIGVEC=yes)
             if test $AFS_SIGVEC = "no"; then
                AC_MSG_RESULT([no])
             else
               AC_MSG_RESULT([yes])
               LDFLAGS="$LDFLAGS -L/usr/ucblib"
               AC_CHECK_LIB(ucb, sigvec,:,[AC_CHECK_LIB(BSD, sigvec)])
             fi
          fi
          if test "$ac_cv_lib_ucb_flock" = "yes" -o "$ac_cv_lib_ucb_sigvec" = "yes"; then
             AFS_LIB_FLAGS="${AFS_LIB_FLAGS} -L/usr/ucblib -R/usr/ucblib"
          fi
          if test "$ac_cv_lib_ucb_flock" = "yes" -o "$ac_cv_lib_ucb_sigvec" = "yes"; then
             AFS_BSD_LIB="-lucb"
          elif test "$ac_cv_lib_BSD_flock" = "yes" -o "$ac_cv_lib_BSD_sigvec" = "yes"; then
             AFS_BSD_LIB="-lBSD"
          fi
          if test "X$AFS_BSD_LIB" != "X" ; then
                AFS_CLIENT_LIBS_STATIC="$AFS_CLIENT_LIBS_STATIC $AFS_BSD_LIB"
                AFS_KTC_LIBS_STATIC="$AFS_KTC_LIBS_STATIC $AFS_BSD_LIB"
                AFS_CLIENT_LIBS="$AFS_CLIENT_LIBS $AFS_BSD_LIB"
                AFS_RX_LIBS="$AFS_CLIENT_LIBS $AFS_BSD_LIB"
                AFS_KTC_LIBS="$AFS_KTC_LIBS $AFS_BSD_LIB"
          fi

          AC_MSG_CHECKING([if libaudit is needed])
	  AFS_LIBAUDIT=""
          LIBS="$cmu_save_LIBS $AFS_CLIENT_LIBS ${LIB_SOCKET}"
          AC_TRY_LINK([#include <afs/param.h>
#ifdef HAVE_AFS_STDS_H
#include <afs/stds.h>
#endif
#include <afs/cellconfig.h>
#include <afs/auth.h>],
          [afsconf_SuperUser();],AFS_AUDIT_LIB="",AFS_AUDIT_LIB="maybe")
          if test "X$AFS_AUDIT_LIB" != "X"; then
          LIBS="$cmu_save_LIBS -lvolser -lvldb -lkauth -lprot -lubik -lauth -laudit -lrxkad -lrx ${AFS_LIB_DIR}/afs/libsys.a -lrx -llwp ${AFS_LIBDES} -lcmd -lcom_err ${AFS_LIB_DIR}/afs/util.a $AFS_BSD_LIB $AFS_DESCOMPAT_LIB $LIB_SOCKET"
             AC_TRY_LINK([#include <afs/param.h>
#ifdef HAVE_AFS_STDS_H
#include <afs/stds.h>
#endif
#include <afs/cellconfig.h>
#include <afs/auth.h>],
             [afsconf_SuperUser();],AFS_AUDIT_LIB="yes")
             if test "X$AFS_AUDIT_LIB" = "Xyes"; then
                 AC_MSG_RESULT([yes])
	         AFS_LIBAUDIT="-laudit"
	         AFS_CLIENT_LIBS_STATIC="${AFS_LIB_DIR}/afs/libvolser.a ${AFS_LIB_DIR}/afs/libvldb.a ${AFS_LIB_DIR}/afs/libkauth.a ${AFS_LIB_DIR}/afs/libprot.a ${AFS_LIB_DIR}/libubik.a ${AFS_LIB_DIR}/afs/libauth.a ${AFS_LIB_DIR}/afs/libaudit.a ${AFS_LIB_DIR}/librxkad.a ${AFS_LIB_DIR}/librx.a ${AFS_LIB_DIR}/afs/libsys.a ${AFS_LIB_DIR}/librx.a ${AFS_LIB_DIR}/liblwp.a ${AFS_LIBDESA} ${AFS_LIB_DIR}/afs/libcmd.a ${AFS_LIB_DIR}/afs/libcom_err.a ${AFS_LIB_DIR}/afs/util.a"
                 AFS_CLIENT_LIBS="-lvolser -lvldb -lkauth -lprot -lubik -lauth -laudit -lrxkad -lrx ${AFS_LIB_DIR}/afs/libsys.a -lrx -llwp ${AFS_LIBDES} -lcmd -lcom_err ${AFS_LIB_DIR}/afs/util.a $AFS_BSD_LIB $AFS_DESCOMPAT_LIB"
                 AFS_RX_LIBS="-lauth -laudit -lrxkad -lrx ${AFS_LIB_DIR}/afs/libsys.a -lrx -llwp ${AFS_LIBDES} -lcmd -lcom_err ${AFS_LIB_DIR}/afs/util.a $AFS_BSD_LIB $AFS_DESCOMPAT_LIB"
             else
                 AC_MSG_RESULT([unknown])
                 AC_MSG_ERROR([Could not use -lauth while testing for -laudit])
             fi 
          else
             AC_MSG_RESULT([no])
          fi

	  AC_CHECK_FUNCS(VL_ProbeServer)
          AC_MSG_CHECKING([if new-style afs_ integer types are defined])
          AC_CACHE_VAL(ac_cv_afs_int32,
dnl The next few lines contain a quoted argument to egrep
dnl It is critical that there be no leading or trailing whitespace
dnl or newlines
[AC_EGREP_CPP(dnl
changequote(<<,>>)dnl
<<(^|[^a-zA-Z_0-9])afs_int32[^a-zA-Z_0-9]>>dnl
changequote([,]), [#include <afs/param.h>
#ifdef HAVE_AFS_STDS_H
#include <afs/stds.h>
#endif],
ac_cv_afs_int32=yes, ac_cv_afs_int32=no)])
          AC_MSG_RESULT($ac_cv_afs_int32)
          if test $ac_cv_afs_int32 = yes ; then
            AC_DEFINE(HAVE_AFS_INT32,, [AFS provides new "unambiguous" type names])
          else
            AC_DEFINE(afs_int16, int16, [it's a type definition])
            AC_DEFINE(afs_int32, int32, [it's a type definition])
            AC_DEFINE(afs_uint16, u_int16, [it's a type definition])
            AC_DEFINE(afs_uint32, u_int32, [it's a type definition])
          fi

          CPPFLAGS="${cmu_save_CPPFLAGS}"
          LDFLAGS="${cmu_save_LDFLAGS}"
          LIBS="${cmu_save_LIBS}"
	  AC_DEFINE(AFS_ENV,, [Use AFS. (find what needs this and nuke it)])
          AC_DEFINE(AFS,, [Use AFS. (find what needs this and nuke it)])
          AC_SUBST(AFS_CLIENT_LIBS_STATIC)
          AC_SUBST(AFS_KTC_LIBS_STATIC)
          AC_SUBST(AFS_CLIENT_LIBS)
          AC_SUBST(AFS_RX_LIBS)
          AC_SUBST(AFS_KTC_LIBS)
          AC_SUBST(AFS_INC_FLAGS)
          AC_SUBST(AFS_LIB_FLAGS)
	  AC_SUBST(AFS_TOP_DIR)
	  AC_SUBST(AFS_LIBAUDIT)
	  AC_SUBST(AFS_LIBDES)
          AC_SUBST(AFS_LIBDESA)
       	fi
	])

AC_DEFUN([CMU_NEEDS_AFS],
[AC_REQUIRE([CMU_AFS])
if test "$ac_cv_found_afs" != "yes"; then
        AC_ERROR([Cannot continue without AFS])
fi])

dnl libssl.m4--Ssl libraries and includes
dnl Derrick Brashear
dnl from KTH kafs and Arla
dnl $Id: libssl.m4,v 1.10 2005/04/26 19:14:08 shadow Exp $

AC_DEFUN([CMU_LIBSSL_INC_WHERE1], [
saved_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$saved_CPPFLAGS -I$1"
CMU_CHECK_HEADER_NOCACHE(openssl/ssl.h,
ac_cv_found_libssl_inc=yes,
ac_cv_found_libssl_inc=no)
CPPFLAGS=$saved_CPPFLAGS
])

AC_DEFUN([CMU_LIBSSL_INC_WHERE], [
   for i in $1; do
      AC_MSG_CHECKING(for libssl headers in $i)
      CMU_LIBSSL_INC_WHERE1($i)
      CMU_TEST_INCPATH($i, ssl)
      if test "$ac_cv_found_libssl_inc" = "yes"; then
        ac_cv_libssl_where_inc=$i
        AC_MSG_RESULT(found)
        break
      else
        AC_MSG_RESULT(not found)
      fi
    done
])

AC_DEFUN([CMU_LIBSSL_LIB_WHERE1], [
saved_LIBS=$LIBS
LIBS="$saved_LIBS -L$1 -lssl -lcrypto $LIB_SOCKET"
AC_TRY_LINK(,
[SSL_write();],
[ac_cv_found_ssl_lib=yes],
ac_cv_found_ssl_lib=no)
LIBS=$saved_LIBS
])

AC_DEFUN([CMU_LIBSSL_LIB_WHERE], [
   for i in $1; do
      AC_MSG_CHECKING(for libssl libraries in $i)
      CMU_LIBSSL_LIB_WHERE1($i)
      dnl deal with false positives from implicit link paths
      CMU_TEST_LIBPATH($i, ssl)
      if test "$ac_cv_found_ssl_lib" = "yes" ; then
        ac_cv_libssl_where_lib=$i
        AC_MSG_RESULT(found)
        break
      else
        AC_MSG_RESULT(not found)
      fi
    done
])

AC_DEFUN([CMU_LIBSSL], [
AC_REQUIRE([CMU_FIND_LIB_SUBDIR])
AC_REQUIRE([CMU_SOCKETS])
AC_ARG_WITH(libssl,
	[  --with-libssl=PREFIX      Compile with Libssl support],
	[if test "X$with_libssl" = "X"; then
		with_libssl=yes
	fi])
AC_ARG_WITH(libssl-lib,
	[  --with-libssl-lib=dir     use libssl libraries in dir],
	[if test "$withval" = "yes" -o "$withval" = "no"; then
		AC_MSG_ERROR([No argument for --with-libssl-lib])
	fi])
AC_ARG_WITH(libssl-include,
	[  --with-libssl-include=dir use libssl headers in dir],
	[if test "$withval" = "yes" -o "$withval" = "no"; then
		AC_MSG_ERROR([No argument for --with-libssl-include])
	fi])

	if test "X$with_libssl" != "X"; then
	  if test "$with_libssl" != "yes" -a "$with_libssl" != no; then
	    ac_cv_libssl_where_lib=$with_libssl/$CMU_LIB_SUBDIR
	    ac_cv_libssl_where_inc=$with_libssl/include
	  fi
	fi

	if test "$with_libssl" != "no"; then 
	  if test "X$with_libssl_lib" != "X"; then
	    ac_cv_libssl_where_lib=$with_libssl_lib
	  fi
	  if test "X$ac_cv_libssl_where_lib" = "X"; then
	    CMU_LIBSSL_LIB_WHERE(/usr/local/$CMU_LIB_SUBDIR/openssl /usr/$CMU_LIB_SUBDIR/openssl /usr/local/$CMU_LIB_SUBDIR /usr/$CMU_LIB_SUBDIR)
	  fi

	  if test "X$with_libssl_include" != "X"; then
	    ac_cv_libssl_where_inc=$with_libssl_include
	  fi
	  if test "X$ac_cv_libssl_where_inc" = "X"; then
	    CMU_LIBSSL_INC_WHERE(/usr/local/include /usr/include)
	  fi
	fi

	AC_MSG_CHECKING(whether to include libssl)
	if test "X$ac_cv_libssl_where_lib" = "X" -a "X$ac_cv_libssl_where_inc" = "X"; then
	  ac_cv_found_libssl=no
	  AC_MSG_RESULT(no)
	else
	  ac_cv_found_libssl=yes
	  AC_MSG_RESULT(yes)
	  LIBSSL_INC_DIR=$ac_cv_libssl_where_inc
	  LIBSSL_LIB_DIR=$ac_cv_libssl_where_lib
	  LIBSSL_INC_FLAGS="-I${LIBSSL_INC_DIR}"
	  LIBSSL_LIB_FLAGS="-L${LIBSSL_LIB_DIR} -lssl -lcrypto"
	  if test "X$RPATH" = "X"; then
		RPATH=""
	  fi
	  case "${host}" in
	    *-*-linux*)
	      if test "X$RPATH" = "X"; then
	        RPATH="-Wl,-rpath,${LIBSSL_LIB_DIR}"
	      else 
 		RPATH="${RPATH}:${LIBSSL_LIB_DIR}"
	      fi
	      ;;
	    *-*-hpux*)
	      if test "X$RPATH" = "X"; then
	        RPATH="-Wl,+b${LIBSSL_LIB_DIR}"
	      else 
		RPATH="${RPATH}:${LIBSSL_LIB_DIR}"
	      fi
	      ;;
	    *-*-irix*)
	      if test "X$RPATH" = "X"; then
	        RPATH="-Wl,-rpath,${LIBSSL_LIB_DIR}"
	      else 
		RPATH="${RPATH}:${LIBSSL_LIB_DIR}"
	      fi
	      ;;
	    *-*-solaris2*)
	      if test "$ac_cv_prog_gcc" = yes; then
		if test "X$RPATH" = "X"; then
		  RPATH="-Wl,-R${LIBSSL_LIB_DIR}"
		else 
		  RPATH="${RPATH}:${LIBSSL_LIB_DIR}"
		fi
	      else
	        RPATH="${RPATH} -R${LIBSSL_LIB_DIR}"
	      fi
	      ;;
	  esac
	  AC_SUBST(RPATH)
	fi
	AC_SUBST(LIBSSL_INC_DIR)
	AC_SUBST(LIBSSL_LIB_DIR)
	AC_SUBST(LIBSSL_INC_FLAGS)
	AC_SUBST(LIBSSL_LIB_FLAGS)
	])


dnl kerberos_v4.m4--Kerberos 4 libraries and includes
dnl Derrick Brashear
dnl from KTH krb and Arla
dnl $Id: kerberos_v4.m4,v 1.28 2005/04/26 19:14:08 shadow Exp $

AC_DEFUN([CMU_KRB_SENDAUTH_PROTO], [
AC_MSG_CHECKING(for krb_sendauth prototype)
AC_TRY_COMPILE(
[#include <krb.h>
int krb_sendauth (long options, int fd, KTEXT ktext, char *service,
                  char *inst, char *realm, u_long checksum,
                  MSG_DAT *msg_data, CREDENTIALS *cred,
                  Key_schedule schedule, struct sockaddr_in *laddr,
                  struct sockaddr_in *faddr, char *version);],
[int foo = krb_sendauth(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0); ],
ac_cv_krb_sendauth_proto=no,
ac_cv_krb_sendauth_proto=yes)
AC_MSG_RESULT($ac_cv_krb_sendauth_proto)
if test "$ac_cv_krb_sendauth_proto" = yes; then
        AC_DEFINE(HAVE_KRB_SENDAUTH_PROTO)dnl
fi
AC_MSG_RESULT($ac_cv_krb_sendauth_proto)
])

AC_DEFUN([CMU_KRB_SET_KEY_PROTO], [
AC_MSG_CHECKING(for krb_set_key prototype)
AC_CACHE_VAL(ac_cv_krb_set_key_proto, [
cmu_save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="${CPPFLAGS} ${KRB_INC_FLAGS}"
AC_TRY_COMPILE(
[#include <krb.h>
int krb_set_key(char *key, int cvt);],
[int foo = krb_set_key(0, 0);],
ac_cv_krb_set_key_proto=no,
ac_cv_krb_set_key_proto=yes)
])
CPPFLAGS="${cmu_save_CPPFLAGS}"
if test "$ac_cv_krb_set_key_proto" = yes; then
	AC_DEFINE(HAVE_KRB_SET_KEY_PROTO)dnl
fi
AC_MSG_RESULT($ac_cv_krb_set_key_proto)
])

AC_DEFUN([CMU_KRB4_32_DEFN], [
AC_MSG_CHECKING(for KRB4_32 definition)
AC_CACHE_VAL(ac_cv_krb4_32_defn, [
cmu_save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="${CPPFLAGS} ${KRB_INC_FLAGS}"
AC_TRY_COMPILE(
[#include <krb.h>
],
[KRB4_32 foo = 1;],
ac_cv_krb4_32_defn=yes,
ac_cv_krb4_32_defn=no)
])
CPPFLAGS="${cmu_save_CPPFLAGS}"
if test "$ac_cv_krb4_32_defn" = yes; then
	AC_DEFINE(HAVE_KRB4_32_DEFINE)dnl
fi
AC_MSG_RESULT($ac_cv_krb4_32_defn)
])

AC_DEFUN([CMU_KRB_RD_REQ_PROTO], [
AC_MSG_CHECKING(for krb_rd_req prototype)
AC_CACHE_VAL(ac_cv_krb_rd_req_proto, [
cmu_save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="${CPPFLAGS} ${KRB_INC_FLAGS}"
AC_TRY_COMPILE(
[#include <krb.h>
int krb_rd_req(KTEXT authent, char *service, char *instance,
unsigned KRB_INT32 from_addr, AUTH_DAT *ad, char *fn);],
[int foo = krb_rd_req(0,0,0,0,0,0);],
ac_cv_krb_rd_req_proto=no,
ac_cv_krb_rd_req_proto=yes)
])
CPPFLAGS="${cmu_save_CPPFLAGS}"
if test "$ac_cv_krb_rd_req_proto" = yes; then
	AC_DEFINE(HAVE_KRB_RD_REQ_PROTO)dnl
fi
AC_MSG_RESULT($ac_cv_krb_rd_req_proto)
])

AC_DEFUN([CMU_KRB_INC_WHERE1], [
saved_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$saved_CPPFLAGS -I$1"
AC_TRY_COMPILE([#include <krb.h>],
[struct ktext foo;],
ac_cv_found_krb_inc=yes,
ac_cv_found_krb_inc=no)
if test "$ac_cv_found_krb_inc" = "no"; then
  CPPFLAGS="$saved_CPPFLAGS -I$1 -I$1/kerberosIV"
  AC_TRY_COMPILE([#include <krb.h>],
  [struct ktext foo;],
  [ac_cv_found_krb_inc=yes],
  ac_cv_found_krb_inc=no)
fi
CPPFLAGS=$saved_CPPFLAGS
])

AC_DEFUN([CMU_KRB_INC_WHERE], [
   for i in $1; do
      AC_MSG_CHECKING(for kerberos headers in $i)
      CMU_KRB_INC_WHERE1($i)
      CMU_TEST_INCPATH($i, krb)
      if test "$ac_cv_found_krb_inc" = "yes"; then
        ac_cv_krb_where_inc=$i
        AC_MSG_RESULT(found)
        break
      else
        AC_MSG_RESULT(not found)
      fi
    done
])

#
# Test for kerberos lib files
#

AC_DEFUN([CMU_KRB_LIB_WHERE1], [
saved_LIBS=$LIBS
LIBS="$saved_LIBS -L$1 -lkrb ${KRB_LIBDES}"
AC_TRY_LINK(,
[dest_tkt();],
[ac_cv_found_krb_lib=yes],
ac_cv_found_krb_lib=no)
LIBS=$saved_LIBS
])

AC_DEFUN([CMU_KRB_LIB_WHERE], [
   for i in $1; do
      AC_MSG_CHECKING(for kerberos libraries in $i)
      CMU_KRB_LIB_WHERE1($i)
      dnl deal with false positives from implicit link paths
      CMU_TEST_LIBPATH($i, krb)
      if test "$ac_cv_found_krb_lib" = "yes" ; then
        ac_cv_krb_where_lib=$i
        AC_MSG_RESULT(found)
        break
      else
        AC_MSG_RESULT(not found)
      fi
    done
])

AC_DEFUN([CMU_KRB4], [
AC_REQUIRE([CMU_FIND_LIB_SUBDIR])
AC_REQUIRE([CMU_SOCKETS])
AC_REQUIRE([CMU_LIBSSL])
AC_ARG_WITH(krb4,
	[  --with-krb4=PREFIX      Compile with Kerberos 4 support],
	[if test "X$with_krb4" = "X"; then
		with_krb4=yes
	fi])
AC_ARG_WITH(krb4-lib,
	[  --with-krb4-lib=dir     use kerberos 4 libraries in dir],
	[if test "$withval" = "yes" -o "$withval" = "no"; then
		AC_MSG_ERROR([No argument for --with-krb4-lib])
	fi])
AC_ARG_WITH(krb4-include,
	[  --with-krb4-include=dir use kerberos 4 headers in dir],
	[if test "$withval" = "yes" -o "$withval" = "no"; then
		AC_MSG_ERROR([No argument for --with-krb4-include])
	fi])

	if test "X$with_krb4" != "X"; then
	  if test "$with_krb4" != "yes" -a "$with_krb4" != "no"; then
	    ac_cv_krb_where_lib=$with_krb4/$CMU_LIB_SUBDIR
	    ac_cv_krb_where_inc=$with_krb4/include
	  fi
	fi
       
	if test "$with_krb4" != "no"; then
	  if test "X$with_krb4_lib" != "X"; then
	    ac_cv_krb_where_lib=$with_krb4_lib
	  fi
	  if test "X$with_krb4_include" != "X"; then
	    ac_cv_krb_where_inc=$with_krb4_include
	  fi
	  if test "X$ac_cv_krb_where_inc" = "X"; then
	    CMU_KRB_INC_WHERE(/usr/athena/include /usr/include/kerberosIV /usr/local/include /usr/include/kerberos)
	  fi

          AC_MSG_CHECKING([if libdes is needed])
          AC_TRY_LINK([],[des_quad_cksum();],KRB_DES_LIB="",KRB_DES_LIB="maybe")
          if test "X$KRB_DES_LIB" != "X"; then
              LIBS="$cmu_save_LIBS -ldes"
              AC_TRY_LINK([], [des_quad_cksum();],KRB_DES_LIB="yes")
              if test "X$KRB_DES_LIB" = "Xyes"; then
                  AC_MSG_RESULT([yes])
                  KRB_LIBDES="-ldes"
                  KRB_LIBDESA='$(KRB_LIB_DIR)/libdes.a'
              else
                  LIBS="$cmu_save_LIBS $LIBSSL_LIB_FLAGS"
                  AC_TRY_LINK([],
                  [des_quad_cksum();],KRB_DES_LIB="libcrypto")
                  if test "X$KRB_DES_LIB" = "Xlibcrypto"; then
                      AC_MSG_RESULT([libcrypto])
                      KRB_LIBDES="$LIBSSL_LIB_FLAGS"
                      KRB_LIBDESA="$LIBSSL_LIB_FLAGS"
                  else
                      LIBS="$cmu_save_LIBS -L$LIBSSL_LIB_DIR -ldescompat $LIBSSL_LIB_FLAGS"
                      AC_TRY_LINK([],
                      [des_quad_cksum();],KRB_DES_LIB="libcrypto+descompat")
                      if test "X$KRB_DES_LIB" = "Xlibcrypto+descompat"; then
                          AC_MSG_RESULT([libcrypto+descompat])
                          KRB_LIBDES="-L$LIBSSL_LIB_DIR -ldescompat $LIBSSL_LIB_FLAGS"
                          KRB_LIBDESA="-L$LIBSSL_LIB_DIR -ldescompat $LIBSSL_LIB_FLAGS"
                      else
                          AC_MSG_RESULT([unknown])
                          AC_MSG_ERROR([Could not use -ldes])
                      fi 
                  fi 
              fi 
          else
             AC_MSG_RESULT([no])
          fi
          if test "X$ac_cv_krb_where_lib" = "X"; then
            CMU_KRB_LIB_WHERE(/usr/athena/$CMU_LIB_SUBDIR /usr/local/$CMU_LIB_SUBDIR /usr/$CMU_LIB_SUBDIR)
          fi
	fi
	  LIBS="${cmu_save_LIBS}"


	AC_MSG_CHECKING([whether to include kerberos 4])
	if test "X$ac_cv_krb_where_lib" = "X" -o "X$ac_cv_krb_where_inc" = "X"; then
	  ac_cv_found_krb=no
	  AC_MSG_RESULT(no)
	else
	  ac_cv_found_krb=yes
	  AC_MSG_RESULT(yes)
	  KRB_INC_DIR=$ac_cv_krb_where_inc
	  KRB_LIB_DIR=$ac_cv_krb_where_lib
	  KRB_INC_FLAGS="-I${KRB_INC_DIR}"
	  KRB_LIB_FLAGS="-L${KRB_LIB_DIR} -lkrb ${KRB_LIBDES}"
	  LIBS="${cmu_save_LIBS} ${KRB_LIB_FLAGS}"
	  AC_CHECK_LIB(resolv, dns_lookup, KRB_LIB_FLAGS="${KRB_LIB_FLAGS} -lresolv",,"${KRB_LIB_FLAGS}")
	  AC_CHECK_LIB(crypt, crypt, KRB_LIB_FLAGS="${KRB_LIB_FLAGS} -lcrypt",,"${KRB_LIB_FLAGS}")
	  LIBS="${LIBS} ${KRB_LIB_FLAGS}"
	  AC_CHECK_FUNCS(krb_get_int krb_life_to_time)
          AC_SUBST(KRB_INC_FLAGS)
          AC_SUBST(KRB_LIB_FLAGS)
	  LIBS="${cmu_save_LIBS}"
	  AC_DEFINE(HAVE_KRB4,,[Kerberos V4 is present])dnl zephyr uses this
	  AC_DEFINE(KERBEROS,,[Use kerberos 4. find out what needs this symbol])
	  if test "X$RPATH" = "X"; then
		RPATH=""
	  fi
	  case "${host}" in
	    *-*-linux*)
	      if test "X$RPATH" = "X"; then
	        RPATH="-Wl,-rpath,${KRB_LIB_DIR}"
	      else 
		RPATH="${RPATH}:${KRB_LIB_DIR}"
	      fi
	      ;;
	    *-*-hpux*)
	      if test "X$RPATH" = "X"; then
	        RPATH="-Wl,+b${KRB_LIB_DIR}"
	      else 
		RPATH="${RPATH}:${KRB_LIB_DIR}"
	      fi
	      ;;
	    *-*-irix*)
	      if test "X$RPATH" = "X"; then
	        RPATH="-Wl,-rpath,${KRB_LIB_DIR}"
	      else 
		RPATH="${RPATH}:${KRB_LIB_DIR}"
	      fi
	      ;;
	    *-*-solaris2*)
	      if test "$ac_cv_prog_gcc" = yes; then
		if test "X$RPATH" = "X"; then
		  RPATH="-Wl,-R${KRB_LIB_DIR}"
		else 
		  RPATH="${RPATH}:${KRB_LIB_DIR}"
		fi
	      else
	        RPATH="${RPATH} -R${KRB_LIB_DIR}"
	      fi
	      ;;
	  esac
	  AC_SUBST(RPATH)
	fi
	])


dnl
dnl macros for configure.in to detect openssl
dnl $Id: openssl.m4,v 1.11 2006/05/17 18:30:19 murch Exp $
dnl

AC_DEFUN([CMU_HAVE_OPENSSL], [
AC_REQUIRE([CMU_FIND_LIB_SUBDIR])
AC_ARG_WITH(openssl,[  --with-openssl=PATH     use OpenSSL from PATH],
	with_openssl=$withval, with_openssl="yes")

	save_CPPFLAGS=$CPPFLAGS
	save_LDFLAGS=$LDFLAGS

	if test -d $with_openssl; then
	  CPPFLAGS="${CPPFLAGS} -I${with_openssl}/include"
	  CMU_ADD_LIBPATH(${with_openssl}/$CMU_LIB_SUBDIR)
	fi

case "$with_openssl" in
	no)
	  with_openssl="no";;
	*) 
	  dnl if openssl has been compiled with the rsaref2 libraries,
	  dnl we need to include the rsaref libraries in the crypto check
                LIB_RSAREF=""
	        AC_CHECK_LIB(rsaref, RSAPublicEncrypt,
			cmu_have_rsaref=yes;
			[AC_CHECK_LIB(RSAglue, RSAPublicEncrypt,
				LIB_RSAREF="-lRSAglue -lrsaref",
				LIB_RSAREF="-lrsaref")],
			cmu_have_rsaref=no)

		AC_CHECK_HEADER(openssl/evp.h, [
			AC_CHECK_LIB(crypto, EVP_DigestInit,
					with_openssl="yes",
					with_openssl="no", $LIB_RSAREF)],
			with_openssl=no)
		;;
esac

	if test "$with_openssl" != "no"; then
		AC_DEFINE(HAVE_OPENSSL,[],[Do we have OpenSSL?])
	else
		CPPFLAGS=$save_CPPFLAGS
		LDFLAGS=$save_LDFLAGS
	fi
])

dnl checking for kerberos 4 libraries (and DES)

AC_DEFUN([SASL_DES_CHK], [
AC_ARG_WITH(des, [  --with-des=DIR          with DES (look in DIR) [yes] ],
	with_des=$withval,
	with_des=yes)

LIB_DES=""
if test "$with_des" != no; then
  if test -d $with_des; then
    CPPFLAGS="$CPPFLAGS -I${with_des}/include"
    LDFLAGS="$LDFLAGS -L${with_des}/lib"
  fi

  if test "$with_openssl" != no; then
    dnl check for openssl installing -lcrypto, then make vanilla check
    AC_CHECK_LIB(crypto, des_cbc_encrypt, [
        AC_CHECK_HEADER(openssl/des.h, [AC_DEFINE(WITH_SSL_DES,[],[Use OpenSSL DES Implementation])
                                       LIB_DES="-lcrypto";
                                       with_des=yes],
                       with_des=no)],
        with_des=no, $LIB_RSAREF)

    dnl same test again, different symbol name
    if test "$with_des" = no; then
      AC_CHECK_LIB(crypto, DES_cbc_encrypt, [
        AC_CHECK_HEADER(openssl/des.h, [AC_DEFINE(WITH_SSL_DES,[],[Use OpenSSL DES Implementation])
                                       LIB_DES="-lcrypto";
                                       with_des=yes],
                       with_des=no)],
        with_des=no, $LIB_RSAREF)
    fi
  fi

  if test "$with_des" = no; then
    AC_CHECK_LIB(des, des_cbc_encrypt, [LIB_DES="-ldes";
                                        with_des=yes], with_des=no)
  fi

  if test "$with_des" = no; then
     AC_CHECK_LIB(des425, des_cbc_encrypt, [LIB_DES="-ldes425";
                                       with_des=yes], with_des=no)
  fi

  if test "$with_des" = no; then
     AC_CHECK_LIB(des524, des_cbc_encrypt, [LIB_DES="-ldes524";
                                       with_des=yes], with_des=no)
  fi

  if test "$with_des" = no; then
    dnl if openssl is around, we might be able to use that for des

    dnl if openssl has been compiled with the rsaref2 libraries,
    dnl we need to include the rsaref libraries in the crypto check
    LIB_RSAREF=""
    AC_CHECK_LIB(rsaref, RSAPublicEncrypt,
                 LIB_RSAREF="-lRSAglue -lrsaref"; cmu_have_rsaref=yes,
                 cmu_have_rsaref=no)

    AC_CHECK_LIB(crypto, des_cbc_encrypt, [
	AC_CHECK_HEADER(openssl/des.h, [AC_DEFINE(WITH_SSL_DES,[],[Use OpenSSL DES Implementation])
					LIB_DES="-lcrypto";
					with_des=yes],
			with_des=no)], 
        with_des=no, $LIB_RSAREF)
  fi
fi

if test "$with_des" != no; then
  AC_DEFINE(WITH_DES,[],[Use DES])
fi

AC_SUBST(LIB_DES)
])

AC_DEFUN([SASL_KERBEROS_V4_CHK], [
  AC_REQUIRE([SASL_DES_CHK])

  AC_ARG_ENABLE(krb4, [  --enable-krb4           enable KERBEROS_V4 authentication [[no]] ],
    krb4=$enableval,
    krb4=no)

  if test "$krb4" != no; then
    dnl In order to compile kerberos4, we need libkrb and libdes.
    dnl (We've already gotten libdes from SASL_DES_CHK)
    dnl we might need -lresolv for kerberos
    AC_CHECK_LIB(resolv,res_search)

    dnl if we were ambitious, we would look more aggressively for the
    dnl krb4 install
    if test -d ${krb4}; then
       AC_CACHE_CHECK(for Kerberos includes, cyrus_krbinclude, [
         for krbhloc in include/kerberosIV include/kerberos include
         do
           if test -f ${krb4}/${krbhloc}/krb.h ; then
             cyrus_krbinclude=${krb4}/${krbhloc}
             break
           fi
         done
         ])

       if test -n "${cyrus_krbinclude}"; then
         CPPFLAGS="$CPPFLAGS -I${cyrus_krbinclude}"
       fi
       LDFLAGS="$LDFLAGS -L$krb4/lib"
    fi

    if test "$with_des" != no; then
      AC_CHECK_HEADER(krb.h, [
        AC_CHECK_LIB(com_err, com_err, [
	  AC_CHECK_LIB(krb, krb_mk_priv,
                     [COM_ERR="-lcom_err"; SASL_KRB_LIB="-lkrb"; krb4lib="yes"],
                     krb4lib=no, $LIB_DES -lcom_err)], [
    	  AC_CHECK_LIB(krb, krb_mk_priv,
                     [COM_ERR=""; SASL_KRB_LIB="-lkrb"; krb4lib="yes"],
                     krb4lib=no, $LIB_DES)])], krb4="no")

      if test "$krb4" != "no" -a "$krb4lib" = "no"; then
	AC_CHECK_LIB(krb4, krb_mk_priv,
                     [COM_ERR=""; SASL_KRB_LIB="-lkrb4"; krb4=yes],
                     krb4=no, $LIB_DES)
      fi
      if test "$krb4" = no; then
          AC_WARN(No Kerberos V4 found)
      fi
    else
      AC_WARN(No DES library found for Kerberos V4 support)
      krb4=no
    fi
  fi

  if test "$krb4" != no; then
    cmu_save_LIBS="$LIBS"
    LIBS="$LIBS $SASL_KRB_LIB"
    AC_CHECK_FUNCS(krb_get_err_text)
    LIBS="$cmu_save_LIBS"
  fi

  AC_MSG_CHECKING(KERBEROS_V4)
  if test "$krb4" != no; then
    AC_MSG_RESULT(enabled)
    SASL_MECHS="$SASL_MECHS libkerberos4.la"
    SASL_STATIC_SRCS="$SASL_STATIC_SRCS ../plugins/kerberos4.c"
    SASL_STATIC_OBJS="$SASL_STATIC_OBJS kerberos4.o"
    AC_DEFINE(STATIC_KERBEROS4,[],[User KERBEROS_V4 Staticly])
    AC_DEFINE(HAVE_KRB,[],[Do we have Kerberos 4 Support?])
    SASL_KRB_LIB="$SASL_KRB_LIB $LIB_DES $COM_ERR"
  else
    AC_MSG_RESULT(disabled)
  fi
  AC_SUBST(SASL_KRB_LIB)
])


# sasl2.m4--sasl2 libraries and includes
# Rob Siemborski
# $Id: sasl2.m4,v 1.52 2006/05/18 19:25:00 murch Exp $

# SASL2_CRYPT_CHK
# ---------------
AC_DEFUN([SASL_GSSAPI_CHK],
[AC_REQUIRE([SASL2_CRYPT_CHK])
AC_REQUIRE([CMU_SOCKETS])
AC_ARG_ENABLE([gssapi],
              [AC_HELP_STRING([--enable-gssapi=<DIR>],
                              [enable GSSAPI authentication [yes]])],
              [gssapi=$enableval],
              [gssapi=yes])
AC_ARG_WITH([gss_impl],
            [AC_HELP_STRING([--with-gss_impl={heimdal|mit|cybersafe|seam|auto}],
                            [choose specific GSSAPI implementation [[auto]]])],
            [gss_impl=$withval],
            [gss_impl=auto])

if test "$gssapi" != no; then
  platform=
  case "${host}" in
    *-*-linux*)
      platform=__linux
      ;;
    *-*-hpux*)
      platform=__hpux
      ;;
    *-*-irix*)
      platform=__irix
      ;;
    *-*-solaris2*)
# When should we use __sunos?
      platform=__solaris
      ;;
    *-*-aix*)
      platform=__aix
      ;;
    *)
      AC_WARN([The system type is not recognized. If you believe that CyberSafe GSSAPI works on this platform, please update the configure script])
      if test "$gss_impl" = "cybersafe"; then
        AC_ERROR([CyberSafe was forced, cannot continue as platform is not supported])
      fi
      ;;
  esac

  cmu_saved_CPPFLAGS=$CPPFLAGS

  if test -d ${gssapi}; then
    CPPFLAGS="$CPPFLAGS -I$gssapi/include"
# We want to keep -I in our CPPFLAGS, but only if we succeed
    cmu_saved_CPPFLAGS=$CPPFLAGS
    LDFLAGS="$LDFLAGS -L$gssapi/lib"

    if test -n "$platform"; then
      if test "$gss_impl" = "auto" -o "$gss_impl" = "cybersafe"; then
        CPPFLAGS="$CPPFLAGS -D$platform"
        if test -d "${gssapi}/appsec-sdk/include"; then
          CPPFLAGS="$CPPFLAGS -I${gssapi}/appsec-sdk/include"
        fi
      fi
    fi
  fi
  AC_CHECK_HEADER([gssapi.h],
                  [AC_DEFINE(HAVE_GSSAPI_H,,
                             [Define if you have the gssapi.h header file])],
                  [AC_CHECK_HEADER([gssapi/gssapi.h],,
                                   [AC_WARN([Disabling GSSAPI - no include files found]); gssapi=no])])

  CPPFLAGS=$cmu_saved_CPPFLAGS

fi

if test "$gssapi" != no; then
  # We need to find out which gssapi implementation we are
  # using. Supported alternatives are: MIT Kerberos 5,
  # Heimdal Kerberos 5 (http://www.pdc.kth.se/heimdal),
  # CyberSafe Kerberos 5 (http://www.cybersafe.com/)
  # and Sun SEAM (http://wwws.sun.com/software/security/kerberos/)
  #
  # The choice is reflected in GSSAPIBASE_LIBS

  AC_CHECK_LIB(resolv,res_search)
  if test -d ${gssapi}; then
     gssapi_dir="${gssapi}/lib"
     GSSAPIBASE_LIBS="-L$gssapi_dir"
     GSSAPIBASE_STATIC_LIBS="-L$gssapi_dir"
  else
     # FIXME: This is only used for building cyrus, and then only as
     # a real hack.  it needs to be fixed.
     gssapi_dir="/usr/local/lib"
  fi

  # Check a full link against the Heimdal libraries.
  # If this fails, check a full link against the MIT libraries.
  # If this fails, check a full link against the CyberSafe libraries.
  # If this fails, check a full link against the Solaris 8 and up libgss.

  if test "$gss_impl" = "auto" -o "$gss_impl" = "heimdal"; then
    gss_failed=0
    AC_CHECK_LIB(gssapi,gss_unwrap,gss_impl="heimdal",gss_failed=1,
                 ${GSSAPIBASE_LIBS} -lgssapi -lkrb5 -lasn1 -lroken ${LIB_CRYPT} ${LIB_DES} -lcom_err ${LIB_SOCKET})
    if test "$gss_impl" != "auto" -a "$gss_failed" = "1"; then
      gss_impl="failed"
    fi
  fi

  if test "$gss_impl" = "auto" -o "$gss_impl" = "mit"; then
    # check for libkrb5support first
    AC_CHECK_LIB(krb5support,krb5int_getspecific,K5SUP=-lkrb5support K5SUPSTATIC=$gssapi_dir/libkrb5support.a,,${LIB_SOCKET})

    gss_failed=0
    AC_CHECK_LIB(gssapi_krb5,gss_unwrap,gss_impl="mit",gss_failed=1,
                 ${GSSAPIBASE_LIBS} -lgssapi_krb5 -lkrb5 -lk5crypto -lcom_err ${K5SUP} ${LIB_SOCKET})
    if test "$gss_impl" != "auto" -a "$gss_failed" = "1"; then
      gss_impl="failed"
    fi
  fi

  # For Cybersafe one has to set a platform define in order to make compilation work
  if test "$gss_impl" = "auto" -o "$gss_impl" = "cybersafe"; then

    cmu_saved_CPPFLAGS=$CPPFLAGS
    cmu_saved_GSSAPIBASE_LIBS=$GSSAPIBASE_LIBS
# FIXME - Note that the libraries are in .../lib64 for 64bit kernels
    if test -d "${gssapi}/appsec-rt/lib"; then
      GSSAPIBASE_LIBS="$GSSAPIBASE_LIBS -L${gssapi}/appsec-rt/lib"
    fi
    CPPFLAGS="$CPPFLAGS -D$platform"
    if test -d "${gssapi}/appsec-sdk/include"; then
      CPPFLAGS="$CPPFLAGS -I${gssapi}/appsec-sdk/include"
    fi

    gss_failed=0

# Check for CyberSafe with two libraries first, than fall back to a single 
# library (older CyberSafe)

    unset ac_cv_lib_gss_csf_gss_acq_user
    AC_CHECK_LIB(gss,csf_gss_acq_user,gss_impl="cybersafe03",
                 [unset ac_cv_lib_gss_csf_gss_acq_user;
                  AC_CHECK_LIB(gss,csf_gss_acq_user,gss_impl="cybersafe",
                               gss_failed=1,$GSSAPIBASE_LIBS -lgss)],
                 [${GSSAPIBASE_LIBS} -lgss -lcstbk5])

    if test "$gss_failed" = "1"; then
# Restore variables
      GSSAPIBASE_LIBS=$cmu_saved_GSSAPIBASE_LIBS
      CPPFLAGS=$cmu_saved_CPPFLAGS

      if test "$gss_impl" != "auto"; then
        gss_impl="failed"
      fi
    fi
  fi

  if test "$gss_impl" = "auto" -o "$gss_impl" = "seam"; then
    gss_failed=0
    AC_CHECK_LIB(gss,gss_unwrap,gss_impl="seam",gss_failed=1,-lgss)
    if test "$gss_impl" != "auto" -a "$gss_failed" = "1"; then
      gss_impl="failed"
    fi
  fi

  if test "$gss_impl" = "mit"; then
    GSSAPIBASE_LIBS="$GSSAPIBASE_LIBS -lgssapi_krb5 -lkrb5 -lk5crypto -lcom_err ${K5SUP}"
    GSSAPIBASE_STATIC_LIBS="$GSSAPIBASE_LIBS $gssapi_dir/libgssapi_krb5.a $gssapi_dir/libkrb5.a $gssapi_dir/libk5crypto.a $gssapi_dir/libcom_err.a ${K5SUPSTATIC}"
  elif test "$gss_impl" = "heimdal"; then
    CPPFLAGS="$CPPFLAGS -DKRB5_HEIMDAL"
    GSSAPIBASE_LIBS="$GSSAPIBASE_LIBS -lgssapi -lkrb5 -lasn1 -lroken ${LIB_CRYPT} ${LIB_DES} -lcom_err"
    GSSAPIBASE_STATIC_LIBS="$GSSAPIBASE_STATIC_LIBS $gssapi_dir/libgssapi.a $gssapi_dir/libkrb5.a $gssapi_dir/libasn1.a $gssapi_dir/libroken.a $gssapi_dir/libcom_err.a ${LIB_CRYPT}"
  elif test "$gss_impl" = "cybersafe03"; then
# Version of CyberSafe with two libraries
    CPPFLAGS="$CPPFLAGS -D$platform -I${gssapi}/appsec-sdk/include"
    GSSAPIBASE_LIBS="$GSSAPIBASE_LIBS -lgss -lcstbk5"
    # there is no static libgss for CyberSafe
    GSSAPIBASE_STATIC_LIBS=none
  elif test "$gss_impl" = "cybersafe"; then
    CPPFLAGS="$CPPFLAGS -D$platform -I${gssapi}/appsec-sdk/include"
    GSSAPIBASE_LIBS="$GSSAPIBASE_LIBS -lgss"
    # there is no static libgss for CyberSafe
    GSSAPIBASE_STATIC_LIBS=none
  elif test "$gss_impl" = "seam"; then
    GSSAPIBASE_LIBS=-lgss
    # there is no static libgss on Solaris 8 and up
    GSSAPIBASE_STATIC_LIBS=none
  elif test "$gss_impl" = "failed"; then
    gssapi="no"
    GSSAPIBASE_LIBS=
    GSSAPIBASE_STATIC_LIBS=
    AC_WARN([Disabling GSSAPI - specified library not found])
  else
    gssapi="no"
    GSSAPIBASE_LIBS=
    GSSAPIBASE_STATIC_LIBS=
    AC_WARN([Disabling GSSAPI - no library])
  fi
fi

#
# Cybersafe defines both GSS_C_NT_HOSTBASED_SERVICE and GSS_C_NT_USER_NAME
# in gssapi\rfckrb5.h
#
if test "$gssapi" != "no"; then
  if test "$gss_impl" = "cybersafe" -o "$gss_impl" = "cybersafe03"; then
    AC_EGREP_CPP(hostbased_service_gss_nt_yes,
                 [#include <gssapi/gssapi.h>
                  #ifdef GSS_C_NT_HOSTBASED_SERVICE
                    hostbased_service_gss_nt_yes
                  #endif],
                 [AC_DEFINE(HAVE_GSS_C_NT_HOSTBASED_SERVICE,,
                            [Define if your GSSAPI implimentation defines GSS_C_NT_HOSTBASED_SERVICE])],
                 [AC_WARN([Cybersafe define not found])])

  elif test "$ac_cv_header_gssapi_h" = "yes"; then
    AC_EGREP_HEADER(GSS_C_NT_HOSTBASED_SERVICE, gssapi.h,
                    [AC_DEFINE(HAVE_GSS_C_NT_HOSTBASED_SERVICE,,
                               [Define if your GSSAPI implimentation defines GSS_C_NT_HOSTBASED_SERVICE])])
  elif test "$ac_cv_header_gssapi_gssapi_h"; then
    AC_EGREP_HEADER(GSS_C_NT_HOSTBASED_SERVICE, gssapi/gssapi.h,
                    [AC_DEFINE(HAVE_GSS_C_NT_HOSTBASED_SERVICE,,
                               [Define if your GSSAPI implimentation defines GSS_C_NT_HOSTBASED_SERVICE])])
  fi

  if test "$gss_impl" = "cybersafe" -o "$gss_impl" = "cybersafe03"; then
    AC_EGREP_CPP(user_name_yes_gss_nt,
                 [#include <gssapi/gssapi.h>
                  #ifdef GSS_C_NT_USER_NAME
                   user_name_yes_gss_nt
                  #endif],
                 [AC_DEFINE(HAVE_GSS_C_NT_USER_NAME,,
                            [Define if your GSSAPI implimentation defines GSS_C_NT_USER_NAME])],
                 [AC_WARN([Cybersafe define not found])])
  elif test "$ac_cv_header_gssapi_h" = "yes"; then
    AC_EGREP_HEADER(GSS_C_NT_USER_NAME, gssapi.h,
                    [AC_DEFINE(HAVE_GSS_C_NT_USER_NAME,,
                               [Define if your GSSAPI implimentation defines GSS_C_NT_USER_NAME])])
  elif test "$ac_cv_header_gssapi_gssapi_h"; then
    AC_EGREP_HEADER(GSS_C_NT_USER_NAME, gssapi/gssapi.h,
                    [AC_DEFINE(HAVE_GSS_C_NT_USER_NAME,,
                               [Define if your GSSAPI implimentation defines GSS_C_NT_USER_NAME])])
  fi
fi

GSSAPI_LIBS=""
AC_MSG_CHECKING([GSSAPI])
if test "$gssapi" != no; then
  AC_MSG_RESULT([with implementation ${gss_impl}])
  AC_CHECK_LIB(resolv,res_search,GSSAPIBASE_LIBS="$GSSAPIBASE_LIBS -lresolv")
  SASL_MECHS="$SASL_MECHS libgssapiv2.la"
  SASL_STATIC_OBJS="$SASL_STATIC_OBJS gssapi.o"
  SASL_STATIC_SRCS="$SASL_STATIC_SRCS ../plugins/gssapi.c"

  cmu_save_LIBS="$LIBS"
  LIBS="$LIBS $GSSAPIBASE_LIBS"
  AC_CHECK_FUNCS(gsskrb5_register_acceptor_identity)
  LIBS="$cmu_save_LIBS"
else
  AC_MSG_RESULT([disabled])
fi
AC_SUBST(GSSAPI_LIBS)
AC_SUBST(GSSAPIBASE_LIBS)
])# SASL_GSSAPI_CHK


# SASL_SET_GSSAPI_LIBS
# --------------------
AC_DEFUN([SASL_SET_GSSAPI_LIBS],
[SASL_GSSAPI_LIBS_SET="yes"
])


# CMU_SASL2
# ---------
# What we want to do here is setup LIB_SASL with what one would
# generally want to have (e.g. if static is requested, make it that,
# otherwise make it dynamic.
#
# We also want to create LIB_DYN_SASL and DYNSASLFLAGS.
#
# Also sets using_static_sasl to "no" "static" or "staticonly"
#
AC_DEFUN([CMU_SASL2],
[AC_REQUIRE([SASL_GSSAPI_CHK])

AC_ARG_WITH(sasl,
            [AC_HELP_STRING([--with-sasl=DIR],[Compile with libsasl2 in <DIR>])],
            with_sasl="$withval",
            with_sasl="yes")

AC_ARG_WITH(staticsasl,
            [AC_HELP_STRING([--with-staticsasl=DIR],
                            [Compile with staticly linked libsasl2 in <DIR>])],
            [with_staticsasl="$withval";
             if test $with_staticsasl != "no"; then
               using_static_sasl="static"
             fi],
            [with_staticsasl="no"; using_static_sasl="no"])

SASLFLAGS=""
LIB_SASL=""

cmu_saved_CPPFLAGS=$CPPFLAGS
cmu_saved_LDFLAGS=$LDFLAGS
cmu_saved_LIBS=$LIBS

if test ${with_staticsasl} != "no"; then
  if test -d ${with_staticsasl}; then
    if test -d ${with_staticsasl}/lib64 ; then
      ac_cv_sasl_where_lib=${with_staticsasl}/lib64
    else
      ac_cv_sasl_where_lib=${with_staticsasl}/lib
    fi
    ac_cv_sasl_where_lib=${with_staticsasl}/lib
    ac_cv_sasl_where_inc=${with_staticsasl}/include

    SASLFLAGS="-I$ac_cv_sasl_where_inc"
    LIB_SASL="-L$ac_cv_sasl_where_lib"
    CPPFLAGS="${cmu_saved_CPPFLAGS} -I${ac_cv_sasl_where_inc}"
    LDFLAGS="${cmu_saved_LDFLAGS} -L${ac_cv_sasl_where_lib}"
  else
    with_staticsasl="/usr"
  fi

  AC_CHECK_HEADER(sasl/sasl.h,
                  [AC_CHECK_HEADER(sasl/saslutil.h,
                                   [for i42 in lib64 lib; do
                                      if test -r ${with_staticsasl}/$i42/libsasl2.a; then
                                        ac_cv_found_sasl=yes
                                        AC_MSG_CHECKING([for static libsasl])
                                        LIB_SASL="$LIB_SASL ${with_staticsasl}/$i42/libsasl2.a"
                                      fi
                                    done
                                    if test ! "$ac_cv_found_sasl" = "yes"; then
                                      AC_MSG_CHECKING([for static libsasl])
                                      AC_ERROR([Could not find ${with_staticsasl}/lib*/libsasl2.a])
                                    fi])])

  AC_MSG_RESULT([found])

  if test "x$SASL_GSSAPI_LIBS_SET" = "x"; then
    LIB_SASL="$LIB_SASL $GSSAPIBASE_STATIC_LIBS"
  else
    SASL_GSSAPI_LIBS_SET=""
    cmu_saved_LIBS="$GSSAPIBASE_STATIC_LIBS $cmu_saved_LIBS" 
  fi
fi

if test -d ${with_sasl}; then
  ac_cv_sasl_where_lib=${with_sasl}/lib
  ac_cv_sasl_where_inc=${with_sasl}/include

  DYNSASLFLAGS="-I$ac_cv_sasl_where_inc"
  if test "$ac_cv_sasl_where_lib" != ""; then
    CMU_ADD_LIBPATH_TO($ac_cv_sasl_where_lib, LIB_DYN_SASL)
  fi
  LIB_DYN_SASL="$LIB_DYN_SASL -lsasl2"
  CPPFLAGS="${cmu_saved_CPPFLAGS} -I${ac_cv_sasl_where_inc}"
  LDFLAGS="${cmu_saved_LDFLAGS} -L${ac_cv_sasl_where_lib}"
fi

# be sure to check for a SASLv2 specific function
AC_CHECK_HEADER(sasl/sasl.h,
                [AC_CHECK_HEADER(sasl/saslutil.h,
                                 [AC_CHECK_LIB(sasl2, prop_get, 
                                               ac_cv_found_sasl=yes,
                                               ac_cv_found_sasl=no)],
                                 ac_cv_found_sasl=no)],
                ac_cv_found_sasl=no)

if test "$ac_cv_found_sasl" = "yes"; then
  if test "$ac_cv_sasl_where_lib" != ""; then
    CMU_ADD_LIBPATH_TO($ac_cv_sasl_where_lib, DYNLIB_SASL)
  fi
  DYNLIB_SASL="$DYNLIB_SASL -lsasl2"
  if test "$using_static_sasl" != "static"; then
    LIB_SASL=$DYNLIB_SASL
    SASLFLAGS=$DYNSASLFLAGS
  fi
else
  DYNLIB_SASL=""
  DYNSASLFLAGS=""
  using_static_sasl="staticonly"
fi

if test "x$SASL_GSSAPI_LIBS_SET" != "x"; then
  SASL_GSSAPI_LIBS_SET=""
  cmu_saved_LIBS="$GSSAPIBASE_LIBS $cmu_saved_LIBS" 
fi

LIBS="$cmu_saved_LIBS"
LDFLAGS="$cmu_saved_LDFLAGS"
CPPFLAGS="$cmu_saved_CPPFLAGS"

AC_SUBST(LIB_DYN_SASL)
AC_SUBST(DYNSASLFLAGS)
AC_SUBST(LIB_SASL)
AC_SUBST(SASLFLAGS)
])# CMU_SASL2


# CMU_SASL2_REQUIRED
# ------------------
AC_DEFUN([CMU_SASL2_REQUIRED],
[AC_REQUIRE([CMU_SASL2])
if test "$ac_cv_found_sasl" != "yes"; then
  AC_ERROR([Cannot continue without libsasl2.
Get it from ftp://ftp.andrew.cmu.edu/pub/cyrus-mail/.])
fi])


# CMU_SASL2_REQUIRE_VER
# ---------------------
AC_DEFUN([CMU_SASL2_REQUIRE_VER],
[AC_REQUIRE([CMU_SASL2_REQUIRED])

cmu_saved_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $SASLFLAGS"

AC_TRY_CPP([
#include <sasl/sasl.h>

#ifndef SASL_VERSION_MAJOR
#error SASL_VERSION_MAJOR not defined
#endif
#ifndef SASL_VERSION_MINOR
#error SASL_VERSION_MINOR not defined
#endif
#ifndef SASL_VERSION_STEP
#error SASL_VERSION_STEP not defined
#endif

#if SASL_VERSION_MAJOR < $1 || SASL_VERSION_MINOR < $2 || SASL_VERSION_STEP < $3
#error SASL version is less than $1.$2.$3
#endif
],,
           [AC_ERROR([Incorrect SASL headers found.  This package requires SASL $1.$2.$3 or newer.])])

CPPFLAGS=$cmu_saved_CPPFLAGS
])# CMU_SASL2_REQUIRE_VER


# CMU_SASL2_CHECKAPOP_REQUIRED
# ----------------------------
AC_DEFUN([CMU_SASL2_CHECKAPOP_REQUIRED],
[AC_REQUIRE([CMU_SASL2_REQUIRED])

cmu_saved_LDFLAGS=$LDFLAGS

LDFLAGS="$LDFLAGS $LIB_SASL"

AC_CHECK_LIB(sasl2, sasl_checkapop,
             [AC_DEFINE(HAVE_APOP,[],[Does SASL support APOP?])],
             [AC_MSG_ERROR([libsasl2 without working sasl_checkapop.  Cannot continue.])])

LDFLAGS=$cmu_saved_LDFLAGS
])# CMU_SASL2_CHECKAPOP_REQUIRED


# SASL2_CRYPT_CHK
# ---------------
AC_DEFUN([SASL2_CRYPT_CHK],
[AC_CHECK_FUNC(crypt, cmu_have_crypt=yes,
               [AC_CHECK_LIB(crypt, crypt,
                             LIB_CRYPT="-lcrypt"; cmu_have_crypt=yes,
                             cmu_have_crypt=no)])
AC_SUBST(LIB_CRYPT)
])# SASL2_CRYPT_CHK

dnl Check for PLAIN (and therefore crypt)

AC_DEFUN([SASL_PLAIN_CHK],[
AC_REQUIRE([SASL2_CRYPT_CHK])

dnl PLAIN
 AC_ARG_ENABLE(plain, [  --enable-plain          enable PLAIN authentication [yes] ],
  plain=$enableval,
  plain=yes)

 PLAIN_LIBS=""
 if test "$plain" != no; then
  dnl In order to compile plain, we need crypt.
  if test "$cmu_have_crypt" = yes; then
    PLAIN_LIBS=$LIB_CRYPT
  fi
 fi
 AC_SUBST(PLAIN_LIBS)

 AC_MSG_CHECKING(PLAIN)
 if test "$plain" != no; then
  AC_MSG_RESULT(enabled)
  SASL_MECHS="$SASL_MECHS libplain.la"
  if test "$enable_static" = yes; then
    SASL_STATIC_OBJS="$SASL_STATIC_OBJS plain.o"
    SASL_STATIC_SRCS="$SASL_STATIC_SRCS ../plugins/plain.c"
    AC_DEFINE(STATIC_PLAIN,[],[Link PLAIN Staticly])
  fi
 else
  AC_MSG_RESULT(disabled)
 fi
])

dnl
dnl macros for configure.in to detect openldap
dnl $Id: openldap.m4,v 1.2 2006/03/13 19:16:11 mel Exp $
dnl

dnl
dnl Check for OpenLDAP version compatility
AC_DEFUN([CMU_OPENLDAP_API],
[AC_CACHE_CHECK([OpenLDAP api], [cmu_cv_openldap_api],[
    AC_EGREP_CPP(__openldap_api,[
#include <ldap.h>

#ifdef LDAP_API_FEATURE_X_OPENLDAP
char *__openldap_api = LDAP_API_FEATURE_X_OPENLDAP;
#endif
],      [cmu_cv_openldap_api=yes], [cmu_cv_openldap_api=no])])
])

dnl
dnl Check for OpenLDAP version compatility
AC_DEFUN([CMU_OPENLDAP_COMPAT],
[AC_CACHE_CHECK([OpenLDAP version], [cmu_cv_openldap_compat],[
    AC_EGREP_CPP(__openldap_compat,[
#include <ldap.h>

/* Require 2.1.27+ and 2.2.6+ */
#if LDAP_VENDOR_VERSION_MAJOR == 2  && LDAP_VENDOR_VERSION_MINOR == 1 && LDAP_VENDOR_VERSION_PATCH > 26
char *__openldap_compat = "2.1.27 or better okay";
#elif LDAP_VENDOR_VERSION_MAJOR == 2  && LDAP_VENDOR_VERSION_MINOR == 2 && LDAP_VENDOR_VERSION_PATCH > 5
char *__openldap_compat = "2.2.6 or better okay";
#elif LDAP_VENDOR_VERSION_MAJOR == 2  && LDAP_VENDOR_VERSION_MINOR > 2
char *__openldap_compat = "2.3 or better okay"
#endif
],      [cmu_cv_openldap_compat=yes], [cmu_cv_openldap_compat=no])])
])


dnl See whether we can use IPv6 related functions
dnl contributed by Hajimu UMEMOTO

AC_DEFUN([IPv6_CHECK_FUNC], [
AC_CHECK_FUNC($1, [dnl
  ac_cv_lib_socket_$1=no
  ac_cv_lib_inet6_$1=no
], [dnl
  AC_CHECK_LIB(socket, $1, [dnl
    LIBS="$LIBS -lsocket"
    ac_cv_lib_inet6_$1=no
  ], [dnl
    AC_MSG_CHECKING([whether your system has IPv6 directory])
    AC_CACHE_VAL(ipv6_cv_dir, [dnl
      for ipv6_cv_dir in /usr/local/v6 /usr/inet6 no; do
	if test $ipv6_cv_dir = no -o -d $ipv6_cv_dir; then
	  break
	fi
      done])dnl
    AC_MSG_RESULT($ipv6_cv_dir)
    if test $ipv6_cv_dir = no; then
      ac_cv_lib_inet6_$1=no
    else
      if test x$ipv6_libinet6 = x; then
	ipv6_libinet6=no
	SAVELDFLAGS="$LDFLAGS"
	LDFLAGS="$LDFLAGS -L$ipv6_cv_dir/lib"
      fi
      AC_CHECK_LIB(inet6, $1, [dnl
	if test $ipv6_libinet6 = no; then
	  ipv6_libinet6=yes
	  LIBS="$LIBS -linet6"
	fi],)dnl
      if test $ipv6_libinet6 = no; then
	LDFLAGS="$SAVELDFLAGS"
      fi
    fi])dnl
])dnl
ipv6_cv_$1=no
if test $ac_cv_func_$1 = yes -o $ac_cv_lib_socket_$1 = yes \
     -o $ac_cv_lib_inet6_$1 = yes
then
  ipv6_cv_$1=yes
fi
if test $ipv6_cv_$1 = no; then
  if test $1 = getaddrinfo; then
    for ipv6_cv_pfx in o n; do
      AC_EGREP_HEADER(${ipv6_cv_pfx}$1, netdb.h,
		      [AC_CHECK_FUNC(${ipv6_cv_pfx}$1)])
      if eval test X\$ac_cv_func_${ipv6_cv_pfx}$1 = Xyes; then
        AC_DEFINE(HAVE_GETADDRINFO,[],[Do we have a getaddrinfo?])
        ipv6_cv_$1=yes
        break
      fi
    done
  fi
fi
if test $ipv6_cv_$1 = yes; then
  ifelse([$2], , :, [$2])
else
  ifelse([$3], , :, [$3])
fi])


dnl See whether we have ss_family in sockaddr_storage
AC_DEFUN([IPv6_CHECK_SS_FAMILY], [
AC_MSG_CHECKING([whether you have ss_family in struct sockaddr_storage])
AC_CACHE_VAL(ipv6_cv_ss_family, [dnl
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>],
	[struct sockaddr_storage ss; int i = ss.ss_family;],
	[ipv6_cv_ss_family=yes], [ipv6_cv_ss_family=no])])dnl
if test $ipv6_cv_ss_family = yes; then
  ifelse([$1], , AC_DEFINE(HAVE_SS_FAMILY,[],[Is there an ss_family in sockaddr_storage?]), [$1])
else
  ifelse([$2], , :, [$2])
fi
AC_MSG_RESULT($ipv6_cv_ss_family)])


dnl whether you have sa_len in struct sockaddr
AC_DEFUN([IPv6_CHECK_SA_LEN], [
AC_MSG_CHECKING([whether you have sa_len in struct sockaddr])
AC_CACHE_VAL(ipv6_cv_sa_len, [dnl
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>],
	       [struct sockaddr sa; int i = sa.sa_len;],
	       [ipv6_cv_sa_len=yes], [ipv6_cv_sa_len=no])])dnl
if test $ipv6_cv_sa_len = yes; then
  ifelse([$1], , AC_DEFINE(HAVE_SOCKADDR_SA_LEN,[],[Does sockaddr have an sa_len?]), [$1])
else
  ifelse([$2], , :, [$2])
fi
AC_MSG_RESULT($ipv6_cv_sa_len)])


dnl See whether sys/socket.h has socklen_t
AC_DEFUN([IPv6_CHECK_SOCKLEN_T], [
AC_MSG_CHECKING(for socklen_t)
AC_CACHE_VAL(ipv6_cv_socklen_t, [dnl
AC_TRY_LINK([#include <sys/types.h>
#include <sys/socket.h>],
	    [socklen_t len = 0;],
	    [ipv6_cv_socklen_t=yes], [ipv6_cv_socklen_t=no])])dnl
if test $ipv6_cv_socklen_t = yes; then
  ifelse([$1], , AC_DEFINE(HAVE_SOCKLEN_T,[],[Do we have a socklen_t?]), [$1])
else
  ifelse([$2], , :, [$2])
fi
AC_MSG_RESULT($ipv6_cv_socklen_t)])


