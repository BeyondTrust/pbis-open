# Microsoft Developer Studio Project File - Name="ServerInfo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** ï“èWÇµÇ»Ç¢Ç≈Ç≠ÇæÇ≥Ç¢ **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ServerInfo - Win32 Debug
!MESSAGE Ç±ÇÍÇÕóLå¯Ç»“≤∏Ãß≤ŸÇ≈ÇÕÇ†ÇËÇ‹ÇπÇÒÅB Ç±ÇÃÃﬂ€ºﬁ™∏ƒÇÀﬁŸƒﬁÇ∑ÇÈÇΩÇﬂÇ…ÇÕ NMAKE ÇégópÇµÇƒÇ≠ÇæÇ≥Ç¢ÅB
!MESSAGE [“≤∏Ãß≤ŸÇÃ¥∏ΩŒﬂ∞ƒ] ∫œ›ƒﬁÇégópÇµÇƒé¿çsÇµÇƒÇ≠ÇæÇ≥Ç¢
!MESSAGE 
!MESSAGE NMAKE /f "ServerInfo.mak".
!MESSAGE 
!MESSAGE NMAKE ÇÃé¿çséûÇ…ç\ê¨ÇéwíËÇ≈Ç´Ç‹Ç∑
!MESSAGE ∫œ›ƒﬁ ◊≤›è„Ç≈œ∏€ÇÃê›íËÇíËã`ÇµÇ‹Ç∑ÅBó·:
!MESSAGE 
!MESSAGE NMAKE /f "ServerInfo.mak" CFG="ServerInfo - Win32 Debug"
!MESSAGE 
!MESSAGE ëIëâ¬î\Ç»ÀﬁŸƒﬁ ”∞ƒﬁ:
!MESSAGE 
!MESSAGE "ServerInfo - Win32 Debug" ("Win32 (x86) Dynamic-Link Library" óp)
!MESSAGE "ServerInfo - Win32 Unicode Debug" ("Win32 (x86) Dynamic-Link Library" óp)
!MESSAGE "ServerInfo - Win32 Release MinSize" ("Win32 (x86) Dynamic-Link Library" óp)
!MESSAGE "ServerInfo - Win32 Release MinDependency" ("Win32 (x86) Dynamic-Link Library" óp)
!MESSAGE "ServerInfo - Win32 Unicode Release MinSize" ("Win32 (x86) Dynamic-Link Library" óp)
!MESSAGE "ServerInfo - Win32 Unicode Release MinDependency" ("Win32 (x86) Dynamic-Link Library" óp)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ServerInfo - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - ìoò^ÇçsÇ¡ÇƒÇ¢Ç‹Ç∑
OutDir=.\Debug
TargetPath=.\Debug\ServerInfo.dll
InputPath=.\Debug\ServerInfo.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ServerInfo - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - ìoò^ÇçsÇ¡ÇƒÇ¢Ç‹Ç∑
OutDir=.\DebugU
TargetPath=.\DebugU\ServerInfo.dll
InputPath=.\DebugU\ServerInfo.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo åxçê : Windows 95 Ç… Unicode DLL Çìoò^Ç∑ÇÈÇ±Ç∆ÇÕÇ≈Ç´Ç‹ÇπÇÒ 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ServerInfo - Win32 Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinSize"
# PROP BASE Intermediate_Dir "ReleaseMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinSize"
# PROP Intermediate_Dir "ReleaseMinSize"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - ìoò^ÇçsÇ¡ÇƒÇ¢Ç‹Ç∑
OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\ServerInfo.dll
InputPath=.\ReleaseMinSize\ServerInfo.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ServerInfo - Win32 Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinDependency"
# PROP BASE Intermediate_Dir "ReleaseMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinDependency"
# PROP Intermediate_Dir "ReleaseMinDependency"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - ìoò^ÇçsÇ¡ÇƒÇ¢Ç‹Ç∑
OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\ServerInfo.dll
InputPath=.\ReleaseMinDependency\ServerInfo.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ServerInfo - Win32 Unicode Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinSize"
# PROP BASE Intermediate_Dir "ReleaseUMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUMinSize"
# PROP Intermediate_Dir "ReleaseUMinSize"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - ìoò^ÇçsÇ¡ÇƒÇ¢Ç‹Ç∑
OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\ServerInfo.dll
InputPath=.\ReleaseUMinSize\ServerInfo.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo åxçê : Windows 95 Ç… Unicode DLL Çìoò^Ç∑ÇÈÇ±Ç∆ÇÕÇ≈Ç´Ç‹ÇπÇÒ 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ServerInfo - Win32 Unicode Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinDependency"
# PROP BASE Intermediate_Dir "ReleaseUMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUMinDependency"
# PROP Intermediate_Dir "ReleaseUMinDependency"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - ìoò^ÇçsÇ¡ÇƒÇ¢Ç‹Ç∑
OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\ServerInfo.dll
InputPath=.\ReleaseUMinDependency\ServerInfo.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo åxçê : Windows 95 Ç… Unicode DLL Çìoò^Ç∑ÇÈÇ±Ç∆ÇÕÇ≈Ç´Ç‹ÇπÇÒ 
	:end 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "ServerInfo - Win32 Debug"
# Name "ServerInfo - Win32 Unicode Debug"
# Name "ServerInfo - Win32 Release MinSize"
# Name "ServerInfo - Win32 Release MinDependency"
# Name "ServerInfo - Win32 Unicode Release MinSize"
# Name "ServerInfo - Win32 Unicode Release MinDependency"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\DiskInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerInfo.def
# End Source File
# Begin Source File

SOURCE=..\ServerInfo.idl

!IF  "$(CFG)" == "ServerInfo - Win32 Debug"

# ADD MTL /tlb "..\ServerInfo.tlb" /h "..\ServerInfo.h" /iid "..\ServerInfo_i.c"

!ELSEIF  "$(CFG)" == "ServerInfo - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "ServerInfo - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "ServerInfo - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "ServerInfo - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "ServerInfo - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ServerInfo.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\DiskInfo.h
# End Source File
# Begin Source File

SOURCE=..\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\DiskInfo.rgs
# End Source File
# Begin Source File

SOURCE=.\ServerInfo.rgs
# End Source File
# End Group
# End Target
# End Project
