# Microsoft Developer Studio Project File - Name="ShoppingClientSec_DLL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ShoppingClientSec_DLL - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "ShoppingClientSec_DLL.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "ShoppingClientSec_DLL.mak" CFG="ShoppingClientSec_DLL - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "ShoppingClientSec_DLL - Win32 Release" ("Win32 (x86) Console Application" 用)
!MESSAGE "ShoppingClientSec_DLL - Win32 Debug" ("Win32 (x86) Console Application" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ShoppingClientSec_DLL - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ShoppingClientSec_DLL___Win32_Release"
# PROP BASE Intermediate_Dir "ShoppingClientSec_DLL___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_DLL"
# PROP Intermediate_Dir "Release_DLL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "C:\OpenSOAP\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib OpenSOAPClient.lib OpenSOAPSecurity.lib /nologo /subsystem:console /machine:I386 /out:"Release_DLL/ShoppingClientSec.exe" /libpath:"C:\OpenSOAP\lib\DLL"

!ELSEIF  "$(CFG)" == "ShoppingClientSec_DLL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ShoppingClientSec_DLL___Win32_Debug"
# PROP BASE Intermediate_Dir "ShoppingClientSec_DLL___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_DLL"
# PROP Intermediate_Dir "Debug_DLL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "C:\OpenSOAP\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib OpenSOAPClient.lib OpenSOAPSecurity.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug_DLL/ShoppingClientSec.exe" /pdbtype:sept /libpath:"C:\OpenSOAP\lib\DLL"

!ENDIF 

# Begin Target

# Name "ShoppingClientSec_DLL - Win32 Release"
# Name "ShoppingClientSec_DLL - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\samples\ShoppingSec\ClCmn.c
# End Source File
# Begin Source File

SOURCE=..\..\samples\ShoppingSec\ClSecCmn.c
# End Source File
# Begin Source File

SOURCE=..\..\samples\ShoppingSec\ShClCmn.c
# End Source File
# Begin Source File

SOURCE=..\..\samples\ShoppingSec\ShClGetProductCount.c
# End Source File
# Begin Source File

SOURCE=..\..\samples\ShoppingSec\ShClGetProductSpec.c
# End Source File
# Begin Source File

SOURCE=..\..\samples\ShoppingSec\ShClGetStockQty.c
# End Source File
# Begin Source File

SOURCE=..\..\samples\ShoppingSec\ShClMainSec.c
# End Source File
# Begin Source File

SOURCE=..\..\samples\ShoppingSec\ShClPlaceOrderCmn.c
# End Source File
# Begin Source File

SOURCE=..\..\samples\ShoppingSec\ShClPlaceOrderSec.c
# End Source File
# Begin Source File

SOURCE=..\..\samples\ShoppingSec\ShClProductList.c
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
