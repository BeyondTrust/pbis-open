!include <win32.mak>

DEFS=-D_UNICODE -DUNICODE -DWIN32_LEAN_AND_MEAN

all: sspi-server.exe

sspi-server.exe: sspi-server.obj
	$(link) $(ldebug) $(conlflags) -out:$@ $** $(conlibs) secur32.lib

clean:
	del *.obj *.pdb *.ilk *.exe

.c.obj:
	$(cc) $(cdebug) $(cflags) $(cvars) $(DEFS) $<
