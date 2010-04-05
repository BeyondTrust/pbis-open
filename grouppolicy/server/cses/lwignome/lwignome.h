#ifndef __LWIGNOME_H__
#define __LWIGNOME_H__

#define LWIGNOME_USER_CLIENT_GUID       "{74533AFA-5A94-4fa5-9F88-B78667C1C0B5}"
#define LWIGNOME_USER_ITEM_GUID         "{CA01F1B8-6435-4607-BAE6-0C444413982B}"

#define LWIGNOME_MACHINE_CLIENT_GUID    "{B078EE20-01A1-4fee-8DCC-032B758FA1F8}"
#define LWIGNOME_MACHINE_ITEM_GUID      "{0375934E-D4C7-40f2-B1D8-21D21E87D430}"

#define LWIGNOME_IDT_LWI_NEW       ".gconf.xml.lwi.mandatory.new"
#define LWIGNOME_IDT_LWI           ".gconf.xml.lwi.mandatory" 

#define LWIGNOME_MAN_DIR        "gconf.xml.mandatory"
#define LWIGNOME_MAN_DIR_NEW    "gconf.xml.mandatory.new"

CENTERROR
GPAGconfAvailable(
    PBOOLEAN Result
    );

CENTERROR
GPASignalGconfDaemon();

CENTERROR
ProcessGnomeGroupPolicy(
    DWORD dwFlags,
    PGPUSER pUser,
    PGROUP_POLICY_OBJECT pGPOModifiedList,
    PGROUP_POLICY_OBJECT pGPODeletedList
    );

CENTERROR
ResetGnomeGroupPolicy(
        PGPUSER pUser
        );

#endif /* __LWIGNOME_H__ */
