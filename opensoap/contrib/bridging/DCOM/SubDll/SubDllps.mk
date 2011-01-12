
SubDllps.dll: dlldata.obj SubDll_p.obj SubDll_i.obj
	link /dll /out:SubDllps.dll /def:SubDllps.def /entry:DllMain dlldata.obj SubDll_p.obj SubDll_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del SubDllps.dll
	@del SubDllps.lib
	@del SubDllps.exp
	@del dlldata.obj
	@del SubDll_p.obj
	@del SubDll_i.obj
