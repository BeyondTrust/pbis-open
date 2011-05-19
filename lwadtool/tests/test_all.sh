#!/bin/sh

exec 2> test_all.sh.log

#
# All the tests below manipulate data within the ROOT_OU. Hence, every invocation of this script must use a
# different ROOT_OU to avoid interference.
#
ROOT_OU="AdtRootOu"
ROOT_OU_NON_SCHEMA=${ROOT_OU}NS

#########################
# CONNECTION PROPERTIES #
#########################

#
# We need either AD server address/port or fully qualified domain name.
#
ADT_CONN_SERVER=
ADT_CONN_PORT=
ADT_CONN_DOMAIN="-s CORPQA-DC3.corpqa.centeris.com"

#
# OPERATIONAL MODE
#
# We are in schema mode by default.
#
ADT_SCHEMA=

#############################
# AUTHENTICATION PROPERTIES #
#############################

#
# If none of the authentication properties is set the machine must be 
# domain-joined so we can do SSO using the cridentials of the user 
# running the script.
#
# We always authenticate via Krb5 ticket unless ADT_AUTH_SEC_MODE is 
# off. In secure mode we use name/password to get TGT.
#
ADT_AUTH_NAME="-n adtool"
ADT_AUTH_PASSWD="-x adtool"

#
# This is only applicable to name/password authentication. We use
# secure mode by default. Set ADT_AUTH_SEC_MODE to off to disable
# secure authentication.
#
ADT_AUTH_SEC_MODE="--no-sec"

#
# Define location of krb5 keytab file.
# If set, ADT_AUTH_NAME must also be defined. ADT_AUTH_PASSWD will
# be ignored in this case.
#
ADT_AUTH_KEYTAB=

#
# Define location of krb5 ticket cache file. This authentication
# method takes priority over name/password or keytab method.
#
ADT_AUTH_KRB5CC=

##################
# COMMON OPTIONS #
##################

#
# Print DNs of the objects to be looked up, modified or searched for.
#
ADT_OPT_QUIET= 

#
# Suppress printing to stdout. Just set the return code. print-dn 
# option makes an exception.
#
ADT_OPT_PRINT_DN=

#
# Log level.
# Acceptable values: 1 (error), 2(warning), 3(info), 4(verbose),
# 5 (trace) (Default: warning).
#
ADT_OPT_LOG_LEVEL="-l 5"

#############
# TEST DATA #
#############
NAME_CTX="DC=corpqa,DC=centeris,DC=com"
CUR_CTX=$NAME_CTX

CELL_OU1="AdtCellOu1"
CELL_OU2="AdtCellOu2"

MOVE_OU="AdtMoveOu"

USERS_OU="Users"
USER1="${ROOT_OU}User1"
USER2="${ROOT_OU}User2"
USER3="${ROOT_OU}User3"

GROUPS_OU="Groups"
GROUP1="${ROOT_OU}Group1"
GROUP2="${ROOT_OU}Group2"
GROUP3="${ROOT_OU}Group3"

COMPUTERS_OU="Computers"
COMPUTER="${ROOT_OU}Computer"

################
# FULL COMMAND #
################
ADT_CMD="./lw-adtool $ADT_CONN_SERVER $ADT_CONN_PORT $ADT_CONN_DOMAIN $ADT_SCHEMA $ADT_AUTH_NAME $ADT_AUTH_PASSWD $ADT_AUTH_SEC_MODE $ADT_AUTH_KEYTAB $ADT_AUTH_KRB5CC $ADT_OPT_QUIET $ADT_OPT_LOG_LEVEL -a"

###################
# Handy functions #
###################

EMSG="************************* RUNNING COMMAND ****************************"

ExitOnError()
{
   say="\nTEST FAILED$1\nSee test_all.sh.log file for details.\n"
   echo $say
   echo $say >&2

   exit 1
}

#
# Remove root OU used by the test 
#
Cleanup()
{
CMD="$ADT_CMD delete-object --dn OU=$ROOT_OU --force"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD #2> /dev/null

CMD="$ADT_CMD delete-object --dn OU=$ROOT_OU_NON_SCHEMA --force"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD #2> /dev/null

}

#
# Create OUs
#
TestCreateOUs()
{
#
# Create root OU. 
#
CMD="$ADT_CMD new-ou --dn OU=$ROOT_OU"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Create OUs for two cells. 
#
CMD="$ADT_CMD new-ou --dn OU=$ROOT_OU --name $CELL_OU1"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CMD="$ADT_CMD new-ou --dn OU=$ROOT_OU --name $CELL_OU2"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Create OUs for two users, groups and computers 
#
CMD="$ADT_CMD new-ou --dn OU=$ROOT_OU,$CUR_CTX --name $USERS_OU"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CMD="$ADT_CMD new-ou --dn OU=$ROOT_OU --name $GROUPS_OU"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CMD="$ADT_CMD new-ou --dn OU=$ROOT_OU --name $COMPUTERS_OU"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}
}

#
# Create OUs
#
TestCreateCells()
{
CUR_CTX="OU=$ROOT_OU,$CUR_CTX"

CMD="$ADT_CMD new-cell --dn OU=$CELL_OU1,$CUR_CTX --default-login-shell=/bin/ksh"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CMD="$ADT_CMD new-cell --dn OU=$CELL_OU2,$CUR_CTX --home-dir-template=%H/%D/%U"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Create user accounts in AD
#
TestCreateUsers()
{
CUR_CTX="OU=$USERS_OU,OU=$ROOT_OU,$CUR_CTX"

CMD="$ADT_CMD new-user --dn $CUR_CTX --cn=${USER1}CN --logon-name=$USER1 --password=$USER1"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CMD="$ADT_CMD new-user --dn $CUR_CTX --cn=${USER2}CN --logon-name=$USER2 --password=$USER2 --no-password-expires"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CMD="$ADT_CMD new-user --dn $CUR_CTX --cn=${USER3}CN --logon-name=$USER3 --password=$USER3 --no-password-expires"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Create security groups in AD
#
TestCreateGroups()
{
CUR_CTX="OU=$GROUPS_OU,OU=$ROOT_OU,$CUR_CTX"

CMD="$ADT_CMD new-group --dn $CUR_CTX --pre-win-2000-name=${GROUP1}Pre --name=$GROUP1"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CMD="$ADT_CMD new-group --dn $CUR_CTX --pre-win-2000-name=${GROUP2}Pre --name=$GROUP2"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CMD="$ADT_CMD new-group --dn $CUR_CTX --pre-win-2000-name=${GROUP3}Pre --name=$GROUP3"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Create computer object in AD
#
TestCreateComputers()
{
CUR_CTX="OU=$COMPUTERS_OU,OU=$ROOT_OU,$CUR_CTX"

CMD="$ADT_CMD new-computer --dn $CUR_CTX --pre-win-2000-name=${COMPUTER} --name=$COMPUTER"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Search for all users (~20000 entries) to make sure paging control works.
#
TestSearchAndPageResults()
{

CMD="$ADT_CMD search-object --search-base $NAME_CTX --filter \(objectclass=user\) -t | grep -s adtool"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || {
ExitOnError
}

}

#
# Search/Lookup objects objects in AD
#
TestSearchAndLookupObjects()
{
CUR_CTX="OU=$ROOT_OU,$CUR_CTX"

#
# Search for and look up OU
#
CMD="$ADT_CMD search-ou --search-base $CUR_CTX --name=$CELL_OU1 -t | $ADT_CMD lookup-object --dn=- --attr=description | grep -s lw-adtool"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Search for and look up user
#
CMD="$ADT_CMD search-user --search-base $CUR_CTX --name=$USER1 -t | $ADT_CMD lookup-object --dn=- --attr=description | grep -s lw-adtool"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Search for and look up group
#
CMD="$ADT_CMD search-group --search-base $CUR_CTX --name=$GROUP1 -t | $ADT_CMD lookup-object --dn=- --attr=description | grep -s lw-adtool"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Search for and look up computer
#
CMD="$ADT_CMD search-computer --search-base $CUR_CTX --name=$COMPUTER -t | $ADT_CMD lookup-object --dn=- --attr=description | grep -s lw-adtool"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Search for and look up a generic object
#
CMD="$ADT_CMD search-object --search-base $CUR_CTX --filter \(samAccountName=$USER1\) -t | $ADT_CMD lookup-object --dn=- --attr=description | grep -s lw-adtool"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Try to enable and disable a user in AD
#
TestEnableDisableUser()
{
CUR_CTX="OU=$ROOT_OU,$CUR_CTX"

#
# Enable user
#
CMD="$ADT_CMD search-user --search-base $CUR_CTX --name=$USER1 -t | $ADT_CMD enable-user --name=-"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Make sure the user is enabled
#
CMD="$ADT_CMD search-user --search-base $CUR_CTX --name=$USER1 -t | $ADT_CMD lookup-object --dn=- --attr=userAccountControl"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && acctControl=`eval $CMD`
[ $? = 0 ] || { 
ExitOnError
}

[ $acctControl = 512 ] || {
ExitOnError 
}

#
# Disable user
#
CMD="$ADT_CMD search-user --search-base $CUR_CTX --name=$USER1 -t | $ADT_CMD disable-user --name=-"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Make sure the user is disabled
#
CMD="$ADT_CMD search-user --search-base $CUR_CTX --name=$USER1 -t | $ADT_CMD lookup-object --dn=- --attr=userAccountControl"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && acctControl=`eval $CMD`
[ $? = 0 ] || { 
ExitOnError
}

[ $acctControl = 514 ] || {
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Try to reset user password in AD
#
TestPasswordReset()
{
CUR_CTX="OU=$ROOT_OU,$CUR_CTX"

#
# Reset password
#
CMD="$ADT_CMD search-user --search-base $CUR_CTX --name=$USER1 -t | $ADT_CMD reset-user-password --name=- --password=${USER1}123 --no-password-expires"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Make sure the password has been reset
#
CMD="$ADT_CMD search-user --search-base $CUR_CTX --name=$USER1 -t | $ADT_CMD lookup-object --dn=- --attr=userAccountControl"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && acctControl=`eval $CMD`
[ $? = 0 ] || { 
ExitOnError
}

[ $acctControl = 66050 ] || {
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Test adding/removing users and groups to/from an AD group
#
TestGroupMembership()
{
CUR_CTX="OU=$ROOT_OU,$CUR_CTX"

#
# Add user1 to group3
#
CMD="$ADT_CMD add-to-group --user $USER1 --to-group=${GROUP3}Pre"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Make sure the user1 has been added to group3
#
CMD="$ADT_CMD search-user --search-base $CUR_CTX --name=$USER1 -t | $ADT_CMD lookup-object --dn=- --attr=memberOf | grep -s $GROUP3"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError
}

#
# Remove user1 from group3
#
CMD="$ADT_CMD remove-from-group --user $USER1 --from-group=${GROUP3}Pre"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Add group1 to group3
#
CMD="$ADT_CMD add-to-group --group ${GROUP1}Pre --to-group=${GROUP3}Pre"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Make sure the group1 has been added to group3
#
CMD="$ADT_CMD search-group --search-base $CUR_CTX --name=$GROUP1 -t | $ADT_CMD lookup-object --dn=- --attr=memberOf | grep -s $GROUP3"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError
}

#
# Remove group1 from group3
#
CMD="$ADT_CMD remove-from-group --group ${GROUP1}Pre --from-group=${GROUP3}Pre"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Try rename an object
#
TestMoveObject()
{
CUR_CTX="OU=$ROOT_OU,$CUR_CTX"

#
# Create an OU. 
#
CMD="$ADT_CMD new-ou --dn OU=$ROOT_OU --name $MOVE_OU"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Rename the OU
#
CMD="$ADT_CMD move-object --from OU=${MOVE_OU},${CUR_CTX} --to OU=${MOVE_OU}Moved,${CUR_CTX}"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Search for LW cells
#
TestSearchCells()
{
CUR_CTX="OU=$ROOT_OU,$CUR_CTX"

#
# Search for and look up OU
#
CMD="$ADT_CMD search-cells --search-base $CUR_CTX -t | wc -l"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && CellsNum=`eval $CMD`
[ $? = 0 ] || { 
ExitOnError 
}

[ $CellsNum = 2 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Try to add/remove users and groups to/from cells
#
TestCellMembership()
{
CUR_CTX="OU=$CELL_OU1,OU=$ROOT_OU,$CUR_CTX"

#
# Add group3 to cell1.
#
CMD="$ADT_CMD add-to-cell --dn $CUR_CTX --group=${GROUP3}Pre"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Remove group3 from cell1.
#
CMD="$ADT_CMD remove-from-cell --dn $CUR_CTX --group=${GROUP3}Pre"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Add group3 to cell1 again.
#
CMD="$ADT_CMD add-to-cell --dn $CUR_CTX --group=${GROUP3}Pre"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Add user1 to cell 1.
#
CMD="$ADT_CMD add-to-cell --dn $CUR_CTX --user=$USER1"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Remove user1 from cell 1.
#
CMD="$ADT_CMD remove-from-cell --dn $CUR_CTX --user=$USER1"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Add user1 to cell 1 again.
#
CMD="$ADT_CMD add-to-cell --dn $CUR_CTX --user=$USER1"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Try link/unlink cells
#
TestLinkCells()
{
CUR_CTX="OU=$ROOT_OU,$CUR_CTX"

SOURCE_DN="OU=$CELL_OU1,$CUR_CTX"
TARGET_DN="OU=$CELL_OU2,$CUR_CTX"

#
# Link cell1 to cell2.
#
CMD="$ADT_CMD link-cell --source-dn $SOURCE_DN --target-dn $TARGET_DN"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Unlink cell2 from cell1.
#
CMD="$ADT_CMD unlink-cell --source-dn $TARGET_DN --target-dn $SOURCE_DN"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Link cell2 to cell1 again.
#
CMD="$ADT_CMD link-cell --source-dn $SOURCE_DN --target-dn $TARGET_DN"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Test edit and lookup cell operations
#
TestChangeLookupCellProperties()
{
CUR_CTX="OU=$ROOT_OU,$CUR_CTX"

CELL_OU="OU=$CELL_OU1,$CUR_CTX"

#
# Change default login shell in cell1
#
CMD="$ADT_CMD edit-cell --dn $CELL_OU --default-login-shell=/bin/csh"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Make sure the login shell has changed in cell1
#
CMD="$ADT_CMD lookup-cell --dn $CELL_OU --default-login-shell | grep -s /bin/csh"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Make sure cell1 contains group3
#
CMD="$ADT_CMD lookup-cell --dn $CELL_OU --groups | grep -s ${GROUP3}"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Make sure cell1 contains user1
#
CMD="$ADT_CMD lookup-cell --dn $CELL_OU --users | grep -s $USER1"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Make sure cell1 is linked
#
CMD="$ADT_CMD lookup-cell --dn $CELL_OU --linked-cells | grep -s -"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Test edit and lookup operations for a cell user
#
TestChangeLookupCellUserProperties()
{
CUR_CTX="OU=$ROOT_OU,$CUR_CTX"

CELL_OU="OU=$CELL_OU1,$CUR_CTX"

#
# Change default login shell for user1
#
CMD="$ADT_CMD edit-cell-user --dn $CELL_OU --user $USER1 --login-shell=/bin/somesh"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Make sure the login shell has changed for user1
#
CMD="$ADT_CMD lookup-cell-user --dn $CELL_OU --user $USER1 --login-shell | grep -s /bin/somesh"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Test edit and lookup operations for a cell group
#
TestChangeLookupCellGroupProperties()
{
CUR_CTX="OU=$ROOT_OU,$CUR_CTX"

CELL_OU="OU=$CELL_OU1,$CUR_CTX"

#
# Change GID for group3
#
CMD="$ADT_CMD edit-cell-group --dn $CELL_OU --group ${GROUP3}Pre --gid=909090"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

#
# Make sure the GID has change for group3
#
CMD="$ADT_CMD lookup-cell-group --dn $CELL_OU --group ${GROUP3}Pre --gid | grep -s 909090"

echo "$EMSG\n$CMD" && echo "$EMSG\n$CMD" >&2 && eval $CMD
[ $? = 0 ] || { 
ExitOnError 
}

CUR_CTX=$NAME_CTX
}

#
# Main procedure
#
Main()
{
Cleanup

TestCreateOUs
TestCreateCells
TestCreateUsers
TestCreateGroups
TestCreateComputers
TestSearchAndLookupObjects
TestEnableDisableUser
TestPasswordReset
TestGroupMembership
TestMoveObject
TestCellMembership
TestSearchCells
TestLinkCells
TestChangeLookupCellProperties
TestChangeLookupCellUserProperties
TestChangeLookupCellGroupProperties
TestSearchAndPageResults
}

################
# RUN THE TEST #
################
Main

#
# Now run the test in non-schema mode
#
Cleanup
ROOT_OU=$ROOT_OU_NON_SCHEMA

Main

