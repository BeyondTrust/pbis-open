#!/bin/sh
#
#

# Taken from /lib/lsb/init_functions
log_use_fancy_output() {
    TPUT=/usr/bin/tput
    EXPR=/usr/bin/expr
    if [ -t 1 ] && [ "x$TERM" != "" ] && [ "x$TERM" != "xdumb" ] && [ -x $TPUT ] && [ -x $EXPR ] && $TPUT hpa 60 >/dev/null 2>&1 && $TPUT setaf 1 >/dev/null 2>&1; then
        [ -z $FANCYTTY ] && FANCYTTY=1 || true
    else
        FANCYTTY=0
    fi
    case "$FANCYTTY" in
        1|Y|yes|true)   true;;
        *)              false;;
    esac
}


#
lwt_init() {
    while [ $# -gt 0 ]
    do
        case "$1" in
        --users)       shift
                    LWT_FILE_USERS=$1
                    ;;
        --groups)   shift
                    LWT_FILE_GROUPS=$1
                    ;;
        --invalid)  shift
                    LWT_FILE_INVALID=$1
                    ;;
        --logfile)  shift
                    LWT_LOG_FILE=$1
                    ;;
        --loglevel) shift
                    LWT_LOG_LEVEL=$1
                    ;;
        *)          echo
                    echo "Usage: lwtest-functions.sh [--users users.csv] [--groups groups.csv] [--invalid invalid.csv] [--loglevel <0-2>] [--logfile apitest.log]"
                    echo 
                    echo "      If no options are given takes default arguements as follows"
                    echo "      --users=/opt/likewise/bin/domain.com.ou.users.csv"
                    echo "      --groups=/opt/likewise/bin/domain.com.ou.groups.csv"
                    echo "      --loglevel=1"
                    echo "      --logfile=/tmp/apitest.log"
                    echo 
                    exit 1;;
        esac
        shift
    done

    COLOR_BROKE=''
    COLOR_FAIL=''
    COLOR_SUCCESS=''
    COLOR_NORMAL=''

    if [ -n "$LWT_LOG_FILE" ]; then
        rm -f "$LWT_LOG_FILE"
#    touch "$LWT_LOG_FILE"
    fi

    if log_use_fancy_output; then
      # 1 : Red   2: Green  3: Yellow
      COLOR_BROKE=`$TPUT setaf 3`
      COLOR_FAIL=`$TPUT setaf 1`
      COLOR_SUCCESS=`$TPUT setaf 2`
      COLOR_NORMAL=`$TPUT op`
    fi
    /bin/echo -e -n "${COLOR_NORMAL}"
}

# Call when exiting or ctrl-c
lwt_cleanup() {
    /bin/echo -e "${COLOR_NORMAL}"
}

# Write a generic message
lwt_msg() {
    local errrr=$?
    /bin/echo -E "$@"
    return $errrr;
}

# Write a success message. 
lwt_success_msg() {
    local errrr=$?    # Preserve error 
    /bin/echo -e -n "${COLOR_SUCCESS}"
    /bin/echo -E -n "$@"
    /bin/echo -e "${COLOR_NORMAL}"
    return $errrr
}

# Write a failure message.
lwt_fail_msg() {
    local errrr=$?   # Preserve error
    /bin/echo -e -n "${COLOR_FAIL}"
    /bin/echo -E -n "$@"
    /bin/echo -e "${COLOR_NORMAL}"
    return $errrr
}

# Write a failure message.
lwt_broke_msg() {
    local errrr=$?   # Preserve error
    /bin/echo -e -n "${COLOR_BROKE}"
    /bin/echo -E -n "$@"
    /bin/echo -e "${COLOR_NORMAL}"
  return $errrr
}

# Execute command as unpriviledged user.
lwt_command_user() {
    local errrr;
    /bin/echo -e "Executing : $lwt_program"
    RESULT=`sudo -u $LWT_USER $@`
    errrr=$?
    return $errrr
}

lwt_join() {
    local status;
    lwt_msg 'Attempting join (after doing a leave)'
    ${LIKEWISE_BIN}/domainjoin-cli leave && ${LIKEWISE_BIN}/domainjoin-cli join $@
    status=$?
    if [ "$status" -eq 0 ]
    then
        lwt_success_msg "Joined domain successfully."
    else
        lwt_fail_msg "Domain join failed, exiting"
        exit 1
    fi
}


#check whether lsass crashed because of test
lwt_check_lsass() {
    REST=`sudo -u $LWT_USER $LIKEWISE_BIN/lwsm status lsass`
    rrrr=$?
    if [ "$rrrr" -eq 1 ]
    then
        lwt_fail_msg  "lwtest crashed lsassd."

        REST=`sudo -u $LWT_USER $LIKEWISE_BIN/lwsm start lsass`
        rrrr=$?
        if [ "$rrrr" -eq 0 ]
        then
            lwt_success_msg "Restarted lsassd."
        
        else
            lwt_fail_msg "Failed to restart lsassd."
        fi
        return
    fi
}

lw_error_success=0;
lw_error_test_failed=3;
lw_error_test_skipped=4;
lw_error_failure=1;

# Source helper functions.
#. /opt/likewise/bin/lwtest-functions.sh
#LWT_LOG_FILE=/tmp/apitest.log
LWT_LOG_LEVEL=1
LWT_USER=root
LIKEWISE_BIN="/opt/likewise/bin"

QUERY=`/opt/likewise/bin/domainjoin-cli query | tr "[A-Z]" "[a-z]"`
DOMAIN=`echo $QUERY | sed -n "s/.*domain \= \([A-Za-z0-9.-]*\).*/\1/p"`
OU=`echo $QUERY | sed -n "s/.*ou=\([^ ,]*\).*/\1/p"`

if [ -z $DOMAIN ]; then
    echo "Please join to a domain before running tests"
    echo
    exit 1
fi

if [ -z $OU ]; then
    OU="computers"
fi

LWT_FILE_USERS=$LIKEWISE_BIN/$DOMAIN.${OU}.users.csv
LWT_FILE_GROUPS=$LIKEWISE_BIN/$DOMAIN.${OU}.groups.csv
LWT_FILE_INVALID=$LIKEWISE_BIN/$DOMAIN.${OU}.invalid.csv
LWT_LOG_FILE=/tmp/$DOMAIN.$OU.api

lwt_init $@
trap lwt_cleanup INT

if [ ! -f $LWT_FILE_USERS  -o ! -f $LWT_FILE_GROUPS  ]; then
    /bin/echo -e -n "${COLOR_FAIL}"
    /bin/echo -E -n "File $LWT_FILE_USERS and/or $LWT_FILE_GROUPS doesnt exist" >&1 | tee $LWT_LOG_FILE 
    /bin/echo -e "${COLOR_NORMAL}"
    
    exit 1
fi

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

if [ -n "$LWT_FILE_INVALID" ]
then
    ARGUMENTS="${ARGUMENTS} --invalid ${LWT_FILE_INVALID}"
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
