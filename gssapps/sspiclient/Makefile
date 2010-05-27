!include <win32.mak>

DEFS=-D_UNICODE -DUNICODE -DWIN32_LEAN_AND_MEAN

all: sspi-client.exe

sspi-client.exe: sspi-client.obj
	$(link) $(ldebug) $(conlflags) -out:$@ $** $(conlibs) secur32.lib

clean:
	del *.obj *.pdb *.ilk *.exe

.c.obj:
	$(cc) $(cdebug) $(cflags) $(cvars) $(DEFS) $<
