adtool(8) - AD tool to manage objects in Active Directory
==========================================================================================

USER SYNOPSIS
--------

`adtool` [<options>] -a search-user --name <name> [<actionoptions>]

`adtool` [<options>] -a new-user --logon-name <name> (--cn|--first-name|--last-name <name>) [<actionoptions>]

`adtool` [<options>] -a reset-user-password --name <name> [<actionoptions>]

`adtool` [<options>] -a unlock-account (--user|--computer <name>)

`adtool` [<options>] -a enable-user --name <name>

`adtool` [<options>] -a disable-user --name <name>

GROUP SYNOPSIS
--------

`adtool` [<options>] -a search-groups --name <name> [<actionoptions>]

`adtool` [<options>] -a new-group --name <name> [<actionoptions>]

`adtool` [<options>] -a add-to-group (--user|--group <name>) --to-group <target-group>

`adtool` [<options>] -a remove-from-group (--user|--group <name>) --from-group <name>

COMPUTER SYNOPSIS
--------

`adtool` [<options>] -a search-computer --name <name> [<actionoptions>]

`adtool` [<options>] -a new-computer --name <name> [<actionoptions>]

OU SYNOPSIS
--------

`adtool` [<options>] -a search-ou --name <name> [<actionoptions>]

`adtool` [<options>] -a new-ou --dn <DN> [<actionoptions>]

OBJECT SYNOPSIS
--------

`adtool` [<options>] -a search-objects [<actionoptions>]

`adtool` [<options>] -a lookup-object --dn <DN> [<actionoptions>]

`adtool` [<options>] -a move-object --from <DN/RDN> --to <DN/RDN>

`adtool` [<options>] -a delete-object --dn <DN> [--force]

`adtool` [<options>] -a set-attr --dn <DN> --attrName <name> [--attrValue <value>]
[ENTERPRISE]

CELL SYNOPSIS
--------

`adtool` [<options>] -a search-cells [<actionoptions>]

`adtool` [<options>] -a new-cell --dn <DN> [<actionoptions>]

`adtool` [<options>] -a add-to-cell --dn <DN> (--user|--group <name>)

`adtool` [<options>] -a remove-from-cell --dn <DN> (--user|--group <name>) [--force]

`adtool` [<options>] -a edit-cell --dn <DN> (--home-dir-template|--default-login-shell <value>)

`adtool` [<options>] -a edit-cell-group --dn <DN> --group <name> (--gid|--group-alias|--description <value>)

`adtool` [<options>] -a edit-cell-user --dn <DN> --user <name> (--uid|--gid|--login-name|--home-dir|--login-shell|--description <value>)

`adtool` [<options>] -a lookup-cell --dn <DN> [<actionoptions>]

`adtool` [<options>] -a lookup-cell-group --dn <DN> --group <name> [<actionoptions>]

`adtool` [<options>] -a lookup-cell-user --dn <DN> --user <name> [<actionoptions>]

`adtool` [<options>] -a link-cell --source-dn <DN> --target-dn <DN>

`adtool` [<options>] -a unlink-cell --source-dn <DN> --target-dn <DN>

`adtool` [<options>] -a delete-cell --dn <DN> [--force]
[/ENTERPRISE]

DESCRIPTION
-----------

**adtool** can create, query and modify objects in Active Directory. **adtool** can also be utilized to create, find and manage objects in cells.

**NOTE: AUTHENTICATION REQUIRED -** This tool is for managing Active Directory objects which requires AD rights. It is **REQUIRED** to be logged in as an AD user with rights to manage AD before using the **adtool**. Alternatively the --domain **CONNECTION OPTION** with an **AUTHENTICATION OPTION** can be used to connect to the domain when logged in as a local user.

HELP OPTIONS
-------

`-u|--usage`

[INDENT]Display brief usage message.

`-?|--help`

[INDENT]Displays brief usage and help information. Help can list all actions(-a), or help on a specific action (-a <action>).

`-v|--version`

[INDENT]Print program version.

COMMON OPTIONS
-------

`-l|--log-level` <int>

[INDENT]Acceptable values: 1(error), 2(warning), 3(info), 4(verbose) 5(trace). Default: 2(warning).

`-q|--quiet`

[INDENT]Suppress printing to stdout. Just set the return code. print-dn option makes an exception.

`-t|--print-dn`

[INDENT]Print DNs of the objects to be looked up, modified or searched for.

`-r|--read-only`

[INDENT]Do not actually modify directory objects when executing actions.

CONNECTION OPTIONS
-------

`-s|--server` <string>

[INDENT]Active Directory server to connect to.

`-d|--domain` <string>

[INDENT]Domain to connect to.

`-p|--port` <int>

[INDENT]TCP port number

`-m|--non-schema`

[INDENT]Turn off schema mode

AUTHENTICATION OPTIONS
-------

`-n|--logon-as` <string>

[INDENT]User name or UPN.

`-x|--passwd` <string>

[INDENT]Password for authentication. (use '-' for stdin input)

`-k|--keytab` <string>

[INDENT]Full path of keytab file, e.g. /etc/krb5.keytab

`-c|--krb5cc` <string>        

[INDENT]Full path of krb5 ticket cache file, e.g. /tmp/krb5cc_foo@centeris.com

`-z|--no-sec`

[INDENT]Turns off secure authentication. Simple bind will be used. Use with caution!

ACTION
-------

`-a|--action` <action>

[INDENT]Action to execute. Type '--help -a' for a list of actions, or '--help -a <action>' for information on a specific action.

USER ACTIONS
-------------

**SEARCH-USER**

`adtool` [<options>] -a search-user --name <string> [--search-base <string>] [--scope <string>]

[INDENT]Search for users, print DNs.

`--search-base` <string>

[INDENT]DN of top-level node to start the search from. (Default:  rootDomainNamingContext of the DC the client connects to)

`--scope` <string>

[INDENT]Search scope. Acceptable values: base, one-level, subtree. Default: subtree

`--name` <string>

[INDENT]Name of the user (DN/RDN, UPN, or SamAccountName). Wildcards   (*) accepted as part of the name.

**EXAMPLE**

[INDENT]Look up "unixHomeDirectory" attribute of a user with samAccountName TestUser.

[INDENT]`adtool -a search-user --name TestUser -t | adtool -a lookup-object --dn - --attr unixHomeDirectory`


**NEW-USER**

`adtool` [<options>] -a new-user --logon-name <string> (--cn|--first-name|--last-name <string>) [--dn <string>] [--pre-win-2000-name <string>] [--description <string>] [-- <string>] [--description <string>] [--description <string>] [--password <string>] [--spn <string>] [--keytab-file <string>] [--no-must-change-password] [--no-password-expires] [--account-enabled]

[INDENT]Create a new user account.

`--dn` <string>

[INDENT]DN/RDN of the parent container/OU containing the user. (use '-' for stdin input)

`--cn` <string>

[INDENT]Common name (CN) of the new user. (use '-' for stdin input)

`--logon-name` <string>

[INDENT]Logon name of the new user. Sets upn attribute. (use '-' for stdin input)

`--pre-win-2000-name` <string>

[INDENT]Pre Windows-2000 logon name (sAMAccountName).

`--first-name` <string>

[INDENT]First name of the new user.

`--last-name` <string>

[INDENT]Last name of the new user.

`--description` <string>

[INDENT]Description of the user.

`--password` <string>

[INDENT]User's password. (use '-' for stdin input)

`--spn` <string>

[INDENT]Set new user account service principal name attribute. A comma separated list can be specified (eg. --spn "nfs, http/"). Default is an empty SPN attribute.

`--keytab-file` <string>

[INDENT]Generate a keytab file for the user. Specify /path/to/file.keytab.

`--no-must-change-password`

[INDENT]User is not required to change the password at next logon.

`--no-password-expires`

[INDENT]The password never expires.

`--account-enabled`                  

[INDENT]User account will be enabled. By default the account is disabled on creation.                          

**EXAMPLE**

[INDENT]Create a new user account TestUser in TestOu.

[INDENT]`adtool -a new-user --dn OU=TestOu --cn TestUser --logon-name TestUser --password=ChangeMe`


**RESET-USER-PASSWORD**

`adtool` [<options>] -a reset-user-password --name <string> [--password <string>] [--spn <string>] [--keytab-file <string>] [--no-must-change-password] [--no-password-expires]

[INDENT]Reset user's password.

`--name` <string>

[INDENT]User to change password for. (DN/RDN, UPN, or SamAccountName; use '-' for stdin input)

`--password` <string>

[INDENT]User's password. If omitted only the password's properties may be changed but not the password itself. (use '-' for stdin input)

`--spn` <string>

[INDENT]Modify user account service principal name attribute. A comma separated list can be specified (eg. --spn "nfs,http/").

`--keytab-file` <string>

[INDENT]Modify/Generate a keytab file for the user. Specify /path/to/file.keytab.

`--no-must-change-password`

[INDENT]User is not required to change the password at next logon. If omitted - user must change password at next logon unless "--no-password-expires" option is specified.

` --no-password-expires`

[INDENT]The password never expires.

**EXAMPLE**

[INDENT]Reset user's password reading the password from TestUser.pwd file.

[INDENT]`cat TestUser.pwd | adtool -a reset-user-password --name TestUser --password=- --no-password-expires`


**UNLOCK-ACCOUNT**

`adtool` [<options>] -a unlock-account (--user|--computer <string>)

[INDENT]Unlock user or computer account.

`--user` <string>

[INDENT]Name of the user (DN/RDN, UPN, or samAccountName; use '-' for stdin input)

`--computer` <string>

[INDENT]Computer name (DN/RDN, SPN, or SamAccountName; use '-' for stdin input).

**EXAMPLE**

[INDENT]Unlock the user account.

[INDENT]`adtool -a unlock-account --user TestUser`


**ENABLE-USER**

`adtool` [<options>] -a enable-user --name <string>

[INDENT]Enable a user account in Active Directory.

`--name` <string>

[INDENT]Name of the user (DN/RDN, UPN, or samAccountName; use '-' for stdin input)

**EXAMPLE**

[INDENT]Enable the user account.

[INDENT]`adtool -a enable-user --name TestUser`


**DISABLE-USER**

adtool [<options>] -a disable-user --name <string>

[INDENT]Disable a user account in Active Directory.

`--name` <string>

[INDENT]Name of the user (DN/RDN, UPN, or samAccountName; use '-' for stdin input)

**EXAMPLE**

[INDENT]Disable the user account.

[INDENT]`adtool -a disable-user --name TestUser`


GROUP ACTIONS
-------------

**SEARCH-GROUPS**

`adtool` [<options>] -a search-groups --name <string> [--scope <string>] [--name <string>]

[INDENT]Search for group objects, print DNs.

`--search-base` <string>

[INDENT]DN of top-level node to start the search from. (Default:  rootDomainNamingContext of the DC the client connects to)

`--scope` <string>

[INDENT]Search scope. Acceptable values: base, one-level, subtree. Default: subtree

`--name` <string>

[INDENT]Name of the group (DN/RDN, UPN, or SamAccountName).  Wildcards (*) accepted as part of the name.

**EXAMPLE**

[INDENT]Looks for group TestGroup under OU TestOU.

[INDENT]`adtool -a search-groups --name TestGroup --scope OU=TestOU`


**NEW-GROUP**

`adtool` [<options>] -a new-group --name <string> [--dn <string>] [--pre-win-2000-name <string>] [--type <string>] [--description <string>]

[INDENT]Create a new global security group.

`--dn` <string>

[INDENT]DN/RDN of the parent container/OU containing the group. (use '-' for stdin input)

`--name` <string>

[INDENT]Name of the group. (use '-' for stdin input)

`--pre-win-2000-name` <string>

[INDENT]Pre Windows-2000 logon name (sAMAccountName).

`--type` <string>

[INDENT]Group type. Acceptable values: domain-local, global, universal. Default: global

`--description` <string>

[INDENT]Description of the group.

**EXAMPLE**

[INDENT]Create a new group.

[INDENT]`adtool -a new-group --dn OU=TestOu --pre-win-2000-name TestGroup --name TestGroup`


**ADD-TO-GROUP**

`adtool` [<options>] -a add-to-group (--user|--group <string>) --to-group <string>

[INDENT]Add a domain user/group to a security group. Either --user or --group need to be stated.

`--user` <string>

[INDENT]User to add to the group (DN/RDN, UPN, or SamAccountName; use '-' for stdin input).

`--group` <string>

[INDENT]Group to add to the group (DN/RDN, or CN; use '-' for stdin input).

`--to-group` <string>

[INDENT]Group to add user or group to (DN/RDN , or CN; use '-' for stdin input)

**EXAMPLE**

[INDENT]Add user TestUser to group TestGroup.

[INDENT]`adtool -a add-to-group --user TestUser --to-group TestGroup`


**REMOVE-FROM-GROUP**

`adtool` [<options>] -a remove-from-group (--user|--group <string>) --from-group <string>

[INDENT]Remove a user/group from a security group.

`--user` <string>   

[INDENT]User to remove from the group (DN/RDN, UPN, or SamAccountName; use '-' for stdin input).

`--group` <string>

[INDENT]Group to remove from the group (DN/RDN, or CN; use '-' for stdin input).

`--from-group` <string>

[INDENT]Group to remove user or group from (DN/RDN , or CN; use '-' for stdin input)

**EXAMPLE**

[INDENT]Remove user TestUser from group TestGroup.

[INDENT]`adtool -a remove-from-group --user TestUser --from-group TestGroup`


COMPUTER ACTIONS
-------------

**SEARCH-COMPUTER**

`adtool` [<options>] -a search-computer --name <string> [--scope <string>] [--name <string>]

[INDENT]Search for computer objects, print DNs.

`--search-base` <string>

[INDENT]DN of top-level node to start the search from. (Default: rootDomainNamingContext of the DC the client connects to)

`--scope` <string>

[INDENT]Search scope. Acceptable values: base, one-level, subtree. Default: subtree

`--name` <string>

[INDENT]Name of the computer (DN/RDN, UPN, or SamAccountName).  Wildcards (*) accepted as part of the name.

**EXAMPLE**

[INDENT]Looks for computer TestComputer under OU TestOU.

[INDENT]`adtool -a search-computer --name TestComputer --scope OU=TestOU`


**NEW-COMPUTER**

`adtool` [<options>] -a new-computer --name <string> [--dn <string>] [--pre-win-2000-name <string>] [--description <string>] [--dnshostname <string>] [--password <string>] [--spn <string>] [--keytab-file <string>]

[INDENT]Create a new computer object.

`--dn` <string>

[INDENT]DN/RDN of the parent container/OU containing the computer. (use '-' for stdin input)

`--name`<string>

[INDENT]Name of the new computer. (use '-' for stdin input)

`--pre-win-2000-name` <string>

[INDENT]Pre Windows-2000 name (sAMAccountName).

`--description` <string>

[INDENT]Description of the computer

`--dnshostname` <string>

[INDENT]Fully-Qualified DNS name of the computer

`--password` <string>

[INDENT]Computer's password. (use '-' for stdin input)

`--spn` <string>

[INDENT]Set new computer account service principal name attribute. A comma separated list can be specified (eg. --spn "nfs, host/"). Default is --spn "host". For an empty SPN attribute use --spn "".

`--keytab-file` <string>

[INDENT]Generate a keytab file for the computer. Specify /path/to/file.keytab. By default keytab file is generated with "host" service class

**EXAMPLE**

[INDENT]Create a new computer under the computers container.

[INDENT]`adtool -a new-computer --name TestComputer`


OU ACTIONS
-------------

**SEARCH-OU**

`adtool` [<options>] -a search-ou --name <string> [--search-base <string>] [--scope <string>]

[INDENT]Search for organizational units, print DNs

`--search-base` <string>

[INDENT]DN of top-level node to start the search from. (Default:  rootDomainNamingContext of the DC the client connects to)

`--scope` <string>

[INDENT]Search scope. Acceptable values: base, one-level, subtree. Default: subtree

`--name` <string>

[INDENT]Name of the OU (DN/RDN, or CN). Wildcards (*) accepted as part of the name.

**EXAMPLE**

[INDENT]Look up "description" attribute of an OU specified by name with a wildcard.

[INDENT]`adtool -a search-ou --name '*Ou' -t | adtool -a lookup-object --dn - --attr description`


**NEW-OU**

`adtool` [<options>] -a new-ou --dn <distinguished name> [--name <string>] [--description <string>]

[INDENT]Create a new organizational unit.

`--dn` <string>

[INDENT]DN/RDN of the new OU or DN/RDN of the parent if "--name" is present. (use '-' for stdin input)

`--name` <string>

[INDENT]Name of the new organizational unit. (use '-' for stdin input)

`--description` <string>

[INDENT]Description of the organizational unit

**EXAMPLE**

[INDENT]Create OU in a root naming context.

[INDENT]`adtool -a new-ou --dn OU=TestOu`


OBJECT ACTIONS
-------------

**SEARCH-OBJECTS**

`adtool` [<options>] -a search-objects [--scope <string>] [--fliter <string>]

[INDENT]Search for any type of objects using LDAP filter.

`--search-base` <string>

[INDENT]DN of top-level node to start the search from. (Default: rootDomainNamingContext of the DC the client connects to)

`--scope` <string>

[INDENT]Search scope. Acceptable values: base, one-level, subtree. Default: subtree

`--filter` <string>

[INDENT]LDAP search filter (RFC 2254). Return all entries if omitted  (Default: (objectClass=top))

**EXAMPLE**

[INDENT]Look up all attributes of an AD object using filter-based search.

[INDENT]`adtool -a search-object --filter '(&(objectClass=person)(displayName TestUser))' -t | adtool -a lookup-object`


**LOOKUP-OBJECT**

`adtool` [<options>] -a lookup-object --dn <string> [--attr <string>] [--raw-time] [--all]

[INDENT]Retrieve object attributes.

`--dn` <string>

[INDENT]DN/RDN of the object to look up. (use '-' for stdin input)

`--attr`  <string>

[INDENT]Attribute to show values of

`--raw-time`

[INDENT]Do not format timestamps (show raw time data)

`--all`

[INDENT]Show values of all object attributes

**EXAMPLE**

[INDENT]Look up OU's GUID

[INDENT]`adtool -a lookup-object --dn OU=TestOU --attr objectGUID`


**MOVE-OBJECT**

`adtool` [<options>] -a move-object --from <string> --to <string>

[INDENT]Move/rename an object.

`--from` <string>

[INDENT]DN/RDN of the object to move/rename. (use '-' for stdin input)

`--to` <string>

[INDENT]DN/RDN of the new object. (use '-' for stdin input)

**EXAMPLE**

[INDENT]Move computer object under the computers container to TestOU.

[INDENT]`adtool -a move-object --from CN=TestComputer,CN=Computers --to CN=TestComputer,OU=TestOU`


**DELETE-OBJECT**

`adtool` [<options>] -a delete-object --dn <string> [--force]

[INDENT]Delete an object.

`--dn` <string>

[INDENT]DN/RDN of the object to delete. (use '-' for stdin input)

`--force`

[INDENT]Remove all children first. Default: fail if the object has any child nodes

**EXAMPLE**

[INDENT]Delete OU and all its children if any (--force).

[INDENT]`adtool -a delete-object --dn OU=TestOU --force`


**SET-ATTR**

`adtool` [<options>] -a set-attr --dn <string> --attrName <string> [--attrValue <value>]

[INDENT]Set/un-set attribute.

`--dn` <string>

[INDENT]DN/RDN of the object.

` --attrName` <string>

[INDENT]Name of attribute.

`--attrValue` <string>

[INDENT]Value of attribute. Multi-value attributes are delimited with a semi-colon. To unset an attribute do not provide the attrValue argument.

**EXAMPLE**

[INDENT]Modify a multi-value attribute using a semi-colon as the delimiter. Note: Attribute value validation is not done. Use with care.

[INDENT]`adtool -a set-attr --dn CN=TestUser,OU=TestOU --attrName otherHomePhone --attrValue "546-872-8383;453-857-9844;954-723-9765"`

[INDENT]Unset an attribute. Note: Attribute value validation is not done. Use with care.

[INDENT]`adtool -a set-attr --dn CN=TestUser,CN=Users,DC=company,DC=com --attrName displayName`
[ENTERPRISE]


CELL ACTIONS
-------------

**SEARCH-CELLS**

`adtool` [<options>] -a search-cells [--search-base <string>] [--scope <string>] [--user <string>] [--group <string>]

[INDENT]Search for AD Bridge Cells.

`--search-base` <string>

[INDENT]DN of top-level node to start the search from. (Default: rootDomainNamingContext of the DC the client connects to)

`--scope` <string>

[INDENT]Search scope. Acceptable values: base, one-level, subtree.  Default: subtree

`--user` <string>

[INDENT]Search for cells the user is a member of (DN/RDN, UPN, or  SamAccountName; use '-' for stdin input).

`--group` <string>

[INDENT]Search for cells the group is a member of (DN/RDN, or CN; use '-' for stdin input).

**EXAMPLE**

[INDENT]Search for AD Bridge Cells in root naming context containing user TestUser.

[INDENT]`adtool -a search-cells --user TestUser`


**NEW-CELL**

`adtool` [<options>] -a new-cell --dn <string> [--home-dir-template <string>] [--default-login-shell <string>]

[INDENT]Create a new AD Bridge Cell.

`--dn` <string>

[INDENT]DN/RDN of the Organizational Unit to create the cell in or DN of the root naming context to create a default cell. (use '-' for stdin input)

`--home-dir-template` <string>

[INDENT]Template for user's home directory. Default: %H/%D/%U

`--default-login-shell` <string>

[INDENT]Users's login shell. Default: /bin/bash

**EXAMPLE**

[INDENT]Create AD Bridge Cell in TestOU setting the default login shell property to /bin/sh.

[INDENT]`adtool -a new-cell --dn OU=TestOu --default-login-shell /bin/sh`


**ADD-TO-CELL**

`adtool` [<options>] -a add-to-cell --dn <string> (--user|--group <string>)

[INDENT]Add user/group to a AD Bridge Cell.

`--dn` <string>

[INDENT]OU containing the cell or DN of the root naming context to modify the default cell. (DN/RDN; use '-' for stdin input)

`--user` <string>

[INDENT]User to add to the cell (DN/RDN, UPN, or SamAccountName; use '-' for stdin input).

`--group` <string>

[INDENT]Group to add to the cell (DN/RDN, or CN; use '-' for stdin input).

**EXAMPLE**

[INDENT]Add group TestGroup to AD Bridge Cell.

[INDENT]`adtool -a add-to-cell --dn OU=TestOU --group TestGroup`

[INDENT]Add user TestGroup to AD Bridge Cell.

[INDENT]`adtool -a add-to-cell --dn OU=TestOU --user TestUser`



**REMOVE-FROM-CELL**

`adtool` [<options>] -a remove-from-cell --dn <string> --user|--group <string> [--force]

[INDENT]Remove user/group from a AD Bridge Cell.

`--dn` <string>

[INDENT]OU containing the cell or DN of the root naming context to modify the default cell. (DN/RDN; use '-' for stdin input)

`--user` <string>

[INDENT]User to remove from the cell (DN/RDN, UPN, or SamAccountName; use '-' for stdin input).

`--group` <string>

[INDENT]Group to remove from the cell (DN/RDN, or CN; use '-' for stdin  input).

`--force` <string> Unset values of uidNumber, gidNumber, loginShell, and unixHomeDirectory attributes when operating on a default cell

**EXAMPLE**

[INDENT]Remove user/group from AD Bridge Cell.

[INDENT]`adtool -a remove-from-cell --dn OU=TestOU --user TestUser`

[INDENT]`adtool -a remove-from-cell --dn OU=TestOU --group TestGroup`


**DELETE-CELL**

`adtool` [<options>] -a delete-cell --dn <string> [--force]

[INDENT]Delete a AD Bridge Cell.

`--dn` <string>

[INDENT]DN/RDN of the cell or OU containing the cell or DN of the root naming context to delete the default cell. (use '-' for stdin input)

`--force` <string>

[INDENT]Remove users/groups/maps from the cell first. Default: fail if cell is not empty

**EXAMPLE**

[INDENT]Delete the AD Bridge Cell TestOU.

[INDENT]`adtool -a delete-cell --dn OU=TestOU --force`


**EDIT-CELL**

`adtool` [<options>] -a edit-cell --dn <string> (--home-dir-template <string> | --default-login-shell <string>)

[INDENT]Modify AD Bridge Cell properties.

`--dn` <string>

[INDENT]DN/RDN of the Organizational Unit containing the cell or DN of the root naming context to edit the default cell. (use '-' for stdin input)

`--home-dir-template` <string>

[INDENT]Template for users' home directory. Default: %H/%D/%U

`--default-login-shell` <string>

[INDENT]Users' default login shell. Default: /bin/bash

**EXAMPLE**

[INDENT]Change the default login shell property of AD Bridge Cell.

[INDENT]`adtool -a edit-cell --dn OU=TestOU --default-login-shell=/bin/bash`


**EDIT-CELL-GROUP**

`adtool` [<options>] -a edit-cell-group --dn <string> --group <string> (--gid <string> | --group-alias <string> | --description <string>)

[INDENT]Modify properties of a cell's group.

`--dn` <string>

[INDENT]DN/RDN of the Organizational Unit containing the cell or DN of the root naming context to edit group in the default cell. (use '-' for stdin input)

`--group` <string>

[INDENT]Group to change cell properties of. (DN/RDN, or CN; use '-' for stdin input).

`--gid` <string>

[INDENT]Group ID

`--group-alias` <string>

[INDENT]Group alias

`--description` <string>

[INDENT]Description of the group

**EXAMPLE**

[INDENT]Set group alias for group TestGroup in the cell TestOU.

[INDENT]`adtool -a edit-cell-group --dn OU=TestOU --group TestGroup --group-alias testers`


**EDIT-CELL-USER**

`adtool` [<options>] -a edit-cell-user --dn <string> --user <string> (--uid <string> | --gid <string> | --login-name <string> | --home-dir <string> | --login-shell <string> | --description <string>)

[INDENT]Modify properties of a cell's user.

`--dn` <string>

[INDENT]DN/RDN of the Organizational Unit containing the cell or DN of the root naming context to edit user in the default cell. (use '-' for stdin input)

`--user` <string>

[INDENT]User to change cell properties of. (DN/RDN, UPN, or SamAccountName; use '-' for stdin input).

`--uid` <string>

[INDENT]User ID

`--gid` <string>

[INDENT]Primary group ID

`--login-name` <string>

[INDENT]User's login name

`--home-dir` <string>

[INDENT]User's home directory

`--login-shell` <string>

[INDENT]User's login shell

`--description` <string>

[INDENT]Description of the user

**EXAMPLE**

[INDENT]Change login shell property of user TestUser in the cell TestOU.

[INDENT]`adtool -a edit-cell-user --dn OU=TestOU --user TestUser --login-shell=/bin/sh`


**LOOKUP-CELL**

`adtool` [<options>] -a lookup-cell --dn <string> [--home-dir-template <string>] [--default-login-shell <string>] [--users <string>] [--groups <string>] [--linked-cells <string>] [--all]

[INDENT]Retrieve AD Bridge Cell properties.

`--dn` <string>

[INDENT]DN/RDN of the Organizational Unit containing the cell or DN  of the root naming context to look up the default cell. (use '-' for stdin input)

`--home-dir-template` <string>

[INDENT]Show template of users' home directory.

`--default-login-shell` <string>

[INDENT]Show users' default login shell.

`--users` <string>

[INDENT]Show users defined in the cell.

`--groups` <string>

[INDENT]Show groups defined in the cell.

`--linked-cells` <string>

[INDENT]Show linked cells

`--all`

[INDENT]Show all cell properties

**EXAMPLE**

[INDENT]Find cells linked to AD Bridge Cell in OU=TestOU

[INDENT]`adtool -a lookup-cell --dn OU=TestOU --linked-cells`


**LOOKUP-CELL-GROUP**

`adtool` [<options>] -a lookup-cell-group --dn <string> --group <string> [--gid <string>] [--group-alias <string>] [--description <string>] [--all]

[INDENT]Retrieve properties of cell's group.

`--dn` <string>

[INDENT]DN/RDN of the Organizational Unit containing the cell or DN of the  root naming context to use the default cell. (use '-' for stdin  input)

`--group` <string>

[INDENT]Group to look up cell properties of. (DN/RDN, or CN; use '-' for   stdin input)

`--gid ` <string>

[INDENT]Show group ID

`--group-alias` <string>

[INDENT]Show group alias

`--description` <string>

[INDENT]Show description of the group

`--all`

[INDENT]Show all properties of the group.

**EXAMPLE**

[INDENT]Look up the properties of group TestGroup in the cell TestOU.

[INDENT]`adtool -a lookup-cell-group --dn OU=TestOU --group TestGroup`


**LOOKUP-CELL-USER**

`adtool` [<options>] -a lookup-cell-user --dn <string> --user <string> [--uid <string>] [--login-name <string>] [--home-dir <string>] [--login-shell <string>] [--description <string>] [--all]

[INDENT]Retrieve properties of cell's user.

`--dn` <string>

[INDENT]DN/RDN of the Organizational Unit containing the cell or DN of the root naming context to use the default cell. (use '-' for stdin input)

`--user` <string>

[INDENT]User to look up cell properties of. (DN/RDN, UPN, or SamAccountName; use '-' for stdin input)

`--uid` <string>

[INDENT]Show user ID.

`--gid` <string>

[INDENT]Show primary group ID.

`--login-name` <string>

[INDENT]Show login name.

`--home-dir` <string>

[INDENT]Show home directory.

`--login-shell` <string>

[INDENT]Show login shell.

`--description` <string>

[INDENT]Show description.

`--all`

[INDENT]Show all properties of the user.

**EXAMPLE**

[INDENT]Look up login shell property of user TestUser in the cell TestOU.

[INDENT]`adtool -a lookup-cell-user --dn OU=TestOU --user TestUser --login-shell`


**LINK-CELL**

`adtool` [<options>] -a link-cell --source-dn <string> --target-dn <string>

[INDENT]Link AD Bridge Cells.

`--source-dn` <string>

[INDENT]OU of the cell to which a new link is to be added. (DN/RDN; use '-' for stdin input).

`--target-dn` <string>

[INDENT]OU containing the cell to link to (DN/RDN; use '-' for stdin input).

**EXAMPLE**

[INDENT]Links the cells in the OUs TestOU and ComputersOU.

[INDENT]`adtool -a link-cell --source-dn OU=TestOU --target-dn OU=ComputersOU`


**UNLINK-CELL**

`adtool` [<options>] -a unlink-cell --source-dn <string> --target-dn <string>

[INDENT]Unlink AD Bridge Cells.

`--source-dn` <string>

[INDENT]OU of the cell from which the link is to be removed (DN/RDN;  use '-' for stdin input).

`--target-dn` <string>

[INDENT]OU containing the cell to unlink from (DN/RDN; use '-' for stdin input).

**EXAMPLE**

[INDENT]Unlinks the cells in the OUs TestOU and ComputersOU.

[INDENT]`adtool -a unlink-cell --source-dn OU=TestOU --target-dn OU=ComputersOU`
[/ENTERPRISE]


SEE ALSO
--------

The full documentation is available online at https://github.com/BeyondTrust/pbis-open/wiki/Documentation and https://www.beyondtrust.com/resources/education/documentation/?subcategory=ad-bridge

VERSION
-------

Version 9.0 +.
