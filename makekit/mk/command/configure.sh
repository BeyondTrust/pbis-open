#
# Copyright (c) Brian Koropoff
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the MakeKit project nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#

##
#
# configure.sh -- configures a project, preparing it to be built
#
##

BASIC_VARIABLES="\
    MK_HOME MK_SHELL MK_ROOT_DIR MK_SOURCE_DIR MK_OBJECT_DIR MK_STAGE_DIR \
    MK_RUN_DIR MK_OPTIONS MK_SEARCH_DIRS MK_MODULE_LIST MK_MODULE_FILES"

. "${MK_HOME}/mk.sh" || exit 1

_mk_emit()
{
    echo "$@" >&6
}

_mk_emitf()
{
    printf "$@" >&6
}

_mk_find_module_imports_recursive()
{
    unset MODULES SUBDIRS
   
    if ! [ -e "${MK_SOURCE_DIR}${1}/MakeKitBuild" ]
    then
        return 0
    fi

    mk_safe_source "${MK_SOURCE_DIR}${1}/MakeKitBuild" || mk_fail "Could not read MakeKitBuild in ${MK_SOURCE_DIR}${1}"

    result="$result $MODULES"
    
    for _dir in ${SUBDIRS}
    do
        if [ "$_dir" != "." ]
        then
            _mk_find_module_imports_recursive "$1/${_dir}"
        fi
    done
}

_mk_set_defaults()
{
    unset -f defaults
    mk_source_or_fail "${MK_SOURCE_DIR}/MakeKitBuild"
    mk_function_exists defaults && defaults
    
    [ -z "$PROJECT_NAME" ] && PROJECT_NAME="$(cd "${MK_SOURCE_DIR}" && basename "$(pwd)")"
}

_mk_find_module_imports()
{
    result=""
    _mk_find_module_imports_recursive ""
}

_mk_modules_rec()
{
    for __module in "$@"
    do
        _mk_contains "$__module" ${MK_MODULE_LIST} && continue
        _mk_find_resource "module/${__module}.sh" || mk_fail "could not find module: $__module"
        set -- "$__module" "$result"
        unset DEPENDS
        . "$result"
        _mk_modules_rec ${DEPENDS}
        MK_MODULE_LIST="$MK_MODULE_LIST $1"
        MK_MODULE_FILES="$MK_MODULE_FILES $2"
    done
}

_mk_module_list()
{
    MK_MODULE_LIST=""
    MK_MODULE_FILES=""

    _mk_modules_rec "$@"
}

_mk_process_build_module()
{
    _mk_find_resource "module/$1.sh"

    MK_CURRENT_FILE="$result"
    MK_BUILD_FILES="$MK_BUILD_FILES $MK_CURRENT_FILE"

    unset -f option configure make
    unset SUBDIRS

    mk_source_or_fail "$MK_CURRENT_FILE"
   
    mk_function_exists option && option
    if mk_function_exists configure
    then
        _mk_configure_prehooks
        configure
        _mk_configure_posthooks
    fi
}

_mk_process_build_configure()
{
    unset -f option configure make
    unset SUBDIRS

    MK_CURRENT_FILE="${MK_SOURCE_DIR}$1/MakeKitBuild"
    MK_BUILD_FILES="$MK_BUILD_FILES $MK_CURRENT_FILE"

    mk_safe_source "$MK_CURRENT_FILE" || mk_fail "Could not read MakeKitBuild in ${MK_SOURCE_DIR}${1}"
    
    mk_function_exists option && option

    MK_SUBDIR="$1"
    mk_msg_verbose "configuring"
    _mk_configure_prehooks
    mk_function_exists configure && configure
    _mk_configure_posthooks
}

_mk_process_build_make()
{
    unset -f option configure make
    _backup_SUBDIRS="$SUBDIRS"

    MK_CURRENT_FILE="${MK_SOURCE_DIR}$1/MakeKitBuild"
    mk_safe_source "$MK_CURRENT_FILE" || mk_fail "Could not read MakeKitBuild in ${MK_SOURCEDIR}${1}"
    SUBDIRS="$_backup_SUBDIRS"
    
    MK_SUBDIR="$1"
    mk_msg_verbose "emitting make rules"
    _mk_make_prehooks
    mk_function_exists make && make
    _mk_make_posthooks
}

_mk_process_build_recursive()
{
    mk_push_vars MK_MSG_DOMAIN MK_CURRENT_FILE SUBDIRS _preorder_make

    MK_MSG_DOMAIN="${1#/}"

    if [ -z "$MK_MSG_DOMAIN" ]
    then
        MK_MSG_DOMAIN="$PROJECT_NAME"
    fi

    mk_mkdir "${MK_OBJECT_DIR}$1"

    # Process configure stage
    _mk_process_build_configure "$1"

    # Write exports files
    _mk_write_exports "${MK_OBJECT_DIR}$1/.MakeKitExports"

    for _dir in ${SUBDIRS}
    do
        if [ "$_dir" = "." ]
        then
            # Process make stage before children
            _preorder_make=yes
            _mk_process_build_make "$1"
        else
            _mk_process_build_recursive "$1/${_dir}"
            _mk_restore_exports "${MK_OBJECT_DIR}${1}/.MakeKitExports"
        fi
    done

    # Process make stage if we didn't do it before child directories
    if [ -z "$_preorder_make" ]
    then
        _mk_process_build_make "$1"
    fi

    mk_pop_vars
}

_mk_process_build()
{
    MK_SUBDIR=":"

    for _module in ${MK_MODULE_LIST}
    do
        MK_MSG_DOMAIN="$_module"
        _mk_process_build_module "${_module}"
    done

    # Write exports file for build root
    _mk_write_exports ".MakeKitExports"

    # Run build functions for project
    _mk_process_build_recursive ''

    MK_SUBDIR=":"
}

mk_option()
{
    unset _found
    mk_push_vars TYPE OPTION DEFAULT VAR PARAM HELP REQUIRED
    mk_parse_params
    
    [ -z "$VAR" ] && VAR="$1"
    [ -z "$OPTION" ] && OPTION="$2"
    [ -z "$DEFAULT" ] && DEFAULT="$3"

    if [ "$VAR" = "MK_HELP" -a "$MK_HELP" != "yes" ]
    then
        _skip_help="yes"
    else
        _skip_help=""
    fi

    mk_unquote_list "$MK_OPTIONS"
    for _arg in "$@"
    do
        case "$_arg" in
            "--$OPTION="*|"--with-$OPTION="*)
                mk_set "$VAR" "${_arg#*=}"
                break
                ;;
            "--$OPTION"|"--enable-$OPTION")
                mk_set "$VAR" "yes"
                break
                ;;
            "--no-$OPTION"|"--disable-$OPTION")
                mk_set "$VAR" "no"
                break
                ;;
        esac
    done

    if ! mk_is_set "$VAR"
    then
        if [ -n "$REQUIRED" ]
        then
            mk_fail "Option not specified: $OPTION"
        else
            mk_set "$VAR" "$DEFAULT"
        fi
    fi

    if [ "$MK_HELP" = "yes" -a -z "$_skip_help" ]
    then
        _mk_print_option
    fi

    mk_pop_vars
}

_mk_print_option()
{
    [ -z "$PARAM" ] && PARAM="value"
    [ -z "$HELP" ] && HELP="No help available"

    if [ -n "$OPTION" -a "$MK_SHOW_VARS" = "no" ]
    then
        _form="--${OPTION}=${PARAM}"
    else
        _form="${VAR}=${PARAM}"
    fi
    _doc="$HELP"

    printf "%s\n" "$_form"
    printf "%s\n" "$_doc"
    
    if mk_is_set "$VAR"
    then
        mk_get "$VAR"
        printf "%s\n" "[$result]"
    elif [ -n "$DEFAULT" ]
    then
        printf "%s\n" "[$DEFAULT]"
    fi
    
    printf "###\n"
}

_mk_write_exports()
{
    {
        for _export in ${MK_EXPORTS}
        do
            mk_get "$_export"
            mk_quote "$result"
            echo "$_export=$result"
        done

        echo "MK_EXPORTS='$MK_EXPORTS'"
    } >"$1"
}

_mk_restore_exports()
{
    unset ${MK_EXPORTS}

    . "$1"
}

mk_export()
{
    for _export in "$@"
    do
        case "$_export" in
            *"="*)
                _val="${_export#*=}"
                _name="${_export%%=*}"
                mk_set "$_name" "$_val"
                _mk_contains "$_name" ${MK_EXPORTS} || MK_EXPORTS="$MK_EXPORTS $_name"
                ;;
            *)
                _mk_contains "$_export" ${MK_EXPORTS} || MK_EXPORTS="$MK_EXPORTS $_export"
                ;;
        esac
    done
}

mk_add_configure_prehook()
{
    if ! _mk_contains "$1" "$_MK_CONFIGURE_PREHOOKS"
    then
        _MK_CONFIGURE_PREHOOKS="$_MK_CONFIGURE_PREHOOKS $1"
    fi
}

mk_add_configure_posthook()
{
    if ! _mk_contains "$1" "$_MK_CONFIGURE_POSTHOOKS"
    then
        _MK_CONFIGURE_POSTHOOKS="$_MK_CONFIGURE_POSTHOOKS $1"
    fi
}

mk_add_make_prehook()
{
    if ! _mk_contains "$1" "$_MK_MAKE_PREHOOKS"
    then
        _MK_MAKE_PREHOOKS="$_MK_MAKE_PREHOOKS $1"
    fi
}

mk_add_make_posthook()
{
    if ! _mk_contains "$1" "$_MK_MAKE_POSTHOOKS"
    then
        _MK_MAKE_POSTHOOKS="$_MK_MAKE_POSTHOOKS $1"
    fi
}

mk_add_complete_hook()
{
    if ! _mk_contains "$1" "$_MK_COMPLETE_HOOKS"
    then
        _MK_COMPLETE_HOOKS="$_MK_COMPLETE_HOOKS $1"
    fi
}

_mk_configure_prehooks()
{
    for _hook in ${_MK_CONFIGURE_PREHOOKS}
    do
        "$_hook"
    done
}

_mk_configure_posthooks()
{
    for _hook in ${_MK_CONFIGURE_POSTHOOKS}
    do
        "$_hook"
    done
}

_mk_make_prehooks()
{
    for _hook in ${_MK_MAKE_PREHOOKS}
    do
        "$_hook"
    done
}

_mk_make_posthooks()
{
    for _hook in ${_MK_MAKE_POSTHOOKS}
    do
        "$_hook"
    done
}

_mk_complete_hooks()
{
    for _hook in ${_MK_COMPLETE_HOOKS}
    do
        "$_hook"
    done
}

_mk_emit_make_header()
{
    _mk_emit "SHELL=${MK_SHELL} -- .MakeKitBuild"
    _mk_emit "MK_CONTEXT=MK_VERBOSE='\$(V)'; _mk_restore_context"
}

_mk_emit_make_footer()
{
    # Run make functions for all modules in reverse order
    _mk_reverse ${MK_MODULE_FILES}
    for _file in ${result}
    do
        _module="${_file##*/}"
        MK_MSG_DOMAIN="${_module%.sh}"

        unset -f make
        
        mk_source_or_fail "${_file}"
        
        if mk_function_exists make
        then
            _mk_make_prehooks
            make
            _mk_make_posthooks
        fi
    done

    _mk_emit ""
    _mk_emit "Makefile:${MK_BUILD_FILES}${MK_CONFIGURE_INPUTS}"
    _mk_emitf '\t@$(MK_CONTEXT) :; mk_msg "regenerating Makefile"; set -- %s; . %s; exit 0\n\n' "$MK_OPTIONS" "'${MK_HOME}/command/configure.sh'"

    for _target in ${MK_CONFIGURE_OUTPUTS}
    do
        _mk_emit "${_target}: Makefile"
        _mk_emit ""
    done

    _mk_emit "sinclude .MakeKitDeps/*.dep"
    _mk_emit ""
}

mk_add_configure_output()
{
    MK_CONFIGURE_OUTPUTS="$MK_CONFIGURE_OUTPUTS $1"
}

mk_add_configure_input()
{
    MK_CONFIGURE_INPUTS="$MK_CONFIGURE_INPUTS $1"
}

mk_help_recursive()
{
    unset -f option
    unset SUBDIRS
    
    mk_safe_source "${MK_SOURCE_DIR}${1}/MakeKitBuild" || mk_fail "Could not read MakeKitBuild in ${MK_SOURCE_DIR}${1}"

    if mk_function_exists option
    then
        if [ -z "$1" ]
        then
            echo "Options (${PROJECT_NAME}):"
        else
            echo "Options (${1#/}):"
        fi
        option
    fi
    
    for _dir in ${SUBDIRS}
    do
        if [ "$_dir" != "." ]
        then
            mk_help_recursive "$1/${_dir}"
        fi
    done
}

mk_help()
{
    echo "Usage: makekit configure [ option | @settings_file ] ..."

    {
        echo "Options:"
        _basic_options

        for _file in ${MK_MODULE_FILES}
        do
            _module="${_file##*/}"
            _module="${_module%.sh}"
            
            unset -f option
            
            mk_source_or_fail "${_file}"
            
            if mk_function_exists "option"
            then
                echo "Options ($_module):"
                option
            fi
        done
        
        if [ -f "${MK_SOURCE_DIR}/MakeKitBuild" ]
        then
            mk_help_recursive ""
        fi
    } | awk -f "${MK_HOME}/help.awk"
}

_basic_options()
{
    mk_option \
        VAR=MK_SOURCE_DIR \
        OPTION=sourcedir \
        PARAM=path \
        DEFAULT='.' \
        HELP="Source directory"
   
    mk_option \
        VAR=MK_OBJECT_DIR \
        OPTION=objectdir \
        PARAM=path \
        DEFAULT='object' \
        HELP="Intermediate file directory"
    
    mk_option \
        VAR=MK_STAGE_DIR \
        OPTION=stagedir \
        PARAM=path \
        DEFAULT='stage' \
        HELP="Staging directory"
    
    mk_option \
        VAR=MK_RUN_DIR \
        OPTION=rundir \
        PARAM=path \
        DEFAULT='run' \
        HELP="Build tool install directory"
        
    mk_option \
        VAR=MK_SHOW_VARS \
        OPTION=show-vars \
        PARAM='yes|no' \
        DEFAULT='no' \
        HELP="Always show options as variable names in help output"

    mk_option \
        VAR=MK_HELP \
        OPTION=help \
        PARAM='yes|no' \
        DEFAULT='no' \
        HELP="Show this help"
}

_mk_sort_params()
{
    mk_quote_list "$@"
    MK_OPTIONS="$result"

    # Sort through parameters and figure out what to do with them
    for _param in "$@"
    do
        case "$_param" in
            "@"*)
                # Read variables from file
                mk_source_or_fail "${_param#@}"
                ;;
            "--"*)
                # Leave it alone for now
                :
                ;;
            *"="*)
                # Set variable
                mk_set "${_param%%=*}" "${_param#*=}"
                ;;
        esac
    done
}

#
# Generates .MakeKitBuild in the build directory, which is used as the shell by make
#
# This script is a concatenation of the core MakeKit functions (mk.sh),
# all imported modules, and a short footer which executes the build command
# passed by make (build.sh).
# 
# Since this script is parsed and executed by the shell for every command make runs,
# we pass it through an awk script which strips it down as much as possible:
#
# - Comments are removed
# - Sections from modules that are only used when running configure are removed
#
# Each build action begins by invoking _mk_restore_context with the name of the 
# subdirectory which generated it.  This sources the .MakeKitExports and
# MakeKitBuild files for that subdirectory.
#
# The end result is that all build actions run in the same context as the
# make() function which produced them.  This makes writing build rules
# more convenient: you can call a helper function in your MakeKitBuild file and
# have it work as you would expect.
# 
_mk_emit_build_script()
{
    {
        echo "### section build"
        # Set essential variables
        for _var in MK_HOME MK_ROOT_DIR MK_SOURCE_DIR MK_OBJECT_DIR PATH
        do
            mk_get "$_var"
            mk_quote "$result"
            echo "$_var=$result"
        done
        echo ""
        cat "${MK_HOME}/mk.sh"
        echo ""
        for _file in ${MK_MODULE_FILES}
        do
            cat "$_file"
            echo ""
        done
        cat "${MK_HOME}/build.sh"
    } | awk -f "${MK_HOME}/build.awk" >.MakeKitBuild || mk_fail "could not write .MakeKitBuild"
}

# Sort through our command line parameters
_mk_sort_params "$@"

# Set up basic variables
MK_MSG_DOMAIN="makekit"
MK_ROOT_DIR="$PWD"
_basic_options

MK_SEARCH_DIRS="${MK_HOME}"

# Look for local modules and scripts in source directory
if [ -d "${MK_SOURCE_DIR}/mklocal" ]
then
    MK_SEARCH_DIRS="${MK_SEARCH_DIRS} ${MK_SOURCE_DIR}/mklocal"
fi

# Find all required modules
_mk_find_module_imports
_mk_module_list ${result}

# Allow top-level MakeKitBuild to set default settings
_mk_set_defaults

MK_MSG_DOMAIN="makekit"

if [ "$MK_HELP" = "yes" ]
then
    mk_help
    exit 0
fi

# Don't allow building in the source directory
_canon_sourcedir="`cd "${MK_SOURCE_DIR}" && pwd`"
_canon_rootdir="`cd "${MK_ROOT_DIR}" && pwd`"

if [ "$_canon_sourcedir" = "$_canon_rootdir" ]
then
    mk_fail "please run configure from a separate directory"
fi

unset _canon_sourcedir _canon_rootdir

# Open log file
exec 4>config.log
MK_LOG_FD=4

mk_msg "initializing"

# Open Makefile for writing
exec 6>.Makefile.new
MK_MAKEFILE_FD=6

# Export basic variables
mk_export ${BASIC_VARIABLES}

# Emit Makefile header
_mk_emit_make_header

# Process build files
_mk_process_build

# Run completion hooks
_mk_complete_hooks

# Emit Makefile footer
_mk_emit_make_footer

# Close and atomically replace Makefile
exec 6>&-
mv -f ".Makefile.new" "Makefile" || mk_fail "could not replace Makefile"

# Close log file
exec 4>&-

# Generate build script
_mk_emit_build_script
