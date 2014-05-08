PROGRAM=Likewise\ Domain\ Join
TYPE=app

INFO=Info.plist

SOURCES=\
    HIFramework/TApplication.cp \
    HIFramework/TCarbonEvent.cp \
    HIFramework/TEventHandler.cp \
    HIFramework/TNib.cp \
    HIFramework/TSheet.cp \
    HIFramework/TWindow.cp \
    CredentialsDialog.cp \
    DomainJoinException.cp \
    DomainJoinInterface.cp \
    DomainJoinWindow.cp \
    DomainLeaveWindow.cp \
    DomainMigrateWindow.cp \
    MainWindow.cp \
    main.cp

REZ_FILES=\
    App.rsrc \
    keys.rsrc

RESOURCES=\
    InfoPlist.strings \
    main.nib \
    Credentials.nib \
    Leave.nib \
    Join.nib \
    DomainJoin.icns \
    Migrate.nib

LOCAL_CFLAGS=-IHIFramework $(_CPPFLAGS) $(_CFLAGS)
LOCAL_LDFLAGS=-framework Carbon -framework Security -framework DirectoryService $(_LDFLAGS)

include Makefile.inc
