
MultiExeps.dll: dlldata.obj MultiExe_p.obj MultiExe_i.obj
	link /dll /out:MultiExeps.dll /def:MultiExeps.def /entry:DllMain dlldata.obj MultiExe_p.obj MultiExe_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del MultiExeps.dll
	@del MultiExeps.lib
	@del MultiExeps.exp
	@del dlldata.obj
	@del MultiExe_p.obj
	@del MultiExe_i.obj
