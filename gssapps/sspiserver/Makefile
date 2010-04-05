!include <win32.mak>

DEFS=-D_UNICODE -DUNICODE -DWIN32_LEAN_AND_MEAN
OBJS=sspi-server.obj

all: sspi-server.exe

sspi-server.exe: $(OBJS)
	$(link) $(ldebug) $(conlflags) -out:$@ $(OBJS) $(conlibs) secur32.lib

clean:
	del *.obj *.pdb *.ilk *.exe

.c.obj:
	$(cc) $(cdebug) $(cflags) $(cvars) $(DEFS) $<
