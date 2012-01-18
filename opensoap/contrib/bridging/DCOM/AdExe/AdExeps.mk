
AdExeps.dll: dlldata.obj AdExe_p.obj AdExe_i.obj
	link /dll /out:AdExeps.dll /def:AdExeps.def /entry:DllMain dlldata.obj AdExe_p.obj AdExe_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del AdExeps.dll
	@del AdExeps.lib
	@del AdExeps.exp
	@del dlldata.obj
	@del AdExe_p.obj
	@del AdExe_i.obj
