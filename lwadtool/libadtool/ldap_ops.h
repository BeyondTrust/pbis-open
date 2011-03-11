/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        ldap.h
 *
 * Abstract:
 *
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: Apr 2, 2010
 *
 */

#ifndef _ADTOOL_LDAP_H_
#define _ADTOOL_LDAP_H_

struct ldapmsg
{
    ber_int_t       lm_msgid;   /* the message id */
    ber_tag_t       lm_msgtype; /* the message type */
    BerElement*     lm_ber;     /* the ber encoded message contents */
    struct ldapmsg* lm_chain;   /* for search - next msg in the resp */
    struct ldapmsg* lm_chain_tail;
    struct ldapmsg* lm_next;    /* next response */
    time_t          lm_time;    /* used to maintain cache */
};

typedef enum ObjectClass {
    ObjectClassUser = 0,   /* class: user; id: samAccountName */
    ObjectClassComputer,   /* class: computer; id: samAccountName$ */
    ObjectClassGroup,      /* class: group; id: samAccountName */
    ObjectClassOU,         /* class: organizationalUnit; id: ou */
    ObjectClassCell,       /* class: container; id: cn=$LikewiseIdentityCel */
    ObjectClassCellUsers,  /* class: container; id: cn=Users (child of ObjectClassCell) */
    ObjectClassCellGroups, /* class: container; id: cn=Groups (child of ObjectClassCell) */
    ObjectClassCellUser,   /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellUsers) */
    ObjectClassCellGroup,  /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellUsers) */
    ObjectClassCellUserNS, /* class: serviceConnectionPoint; id: cn (child of ObjectClassCellUsers) Non-schema mode. */
    ObjectClassCellGroupNS,/* class: serviceConnectionPoint; id: cn (child of ObjectClassCellUsers) Non-schema mode. */
    ObjectClassAny         /* class: *; id: cn */
} ObjectClassT;

typedef struct ObjectClassAttrVal {
    ObjectClassT obj;
    PSTR vals[10];
} ObjectClassAttrValT;

typedef struct AttrVals {
    PSTR attr;
    PSTR *vals;
} AttrValsT;

typedef struct Sid {
   UCHAR Revision;
   UCHAR SubAuthorityCount;
   UCHAR IdentifierAuthority[6];
   ULONG SubAuthority[];
} SidT, *SidTP;

/**
 * Connect to AD server.
 *
 * @param appContext Application context reference.
 * @return 0 on success; error code on failure.
 */
extern DWORD ConnectAD(IN AppContextTP appContext);

/**
 * Close all connections to AD server.
 *
 * @param appContext Application context reference.
 */
extern VOID CloseADConnections(IN AppContextTP appContext);

/**
 * Close connection to AD server.
 *
 * @param appContext Application context reference.
 * @param conn Connection.
 */
extern VOID CloseADConnection(IN AppContextTP appContext, ADServerConnectionTP conn);

/**
 * Resolve DN of the object.
 *
 * @param appContext Application context reference.
 * @param class Object class
 * @param rdn Object's DN or RDN
 * @param outDN Resolved DN or NULL on failure. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
extern DWORD ResolveDN(IN AppContextTP appContext, IN ObjectClassT class, IN PSTR rdn, OUT PSTR *outDN);

/**
 * Get DNs of all children of the provided object that are of the specified type.
 *
 * @param appContext Application context reference.
 * @param class Object class of the children
 * @param rdn Object's DN or RDN
 * @param outDNs Null terminated array of all children. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
extern DWORD GetChildren(IN AppContextTP appContext, IN ObjectClassT class, IN PSTR rdn, OUT PSTR **outDNs);

/**
 * Search for children of the provided object that are of the specified type.
 *
 * @param appContext Application context reference.
 * @param class Object class of the children
 * @param rdn Object's DN or RDN
 * @param filterOpt Optional filter
 * @param scope Search scope
 * @param outDNs Null terminated array of all children. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
extern DWORD SearchForObject(IN AppContextTP appContext, IN ObjectClassT class, IN INT scope, IN PSTR filterOpt, IN PSTR rdn, OUT PSTR **outDNs);

/**
 * Find object in AD.
 *
 * @param appContext Application context reference.
 * @param class Object class
 * @param inName Identification component
 * @param outDN Resolved DN or NULL on failure. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
extern DWORD LocateADObject(IN AppContextTP appContext, IN ObjectClassT class, IN PSTR inName, OUT PSTR *outDN);

/**
 * Get various properties of the root DSE.
 *
 * @param appContext Application context reference.
 * @return 0 on success; error code on failure.
 */
extern DWORD GetRootDseProps(IN AppContextTP appContext);

/**
 * Create a new object in AD.
 *
 * @param appContext Application context reference.
 * @param avp Attribute-values pairs.
 * @param dn Object's DN.
 * @return 0 on success; error code on failure.
 */
extern DWORD CreateADObject(IN AppContextTP appContext, IN PSTR dn, IN AttrValsT *avp);

/**
 * Get values of the ObjectClass attribute.
 *
 * @param class Object class
 * @return Object class values.
 */
extern PSTR* GetObjectClassVals(ObjectClassT class);

/**
 * Get object's GUID.
 *
 * @param appContext Application context reference.
 * @param dn Object class
 * @param guid Object's guid. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
extern DWORD GetObjectGUID(IN AppContextTP appContext, IN PSTR dn, OUT PSTR *guid);

/**
 * Get object's SID.
 *
 * @param appContext Application context reference.
 * @param dn Object class
 * @param guid Object's SID. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
extern DWORD GetObjectSID(IN AppContextTP appContext, IN PSTR dn, OUT PSTR *sid);

/**
 * Get object's Likewise ID.
 *
 * @param appContext Application context reference.
 * @param dn Object class
 * @param id Object's hashed ID. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
extern DWORD GetObjectDerivedID(IN AppContextTP appContext, IN PSTR dn, OUT PSTR *sid);

/**
 * Get object's SID in raw form.
 *
 * @param appContext Application context reference.
 * @param dn Object class
 * @param sid Object's SID. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
extern DWORD GetObjectSIDBytes(IN AppContextTP appContext, IN PSTR dn, OUT PVOID *sid);

/**
 * Modify attributes of an object in AD.
 *
 * @param appContext Application context reference.
 * @param avp Attribute-values pairs.
 * @param dn Object's DN.
 * @param opType Operation type: 0 - add; 2 - replace; 1 - delete.
 * @return 0 on success; error code on failure.
 */
extern DWORD
ModifyADObject(IN AppContextTP appContext, IN PSTR dn, IN AttrValsT *avp, IN INT opType);

/**
 * Get object's attributes.
 *
 * @param appContext Application context reference.
 * @param dn Object class
 * @param avp Attribute-values pairs. (Dynamically allocated).
 * @return 0 on success; error code on failure.
 */
extern DWORD GetObjectAttrs(IN AppContextTP appContext, IN PSTR dn, OUT AttrValsT *avp);

/**
 * Get all object's attributes.
 *
 * @param appContext Application context reference.
 * @param dn Object DN
 * @param avp Attribute-values pairs. (Dynamically allocated. Caller must free avp).
 * @return 0 on success; error code on failure.
 */
extern DWORD GetAllObjectAttrs(IN AppContextTP appContext, IN PSTR dn, OUT AttrValsT **avp);

/**
 * Delete an AD object.
 *
 * @param appContext Application context reference.
 * @param dn Object to delete.
 * @param isRecursive If != 0 - delete all children.
 * @return 0 on success; error code on failure.
 */
extern DWORD DeleteADObject(IN AppContextTP appContext, IN PSTR dn, IN INT isRecursive);

/**
 * Locate AD user.
 *
 * @param appContext Application context reference.
 * @param name Name of the user. Can be in any format. Will be set to DN on success.
 * @return 0 on success; error code on failure.
 */
extern DWORD LocateADUser(IN AppContextTP appContext, IN OUT PSTR *name);

/**
 * Locate AD computer.
 *
 * @param appContext Application context reference.
 * @param name Name of the computer. Can be in any format. Will be set to DN on success.
 * @return 0 on success; error code on failure.
 */
extern DWORD LocateADComputer(IN AppContextTP appContext, IN OUT PSTR *name);

/**
 * Locate AD group.
 *
 * @param appContext Application context reference.
 * @param name Name of the user. Can be in any format. Will be set to DN on success.
 * @return 0 on success; error code on failure.
 */
extern DWORD LocateADGroup(IN AppContextTP appContext, IN OUT PSTR *name);

/**
 * Check whether the cell specified by dn parameter is in non-schema mode.
 *
 * @param appContext Application context reference.
 * @param name Name of the user. Can be in any format. Will be set to DN on success.
 * @param res Will be set to TRUE is the cell is in non-schema mode.
 * @return 0 on success; error code on failure.
 */
extern DWORD IsCellInNonSchemaMode(IN AppContextTP appContext, IN PSTR dn, OUT BOOL *res);

/**
 * Get all cells the user is a member of.
 *
 * @param appContext Application context reference.
 * @param sb Search base. If NULL - the default naming context will be used.
 * @param name Name of the user.
 * @param cells DNs of the cells the user is a member of.
 * @return 0 on success; error code on failure.
 */
extern DWORD GetCellsForUser(IN AppContextTP appContext, IN PSTR sb, IN PSTR name, OUT PSTR **cells);

/**
 * Get all cells the group is a member of.
 *
 * @param appContext Application context reference.
 * @param sb Search base. If NULL - the default naming context will be used.
 * @param name Name of the group.
 * @param cells DNs of the cells the user is a member of.
 * @return 0 on success; error code on failure.
 */
extern DWORD GetCellsForGroup(IN AppContextTP appContext, IN PSTR sb, IN PSTR name, OUT PSTR **cells);

/**
 * Rename/move object.
 *
 * @param appContext Application context reference.
 * @param dn DN of the object to move.
 * @param newRDN New RDN of the object.
 * @param newParent New parent DN of the object.
 * @return 0 on success; error code on failure.
 */
extern DWORD MoveADObject(IN AppContextTP appContext, IN PSTR dn, IN PSTR newRDN, IN PSTR newParent);

#endif /* _ADTOOL_LDAP_H_ */
