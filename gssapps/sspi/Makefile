!include <win32.mak>

cflags=$(cflags) -W3 -WX -wd4996 -wd4127
cvars=$(cvars) -D_UNICODE -DUNICODE -DWIN32_LEAN_AND_MEAN

all: sspi-server.exe sspi-client.exe

sspi-server.exe: sspi-server.obj sspi-common.obj
	$(link) $(ldebug) $(conlflags) -out:$@ $** $(conlibs) secur32.lib

sspi-client.exe: sspi-client.obj sspi-common.obj
	$(link) $(ldebug) $(conlflags) -out:$@ $** $(conlibs) secur32.lib

sspi-common.obj: sspi-common.h
sspi-server.obj: sspi-common.h
sspi-client.obj: sspi-common.h

.c.obj:
	$(cc) $(cdebug) $(cflags) $(cvars) -Fo$@ $<

clean:
	del *.obj *.pdb *.ilk *.exe
