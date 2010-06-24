AUTOMAKE_OPTIONS = 1.7
# Top-level Makefile.am for SASL
# Rob Earhart
#
################################################################
# Copyright (c) 2000 Carnegie Mellon University.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer. 
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# 3. The name "Carnegie Mellon University" must not be used to
#    endorse or promote products derived from this software without
#    prior written permission. For permission or any other legal
#    details, please contact  
#      Office of Technology Transfer
#      Carnegie Mellon University
#      5000 Forbes Avenue
#      Pittsburgh, PA  15213-3890
#      (412) 268-4387, fax: (412) 268-7395
#      tech-transfer@andrew.cmu.edu
#
# 4. Redistributions of any form whatsoever must retain the following
#    acknowledgment:
#    "This product includes software developed by Computing Services
#     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
#
# CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
# THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
# FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
# AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
# OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
################################################################

if SASLAUTHD
SAD = saslauthd
else
SAD =
endif
if PWCHECK
PWC = pwcheck
else
PWC = 
endif
if SAMPLE
SAM = sample
else
SAM =
endif
if JAVA
JAV = java
else
JAV =
endif
if MACOSX
INSTALLOSX = install-exec-local-osx
else 
INSTALLOSX = 
endif

SUBDIRS=include sasldb plugins lib utils doc man $(PWC) $(SAM) $(JAV) $(SAD)
EXTRA_DIST=config cmulocal win32 mac dlcompat-20010505 NTMakefile INSTALL.TXT

dist-hook:
	@find $(distdir) -exec chmod o+w {} ';'
	@find $(distdir) -name CVS -print | xargs -t rm -rf
	(cd $(distdir)/plugins && sh makeinit.sh)

framedir = /Library/Frameworks/SASL2.framework
install-exec-local: $(INSTALLOSX)
	@if test "$(plugindir)" != "$(prefix)/lib/sasl2"; then \
	  echo "********************************************************"; \
	  echo "* WARNING:"; \
	  echo "* Plugins are being installed into $(prefix)/lib/sasl2,"; \
	  echo "* but the library will look for them in $(plugindir)."; \
	  echo "* You need to make sure that the plugins will eventually"; \
	  echo "* be in $(plugindir) -- the easiest way is to make a"; \
	  echo "* symbolic link from $(plugindir) to $(prefix)/lib/sasl2,"; \
	  echo "* but this may not be appropriate for your site, so this"; \
	  echo "* installation procedure won't do it for you."; \
	  echo "*"; \
          echo "* If you don't want to do this for some reason, you can"; \
          echo "* set the location where the library will look for plugins"; \
	  echo "* by setting the environment variable SASL_PATH to the path"; \
	  echo "* the library should use."; \
	  echo "********************************************************"; \
	fi
install-exec-local-osx:
	$(mkinstalldirs) $(framedir)/Versions/A/Headers
	$(mkinstalldirs) $(framedir)/Versions/A/Resources
	cd $(framedir)/Versions ; ln -fs A Current
	cd $(framedir) ; ln -fs Versions/A/Headers .
	cd $(framedir) ; ln -fs Versions/A/Resources .
	$(INSTALL_DATA) $(srcdir)/config/Info.plist $(framedir)/Versions/A/Resources

