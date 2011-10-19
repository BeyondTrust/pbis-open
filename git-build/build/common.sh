#!/bin/bash

#
# This file should be sourced.
#

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

function check_arg
{
    if [ -z "$2" ]; then
        warn "Missing argument: $1"
        exit 1
    fi
}

function check_dir_arg
{
    check_arg "$1" "$2"
    if [ ! -d "$2" ]; then
        warn "Missing dir: $2"
        exit 1
    fi
}

function exists_function
{
    if [ "`type -t ${1}`" = "function" ]; then
	return 0
    else
	return 1
    fi
}

function get_extension
{
    local base=`basename $1`
    local ext=`echo "${base}" | perl -e '$_ = <>; s/^.*\.([^.]+)$/$1/; print $_;
'`
    if [ -z "$ext" ]; then
        warn "No extension found for \"$1\""
        exit 1
    fi
    echo "$ext"
}

function get_rpm_nosignature_option
{
    #
    # Determine whether platform support --nosignature option
    #
    rpm --nosignature > /dev/null 2>&1
    local rc=$?
    if [ $rc -ne 0 ]; then
        local RPM_NOSIGNATURE=""
    else
        local RPM_NOSIGNATURE="--nosignature"
    fi
    echo "${RPM_NOSIGNATURE}"
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

function find_one_file
{
    local spec="$1"

    local files
    files=`ls -t ${spec}`
    exit_on_error $?

    local count
    count=`echo $files | wc -w`
    exit_on_error $?

    if [ $count -ne 1 ]; then
	warn "Should have gotten only 1 file:"
	warn "  $files"
	exit 1
    fi

    echo "$files"
}

function check_is_rhel_21
{
    if [ -f /etc/redhat-release ]; then
        grep "Red Hat Linux .* Server release 2.1" /etc/redhat-release
        if [ $? -eq 0 ]; then
            return 0
        fi
    fi
    return 1
}

function get_var_value_from_text
{
    # $1 - text
    # $2 - var name
    echo "$1" | sed -e /^"${2}"=/!d -e s/^"${2}"=// | tail -1
}

function optional_append_variable
{
    # Given 3-4 params: <NAME> <true|false> <VALUE> [SEPARATOR]
    # appends VALUE to variable NAME depending on true|false.
    if [ -z "$1" ]; then
	warn "Missising parameter 1 (variable name)."
	return 1
    fi
    case "$2" in
	true|false)
	    ;;
	*)
	    warn "Invalid parameter 2 (true or false): '$2'."
	    return 1
	    ;;
    esac
    if "$2" ; then
        if [ -n "${!1}" ]; then
	    if [ -n "$4" ]; then
		eval "$1=\"${!1}$4$3\""
	    else
		eval "$1=\"${!1} $3\""
	    fi
        else
            eval "$1=$3"
        fi
    fi
}
