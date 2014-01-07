# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
# ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
# TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.
#
# Copyright (C) 1998-1999 Microsoft Corporation.  All rights reserved.

!IF "$(TARGETOS)" == "WINNT"

!include <win32.mak>

DEFS    = -D_UNICODE -DUNICODE -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS -WX

.c.obj:
        $(cc) $(cdebug) $(cflags) $(cvars) $(DEFS) $<


all: gssclient.exe gssserver.exe

gssclient.exe: gssclient.obj gssmisc.obj
        $(link) $(ldebug) $(conlflags) -out:$@ gssclient.obj gssmisc.obj \
                  $(conlibs) secur32.lib

gssserver.exe: gssserver.obj gssmisc.obj
        $(link) $(ldebug) $(conlflags) -out:$@ gssserver.obj gssmisc.obj \
                  $(conlibs) secur32.lib

clean:
        del *.obj
        del *.pdb
        del *.bak

cleaner: clean
        del gssclient.exe
        del gssserver.exe

!ELSE
!MESSAGE Sorry this sample only builds for the Windows NT Platform.  Set TARGETOS to "WINNT".
!ENDIF
