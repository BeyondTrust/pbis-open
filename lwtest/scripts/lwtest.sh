#!/bin/sh
# Run the tests of the Likewise test suite.

# Constants returned by test programs.
lw_error_success=0;
lw_error_test_failed=3;
lw_error_test_skipped=4;
lw_error_failure=1;

# Source helper functions.
. /opt/likewise/bin/lwtest-functions.sh

lwt_init $@
trap lwt_cleanup INT

# Prepare arguments for test programs.
ARGUMENTS=
if [ -n "$LWT_FILE_USERS" ] 
then
    ARGUMENTS="${ARGUMENTS} --users ${LWT_FILE_USERS}"
fi


if [ -n "$LWT_FILE_GROUPS" ]
then
    ARGUMENTS="${ARGUMENTS} --groups ${LWT_FILE_GROUPS}"
fi

if [ -n "$LWT_LOG_FILE" ]
then 
    ARGUMENTS="${ARGUMENTS} --log-file ${LWT_LOG_FILE}"
fi

if [ -n "$LWT_LOG_LEVEL" ]
then 
    ARGUMENTS="${ARGUMENTS} --log-level ${LWT_LOG_LEVEL}"
fi

ARGUMENTS="${ARGUMENTS} --append"


lwt_test_programs="lwt-lsa-authenticate-user lwt-lsa-check-user-info lwt-lsa-enum-users lwt-lsa-find-group-by-name lwt-lsa-find-group-by-id lwt-lsa-find-user-by-id lwt-lsa-find-user-by-name lwt-lsa-open-session lwt-lsa-validate-user lwt-lsa-verify-sid-info lwt-lsa-enum-groups lwt-lsa-validate-groupinfo-by-name lwt-lsa-validate-groupinfo-by-id lwt-lsa-validate-groupinfo-by-api lwt-lsa-get-metrics lwt-lsa-get-status "


lwt_check_lsass "lsassd"

RESULT=`sudo -u $LWT_USER $LIKEWISE_BIN/lw-get-status`
echo

lwt_join ${LWT_JOIN_ARGUMENTS}
sleep 5 
echo

lwtCountSuccess=0
lwtCountSkip=0
lwtCountFail=0
for lwt_program in $lwt_test_programs
do
    
    lwt_command_user "${LIKEWISE_BIN}/$lwt_program" "${ARGUMENTS}"
    lwt_status="$?"
    if [ "$lwt_status" -eq $lw_error_success ]
    then
        lwt_success_msg "Pass" 
        lwtCountSuccess=$(($lwtCountSuccess + 1))
    elif [ "$lwt_status" -eq $lw_error_test_skipped ]
    then
        lwt_success_msg "Skip"
        lwtCountSkip=$(($lwtCountSkip + 1))
    elif [ "$lwt_status" -eq $lw_error_test_failed ]
    then
        lwt_fail_msg "Fail" 
        lwtCountFail=$(($lwtCountFail + 1))
    else
        lwt_broke_msg "Broke"
        lwtCountFail=$(($lwtCountFail + 1))
    fi
     
    lwt_check_lsass    
    echo
    echo
    echo
done


echo "Online LSA API Test Results"
echo "  APIs Passed: $lwtCountSuccess"
echo "  APIs Skipped:$lwtCountSkip"
echo "  APIs Failed: $lwtCountFail"
echo 
echo 

lwt_msg 'Finished all tests'

#exit $lwtr_unknown_error;
