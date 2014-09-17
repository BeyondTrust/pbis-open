
SubExeps.dll: dlldata.obj SubExe_p.obj SubExe_i.obj
	link /dll /out:SubExeps.dll /def:SubExeps.def /entry:DllMain dlldata.obj SubExe_p.obj SubExe_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del SubExeps.dll
	@del SubExeps.lib
	@del SubExeps.exp
	@del dlldata.obj
	@del SubExe_p.obj
	@del SubExe_i.obj
