; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CDicClientView
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "dicclient.h"
LastPage=0

ClassCount=5
Class1=CDicClientApp
Class2=CAboutDlg
Class3=CDicClientDoc
Class4=CDicClientView
Class5=CMainFrame

ResourceCount=3
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Resource3=IDD_DIALOGBAR

[CLS:CDicClientApp]
Type=0
BaseClass=CWinApp
HeaderFile=dicClient.h
ImplementationFile=dicClient.cpp

[CLS:CAboutDlg]
Type=0
BaseClass=CDialog
HeaderFile=dicClient.cpp
ImplementationFile=dicClient.cpp
LastObject=CAboutDlg

[CLS:CDicClientDoc]
Type=0
BaseClass=CDocument
HeaderFile=dicClientDoc.h
ImplementationFile=dicClientDoc.cpp

[CLS:CDicClientView]
Type=0
BaseClass=CHtmlView
HeaderFile=dicClientView.h
ImplementationFile=dicClientView.cpp
LastObject=IDC_DICLIST
Filter=D
VirtualFilter=7VWC

[CLS:CMainFrame]
Type=0
BaseClass=CFrameWnd
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[TB:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_CUT
Command5=ID_EDIT_COPY
Command6=ID_EDIT_PASTE
Command7=ID_FILE_PRINT
Command8=ID_APP_ABOUT
CommandCount=8

[MNU:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_SAVE_AS
Command5=ID_FILE_MRU_FILE1
Command6=ID_APP_EXIT
Command7=ID_EDIT_UNDO
Command8=ID_EDIT_CUT
Command9=ID_EDIT_COPY
Command10=ID_EDIT_PASTE
Command11=ID_VIEW_TOOLBAR
Command12=ID_VIEW_STATUS_BAR
Command13=ID_APP_ABOUT
CommandCount=13

[ACL:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_EDIT_COPY
Command2=ID_FILE_NEW
Command3=ID_FILE_OPEN
Command4=ID_FILE_SAVE
Command5=ID_EDIT_PASTE
Command6=ID_EDIT_UNDO
Command7=ID_EDIT_CUT
Command8=ID_NEXT_PANE
Command9=ID_PREV_PANE
Command10=ID_EDIT_COPY
Command11=ID_EDIT_PASTE
Command12=IDC_SEARCH
Command13=ID_EDIT_CUT
Command14=ID_EDIT_UNDO
CommandCount=14

[DLG:IDD_DIALOGBAR]
Type=1
Class=CDicClientView
ControlCount=9
Control1=IDC_STATIC,static,1342308352
Control2=IDC_SEARCH,button,1342242816
Control3=IDC_KEYWORD,edit,1350631552
Control4=IDC_DICLIST,combobox,1344340227
Control5=IDC_STATIC,static,1342308352
Control6=IDC_SEARCHOPTION,combobox,1344340227
Control7=IDC_STATIC,static,1342308352
Control8=IDC_ITEMLIST,listbox,1352728835
Control9=IDC_STATIC,static,1342308352

