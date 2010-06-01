!include <win32.mak>

cflags=$(cflags) -W3 -WX -wd4996 -wd4127
cvars=$(cvars) -D_UNICODE -DUNICODE -DWIN32_LEAN_AND_MEAN

all: sspi-server.exe sspi-client.exe

sspi-server.exe: sspi-server.obj
	$(link) $(ldebug) $(conlflags) -out:$@ $** $(conlibs) secur32.lib

sspi-client.exe: sspi-client.obj
	$(link) $(ldebug) $(conlflags) -out:$@ $** $(conlibs) secur32.lib

sspi-server.obj: sspi-server.c
	$(cc) $(cdebug) $(cflags) $(cvars) -Fo$@ $?

sspi-client.obj: sspi-client.c
	$(cc) $(cdebug) $(cflags) $(cvars) -W4 -Fo$@ $?

clean:
	del *.obj *.pdb *.ilk *.exe

!if 0
.c.obj:
	$(cc) $(cdebug) $(cflags) $(cvars) $<
!endif
