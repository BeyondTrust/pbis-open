# Microsoft Developer Studio Project File - Name="OpenSOAPSecurity" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=OpenSOAPSecurity - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "OpenSOAPSecurity.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "OpenSOAPSecurity.mak" CFG="OpenSOAPSecurity - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "OpenSOAPSecurity - Win32 Release" ("Win32 (x86) Static Library" 用)
!MESSAGE "OpenSOAPSecurity - Win32 Debug" ("Win32 (x86) Static Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OpenSOAPSecurity - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "OpenSOAPSecurity___Win32_Release"
# PROP BASE Intermediate_Dir "OpenSOAPSecurity___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\include" /I "C:\OPENSSL\include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "OPENSOAP_STATIC" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "OpenSOAPSecurity - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "OpenSOAPSecurity___Win32_Debug"
# PROP BASE Intermediate_Dir "OpenSOAPSecurity___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "..\..\include" /I "C:\OPENSSL\include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "OPENSOAP_STATIC" /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "OpenSOAPSecurity - Win32 Release"
# Name "OpenSOAPSecurity - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\security\CA_database.c
# End Source File
# Begin Source File

SOURCE=..\..\src\security\cert_fio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\security\sec_dconv.c
# End Source File
# Begin Source File

SOURCE=..\..\src\security\sec_encrypt.c
# End Source File
# Begin Source File

SOURCE=..\..\src\security\sec_encryptEnvelope.c
# End Source File
# Begin Source File

SOURCE=..\..\src\security\sec_genrsakey.c
# End Source File
# Begin Source File

SOURCE=..\..\src\security\sec_hash.c
# End Source File
# Begin Source File

SOURCE=..\..\src\security\sec_keyfile.c
# End Source File
# Begin Source File

SOURCE=..\..\src\security\sec_rsasig.c
# End Source File
# Begin Source File

SOURCE=..\..\src\security\sec_signEnvelope.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src\security\security_defs.h
# End Source File
# End Group
# End Target
# End Project
