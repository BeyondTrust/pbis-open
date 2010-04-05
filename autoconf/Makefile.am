## Process this file with automake to create Makefile.in. -*-Makefile-*-

## Makefile for Autoconf.

## Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2006 Free
## Software Foundation, Inc.

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
## 02110-1301, USA.

# bin/ must be run first, as it builds executables needed for tests.
# autom4te uses autotest.m4f to generate `testsuite', so build tests last.
SUBDIRS = bin . lib man doc tests

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = ChangeLog.0 ChangeLog.1 ChangeLog.2 \
	     BUGS \
	     GNUmakefile Makefile.maint Makefile.cfg \
	     config/announce-gen config/prev-version.txt

MAINTAINERCLEANFILES = $(srcdir)/INSTALL

## -------------------- ##
## Forwarding targets.  ##
## -------------------- ##

html:
	cd doc && $(MAKE) $(AM_MAKEFLAGS) $@

.PHONY: html


## --------- ##
## INSTALL.  ##
## --------- ##

pkgdata_DATA = $(srcdir)/INSTALL
AM_MAKEINFOFLAGS = --no-headers --no-validate --no-split
$(srcdir)/INSTALL: $(top_srcdir)/doc/install.texi
	$(MAKEINFO) $(AM_MAKEINFOFLAGS) $(MAKEINFOFLAGS) -o $@ \
	  $(top_srcdir)/doc/install.texi

## maintainer-check ##
maintainer-check: maintainer-check-tests
maintainer-check-tests:
	cd tests && make maintainer-check


## ----------------------------------- ##
## Updating Perl files from Automake.  ##
## ----------------------------------- ##

WGET = wget
WGETFLAGS = -C off

## Fetch the latest versions of files we care about.
automake_cvsweb = \
   http://sources.redhat.com/cgi-bin/cvsweb.cgi/~checkout~/automake
automake_cvsargs = \
   'content-type=text/plain&cvsroot=automake'
autom4te_files = \
  Autom4te/Configure_ac.pm \
  Autom4te/Channels.pm \
  Autom4te/FileUtils.pm \
  Autom4te/Struct.pm \
  Autom4te/XFile.pm

autom4te-update:
	rm -f -r Fetchdir > /dev/null 2>&1
	mkdir -p Fetchdir/Autom4te
	for file in $(autom4te_files); do \
	  $(WGET) $(WGET_FLAGS) \
	    "$(automake_cvsweb)/lib/$file?$(automake_cvsargs)" \
	    -O "Fetchdir/$$file" || exit; \
	done
	perl -pi -e 's/Automake::/Autom4te::/g' Fetchdir/Autom4te/*.pm
	for file in $(autom4te_files); do \
	  $(move_if_change) Fetchdir/$$file $(srcdir)/lib/$$file || exit; \
	done
