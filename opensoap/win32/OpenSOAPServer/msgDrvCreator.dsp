# Microsoft Developer Studio Project File - Name="msgDrvCreator" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=msgDrvCreator - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "msgDrvCreator.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "msgDrvCreator.mak" CFG="msgDrvCreator - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "msgDrvCreator - Win32 Release" ("Win32 (x86) Console Application" 用)
!MESSAGE "msgDrvCreator - Win32 Debug" ("Win32 (x86) Console Application" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "msgDrvCreator - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "C:\libxml2\include" /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "NO_LOGGER" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libxml2.lib wsock32.lib OpenSOAPClient.lib OpenSOAPSecurity.lib /nologo /subsystem:console /machine:I386 /libpath:"C:\libxml2\lib" /libpath:"..\OpenSOAP\Release_DLL"

!ELSEIF  "$(CFG)" == "msgDrvCreator - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "C:\libxml2\include" /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "DEBUG" /D "NO_LOGGER" /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libxml2.lib wsock32.lib OpenSOAPClient.lib OpenSOAPSecurity.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"C:\libxml2\lib" /libpath:"..\OpenSOAP\Debug_DLL"

!ENDIF 

# Begin Target

# Name "msgDrvCreator - Win32 Release"
# Name "msgDrvCreator - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\server\FileIDFunc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\Forwarder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\FwdQueuePushChnlSelector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\MsgDrv.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\MsgDrvCreator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\MsgDrvCreator_main.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\ReqQueuePushChnlSelector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\SrvDrv.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\SSMLAttrHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\SSMLInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\SSMLInfoFunc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\Timeout.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\TTLAdopter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\TTLTablePushChnlSelector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\server\TTLTableRefChnlSelector.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
