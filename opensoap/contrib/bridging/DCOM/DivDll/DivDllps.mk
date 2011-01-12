
DivDllps.dll: dlldata.obj DivDll_p.obj DivDll_i.obj
	link /dll /out:DivDllps.dll /def:DivDllps.def /entry:DllMain dlldata.obj DivDll_p.obj DivDll_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del DivDllps.dll
	@del DivDllps.lib
	@del DivDllps.exp
	@del dlldata.obj
	@del DivDll_p.obj
	@del DivDll_i.obj
