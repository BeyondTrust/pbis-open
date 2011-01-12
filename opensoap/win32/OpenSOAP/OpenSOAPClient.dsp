# Microsoft Developer Studio Project File - Name="OpenSOAPClient" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=OpenSOAPClient - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "OpenSOAPClient.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "OpenSOAPClient.mak" CFG="OpenSOAPClient - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "OpenSOAPClient - Win32 Release" ("Win32 (x86) Static Library" 用)
!MESSAGE "OpenSOAPClient - Win32 Debug" ("Win32 (x86) Static Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OpenSOAPClient - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "OpenSOAPClient___Win32_Release"
# PROP BASE Intermediate_Dir "OpenSOAPClient___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\include" /I "C:\LIBXML2\include" /I "C:\OPENSSL\include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "OPENSOAP_STATIC" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "OpenSOAPClient - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "OpenSOAPClient___Win32_Debug"
# PROP BASE Intermediate_Dir "OpenSOAPClient___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /I "C:\LIBXML2\include" /I "C:\OPENSSL\include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "OPENSOAP_STATIC" /FR /YX /FD /GZ /c
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

# Name "OpenSOAPClient - Win32 Release"
# Name "OpenSOAPClient - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\api\AddrInfo.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\Block.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\ByteArray.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\ClientSocket.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\CStdio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\DLinkList.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\Envelope.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\Locale.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\Object.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\OpenSOAP.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\Serializer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\Socket.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\Stream.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\String.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\StringHash.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\Transport.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\XMLAttr.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\XMLElm.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\XMLNamespace.c
# End Source File
# Begin Source File

SOURCE=..\..\src\api\XMLNode.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
