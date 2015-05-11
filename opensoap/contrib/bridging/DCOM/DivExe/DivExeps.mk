
DivExeps.dll: dlldata.obj DivExe_p.obj DivExe_i.obj
	link /dll /out:DivExeps.dll /def:DivExeps.def /entry:DllMain dlldata.obj DivExe_p.obj DivExe_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del DivExeps.dll
	@del DivExeps.lib
	@del DivExeps.exp
	@del dlldata.obj
	@del DivExe_p.obj
	@del DivExe_i.obj
