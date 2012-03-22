
MultiDllps.dll: dlldata.obj MultiDll_p.obj MultiDll_i.obj
	link /dll /out:MultiDllps.dll /def:MultiDllps.def /entry:DllMain dlldata.obj MultiDll_p.obj MultiDll_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del MultiDllps.dll
	@del MultiDllps.lib
	@del MultiDllps.exp
	@del dlldata.obj
	@del MultiDll_p.obj
	@del MultiDll_i.obj
