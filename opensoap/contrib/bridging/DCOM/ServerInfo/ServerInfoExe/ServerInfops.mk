
ServerInfops.dll: dlldata.obj ServerInfo_p.obj ServerInfo_i.obj
	link /dll /out:ServerInfops.dll /def:ServerInfops.def /entry:DllMain dlldata.obj ServerInfo_p.obj ServerInfo_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del ServerInfops.dll
	@del ServerInfops.lib
	@del ServerInfops.exp
	@del dlldata.obj
	@del ServerInfo_p.obj
	@del ServerInfo_i.obj
