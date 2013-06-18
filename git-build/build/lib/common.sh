#!/bin/bash

function check_arg
{
    if [ -z "$2" ]; then
        warn "Missing argument: $1"
        exit 1
    fi
}

function warn
{
    echo "$@" 1>&2
}

function exit_on_error
{
    if [ $1 -ne 0 ]; then
        if [ -n "$2" ]; then
            warn "$2"
        else
            warn "Exiting on error $1"
        fi
        exit $1
    fi
}

TIMER_START=0
TIMER_LABEL=""

function timer_start
{
    TIMER_LABEL="$1"
    TIMER_START=`perl -e 'print time'`
    exit_on_error $?
}

function timer_stop
{
    if [ "${TIMER_START}" != "0" ]; then
	local ELAPSED=`perl -e "print time - $TIMER_START"`
	echo ""
	if [ -n "${TIMER_LABEL}" ]; then
	    echo "Elapsed Time ($TIMER_LABEL): $ELAPSED seconds"
	else
	    echo "Elapsed Time: $ELAPSED seconds"
	fi
	echo ""
	TIMER_START=0
	TIMER_LABEL=""
    fi
}

function set_ctrlc_trap
{
    trap 'echo "CTRL-C pressed"; timer_stop ; exit_on_error 13' TERM INT
}

function time_command
{
    #
    # Execute the real build_loop in a subshell so we can time it even
    # if it exits.  Note that it is up the caller to make sure that
    # set_ctrlc_trap has been called to get timing info on CTRL-C.
    #
    # Parameters:
    #
    # 1 - timer label
    # 2 - command/function
    #
    # Returns:
    #
    #   Return or exit code of command/function that was executed.
    #   That means that a function that exists will not return a code.
    #

    local _timer_label="$1"
    local _timer_command="$2"
    shift 2

    check_arg _timer_label "${_timer_label}"
    check_arg _timer_label "${_timer_command}"

    timer_start "${_timer_label}"
    ( "${_timer_command}" "$@" )
    local rc=$?
    timer_stop
    return $rc
}

_capitalize()
{
    local first="`echo ${1:0:1} | tr '[:lower:]' '[:upper:]'`"

    echo "${first}${1:1}"
}

_unique_list()
{
    xargs | awk 'BEGIN { RS=" "; } { print $1; }' | sort | uniq | xargs
}

_contains()
{
    local needle=$1
    local hay
    shift

    for hay in $@
    do
	test "$needle" = "$hay" && return 0
    done

    return 1
}

component_available()
{
    local comp_file

    for comp_file in ${BUILD_ROOT}/build/components/*.comp
    do
	basename "${comp_file}" | sed 's/\.comp$//'
    done
}

