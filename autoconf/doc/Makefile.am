## Process this file with automake to create Makefile.in.

## Makefile for Autoconf.
## Copyright (C) 2000, 2001, 2002, 2003  Free Software Foundation, Inc.

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

AM_MAKEINFOFLAGS = --no-split
TEXI2DVI = texi2dvi --batch
TEXI2HTML = texi2html
TEXI2HTML_FLAGS = -split_chapter

info_TEXINFOS = autoconf.texi standards.texi
autoconf_TEXINFOS = fdl.texi install.texi
standards_TEXINFOS = make-stds.texi

# Files from texi2dvi that should be removed, but which Automake does
# not know.
CLEANFILES = autoconf.ACs  autoconf.cvs  autoconf.MSs  autoconf.prs \
	     autoconf.ATs  autoconf.evs  autoconf.fns  autoconf.ovs \
	     autoconf.tmp \
	     autoconf*.html standards*.html


## ----------------------------- ##
## Other documentation formats.  ##
## ----------------------------- ##

html: autoconf_1.html standards_1.html

autoconf_1.html: autoconf.texi install.texi
	$(TEXI2HTML) $(TEXI2HTML_FLAGS) $(srcdir)/autoconf.texi

standards_1.html: standards.texi make-stds.texi
	$(TEXI2HTML) $(TEXI2HTML_FLAGS) $(srcdir)/standards.texi


.PHONY: html
