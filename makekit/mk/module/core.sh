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
# core.sh -- implements core build logic
#
##

DEPENDS="platform program"

#<
# @var MK_DEBUG
# @brief Controls debug mode
# @export
# @value yes Debug mode is turned on
# @value no Debug mode is turned off
#
# Decides whether build products should be created
# in "debug" mode.  This is an abstract setting which
# is respected by other modules in whatever way is appropriate.
# For example, when debug mode is on the compiler module
# turns off optimization compiler flags and the package-deb
# module builds .deb package with unstripped binaries.
#>

### section common

_mk_declare_output()
{
    case " $_MK_OUTPUT_VARS " in
        *" $1 "*)
            return 0
            ;;
    esac

    _MK_OUTPUT_VARS="$_MK_OUTPUT_VARS $1"
}

#<
# @brief Declare special variables
# @usage [-i] [-e] [-s] [-o] vars...
# @usage [-i] [-e] [-s] [-o] var=value...
# @option -i Marks all listed variables as inherited by subdirectories.
#            Each subdirectory may override the variable by setting it
#            to a new value; the original value will be restored upon
#            returning to the parent directory.
# @option -e Mark all listed variables as exported.  Their values
#            will be available when the user runs <cmd>make</cmd>.
# @option -s Mark all listed variables as system-dependent.  The
#            variables may have different values for each target system.
#            The values will be swapped out when the target system is
#            changed with <funcref>mk_system</funcref>.
# @option -o Mark all listed variables as output variables.  Their
#            values will be substituted when processing a file with
#            <funcref>mk_output_file</funcref>.
#
# Marks each variable in the list with the above attributes.
# If the second form is used for a variable, it will also be assigned
# <param>value</param> at the same time.
#
# @example
# configure()
# {
#     mk_declare -e FOO="foo"
#     BAR="bar"
# }
#
# make()
# {
#     mk_target TARGET="foobar" my_custom_function '$@'
# }
#
# my_custom_function()
# {
#     # The value of FOOBAR is available here because it was exported
#     echo "$FOOBAR" > "$1"
#     # The value of BAR is not available, so this will not print "bar"
#     mk_msg "BAR=$BAR"
# }
#>
mk_declare()
{
    _inherited=false
    _exported=false
    _system=false
    _output=false

    while [ "$#" -gt 0 ]
    do
        case "$1" in
            '-e') _exported=true;;
            '-i') _inherited=true;;
            '-s') _system=true;;
            '-o') _output=true;;
            *'='*)
                _name="${1%%=*}"
                _val="${1#*=}"
                mk_set "$_name" "$_val"
                if $_system
                then
                    _mk_declare_system "$_name"
                    $_output && _mk_declare_output "$_name"
                else
                    $_inherited && _mk_declare_inherited "$_name"
                    $_exported && _mk_declare_exported "$_name"
                    $_output && _mk_declare_output "$_name"
                fi
                ;;
            *)
                if $_system
                then
                    _mk_declare_system "$1"
                    $_output && _mk_declare_output "$1"
                else
                    $_inherited && _mk_declare_inherited "$1"
                    $_exported && _mk_declare_exported "$1"
                    $_output && _mk_declare_output "$1"
                fi
                ;;
        esac
        shift
    done
}

mk_export()
{
    mk_deprecated "mk_export is deprecated; use mk_declare -e instead"

    for _export
    do
        case "$_export" in
            *"="*)
                _val="${_export#*=}"
                _name="${_export%%=*}"
                mk_set "$_name" "$_val"
                _mk_declare_exported "$_name"
                _mk_declare_inherited "$_name"
                ;;
            *)
                _mk_declare_exported "$_export"
                _mk_declare_inherited "$_export"
                ;;
        esac
    done
}

#<
# @brief Get value of exported variable
# @usage dir var
# 
# Gets the value of the exported variable
# <param>var</param> in the directory 
# <param>dir</param> (relative to the directory
# containing the current MakeKitBuild).  The
# specified directory must have been previously
# processed.
#
# The value is placed in <var>result</var>.
#>
mk_get_export()
{
    # $1 = directory (relative to MK_SUBDIR)
    # $2 = variable
    result=$(mk_safe_source "${MK_OBJECT_DIR}${MK_SUBDIR}/$1/.MakeKitExports" && 
             mk_get "$2" &&
             echo "$result")
    [ "$?" -eq 0 ] || mk_fail "could not read ${MK_OBJECT_DIR}${MK_SUBDIR}/$1/.MakeKitExports"
}

_mk_get_stage_targets_filter()
{
    mk_unquote_list "$STAGE_TARGETS"
    for _target
    do
        if "$SELECT" "$_target"
        then
            mk_quote "$_target"
            TARGETS="$TARGETS $result"
        fi
    done
}

_mk_get_stage_targets_rec()
{
    mk_push_vars DIR="$1"

    STAGE_TARGETS=""
    TARGET_SUBDIRS=""

    if [ "${DIR%/}" = "${MK_OBJECT_DIR}${MK_SUBDIR}" ]
    then
        STAGE_TARGETS="$MK_SUBDIR_TARGETS"
        TARGET_SUBDIRS="$MK_SUBDIRS"
    else 
        mk_safe_source "$DIR/.MakeKitTargets"
    fi

    if [ -n "$STAGE_TARGETS" ]
    then
        _mk_get_stage_targets_filter
    fi

    if [ -n "$TARGET_SUBDIRS" ]
    then
        mk_unquote_list "$TARGET_SUBDIRS"
        for SUBDIR
        do
            [ "$SUBDIR" != "." ] && _mk_get_stage_targets_rec "$DIR/$SUBDIR"
        done
    fi

    mk_pop_vars
}

_mk_compile_select()
{
    set -f
    mk_unquote_list "$SELECT"
    set +f
    mk_fnmatch_compile "$@"
    SELECT="$result"
}

mk_get_stage_targets()
{
    mk_push_vars \
        SELECT="*" \
        CLEAN_TARGETS STAGE_TARGETS TARGET_SUBDIRS \
        DIR SUBDIR TARGETS
    mk_parse_params

    _mk_compile_select

    for DIR
    do
        case "$DIR" in
            "@"*)
                _mk_get_stage_targets_rec "${MK_OBJECT_DIR}/${DIR#@}"
                ;;
            *)
                _mk_get_stage_targets_rec "${MK_OBJECT_DIR}${MK_SUBDIR}/${DIR}"
                ;;
        esac
    done

    result="${TARGETS# }"
    mk_pop_vars
    unset -f "$SELECT"
}

_mk_get_clean_targets_rec()
{
    mk_push_vars DIR="$1"
    if mk_safe_source "$DIR/.MakeKitTargets"
    then
        TARGETS="$TARGETS $CLEAN_TARGETS"
        mk_unquote_list "$TARGET_SUBDIRS"
        for SUBDIR
        do
            [ "$SUBDIR" != "." ] && _mk_get_clean_targets_rec "$DIR/$SUBDIR"
        done
    fi
    mk_pop_vars
}

mk_get_clean_targets()
{
    mk_push_vars \
        TARGET_SUBDIRS CLEAN_TARGETS STAGE_TARGETS DIR SUBDIR TARGETS # temporaries
    mk_parse_params

    for DIR
    do
        case "$DIR" in
            "@"*)
                _mk_get_clean_targets_rec "${MK_OBJECT_DIR}/${DIR#@}"
                ;;
            *)
                _mk_get_clean_targets_rec "${MK_OBJECT_DIR}${MK_SUBDIR}/${DIR}"
                ;;
        esac
    done

    result="${TARGETS# }"
    mk_pop_vars
}

mk_safe_rm()
{
    if [ -z "${MK_ROOT_DIR}" -o "$PWD" != "${MK_ROOT_DIR}" ]
    then
        mk_fail "CRITICAL: attempt to mk_safe_rm outside of build directory"
    fi

    mk_normalize_path "$1"
    
    case "${result}" in
        '/'*|'..'|'..'/*)
            mk_fail "CRITICAL: attempt to mk_safe_rm path that escape build directory: $result"
            ;;
    esac

    mk_run_or_fail rm -rf -- "$result"
}

#<
# @brief Note deprecated function usage
# @usage message...
#
# If warn-on-deprecated is on, warns the user about deprecated
# function usage by passing <param>message</param> to
# <funcref>mk_warn</funcref>.
#>
mk_deprecated()
{
    [ "$MK_WARN_DEPRECATED" = "yes" ] && mk_warn "$@"
}

mk_run_script()
{
    if _mk_find_resource "script/${1}.sh"
    then
        shift
        mk_parse_params
        . "$result"
        return "$?"
    else
        mk_fail "could not find script: $1"
    fi
}

mk_resolve_target()
{
    case "$1" in
        "@"*)
            # Already an absolute target, leave as is
            result="$1"
            ;;
        *)
            # Resolve to absolute target
            case "$1" in
                "/"*)
                    # Input is a product in the staging area
                    result="@${MK_STAGE_DIR}$1"
                    ;;
                *)
                    __source_file="${MK_SOURCE_DIR}${MK_SUBDIR}/${1}"
                    
                    if [ -e "${__source_file}" ]
                    then
                        # Input is a source file
                        result="@${__source_file}"
                    else
                        # Input is an object file
                        # Makefile targets are matched verbatim, so
                        # we need to normalize the file path so that paths
                        # with '.' or '..' are reduced to the canonical form
                        # that appears on the left hand side of make rules.
                        mk_normalize_path "${MK_OBJECT_DIR}${MK_SUBDIR}/${1}"
                        result="@$result"
                    fi
                    ;;
            esac
            ;;
    esac
}

__mk_resolve()
{
    # Accumulator variable
    __resolve_result=""
    # Save the resolve function and quote function
    __resolve_func="$2"
    __resolve_quote="$3"
    # Save the current directory
    __resolve_PWD="$PWD"
    if [ "$MK_SUBDIR" != ":" ]
    then
        # Change to the source subdirectory so that pathname expansion picks up source files.
        cd "${MK_SOURCE_DIR}${MK_SUBDIR}" || mk_fail "could not change to directory ${MK_SOURCE_DIR}${MK_SUBDIR}"
    fi
    # Unquote the list into the positional parameters.  This will perform pathname expansion.
    mk_unquote_list "$1"
    # Restore the current directory
    cd "$__resolve_PWD"

    # For each expanded item
    for __resolve_item in "$@"
    do
        # Resolve the item to a fully-qualified target/file using the resolve function
        "$__resolve_func" "$__resolve_item"
        # Quote the result using the quote function
        "$__resolve_quote" "$result"
        # Accumulate
        __resolve_result="$__resolve_result $result"
    done

    # Strip off the leading space
    result="${__resolve_result# }"
}

mk_resolve_file()
{
    mk_resolve_target "$@"
    result="${result#@}"
}

mk_resolve_targets()
{
    __mk_resolve "$1" mk_resolve_target mk_quote
}

mk_resolve_files_space()
{
    __mk_resolve "$1" mk_resolve_file mk_quote_space
}

mk_resolve_files()
{
    __mk_resolve "$1" mk_resolve_file mk_quote
}

### section configure

mk_skip_subdir()
{
    __skip="$1"
    
    mk_unquote_list "$SUBDIRS"
    SUBDIRS=""
    for __subdir in "$@"
    do
        [ "$__subdir" = "$__skip" ] || SUBDIRS="$SUBDIRS $__subdir"
    done

    # Trim skipped directory out of object tree if it
    # exists from a previous configure run
    mk_safe_rm "${MK_OBJECT_DIR}${MK_SUBDIR}/$__skip"

    unset __skip __subdir
}

mk_comment()
{
    _mk_emitf "\n#\n# %s\n#\n" "$*"
}

_mk_rule()
{
    __lhs="$1"
    shift
    __command="$1"
    shift

    if [ -n "$__command" ]
    then
        _mk_emitf '\n%s: %s\n\t@$(MK_CONTEXT) "%s"; mk_system "%s"; \\\n\t%s\n' "$__lhs" "${*# }" "$MK_SUBDIR" "$SYSTEM" "${__command# }"
    else
        _mk_emitf '\n%s: %s\n' "$__lhs" "${*# }"
    fi
}

_mk_build_command()
{
    for __param
    do
        case "$__param" in
            "%<"|"%>"|"%<<"|"%>>"|"%;")
                __command="$__command ${__param#%}"
                ;;
            "@"*)
                mk_quote "${__param#@}"
                __command="$__command $result"
                ;;
            "&"*)
                mk_resolve_files "${__param#&}"
                __command="$__command $result"
                ;;
            "%"*)
                mk_get "${__param#%}"

                if [ -n "$result" ]
                then
                    mk_quote "${__param#%}=$result"
                    __command="$__command $result"
                fi
                ;;
            "#"*)
                __command="$__command ${__param#?}"
                ;;
            "*"*)
                _mk_build_command_expand "${__param#?}"
                ;;
            *)
                mk_quote "$__param"
                __command="$__command $result"
                ;;
        esac
    done
}

_mk_build_command_expand()
{
    mk_unquote_list "$1"
    _mk_build_command "$@"
}

mk_target()
{
    mk_push_vars TARGET DEPS SYSTEM="$MK_SYSTEM"
    mk_parse_params

    __resolved=""
    __command=""

    _mk_build_command "$@"

    mk_resolve_files_space "$DEPS"
    __resolved="$result"

    mk_resolve_target "$TARGET"
    __target="$result"
    mk_quote_space "${result#@}"

    _mk_rule "$result" "${__command}" "${__resolved}"

    case "$__target" in
        "@${MK_STAGE_DIR}"/*)
            mk_quote "$__target"
            MK_SUBDIR_TARGETS="$MK_SUBDIR_TARGETS $result"
            MK_STAGE_TARGETS="$MK_STAGE_TARGETS $result"
            ;;
        "@${MK_OBJECT_DIR}"/*)
            mk_add_clean_target "$__target"
            ;;
    esac

    mk_pop_vars

    result="$__target"
}

mk_install_file()
{
    mk_deprecated "mk_install_file is deprecated; use mk_stage"
    mk_push_vars FILE INSTALLFILE INSTALLDIR MODE
    mk_parse_params

    if [ -z "$INSTALLFILE" ]
    then
        INSTALLFILE="$INSTALLDIR/$FILE"
    fi

    mk_resolve_target "$FILE"
    _resolved="$result"

    mk_target \
        TARGET="$INSTALLFILE" \
        DEPS="'$_resolved' $*" \
        mk_run_script install %MODE '$@' "$_resolved"

    mk_pop_vars
}

mk_install_files()
{
    mk_deprecated "mk_install_files is deprecated; use mk_stage"
    mk_push_vars INSTALLDIR FILES MODE
    mk_parse_params

    unset _inputs

    mk_quote_list "$@"
    mk_unquote_list "$FILES $result"

    for _file
    do
        mk_install_file \
            INSTALLDIR="$INSTALLDIR" \
            FILE="$_file" \
            MODE="$MODE"
    done

    mk_pop_vars
}

#<
# @brief Create symlink in staging area
# @usage TARGET=target LINK=link
#
# @option DEPS=deps
# Specify additional dependencies of <param>link</param>
# other than <param>target</param>
#
# Creates a symlink at <param>link</param> which points to
# <param>target</param>.
#
# @example
# # Make symlink /etc/foobar.conf that points to foobar.conf.default
#
# mk_symlink LINK=/etc/foobar.conf TARGET=foobar.conf.default
# @endexample
#>
mk_symlink()
{
    mk_push_vars TARGET LINK DEPS
    mk_parse_params
    
    [ -z "$TARGET" ] && TARGET="$1"
    [ -z "$LINK" ] && LINK="$2"

    case "$TARGET" in
        @*)
            mk_quote "$TARGET"
            DEPS="$DEPS $result"
            TARGET="${TARGET#@}"
            ;;
        /*)
            mk_quote "$TARGET"
            DEPS="$DEPS $result"
            ;;
        *)
            mk_quote "${LINK%/*}/${TARGET}"
            DEPS="$DEPS $result"
            ;;
    esac

    mk_target \
        TARGET="$LINK" \
        DEPS="$DEPS" \
        _mk_core_symlink "$TARGET" "&$LINK"

    mk_pop_vars
}

#<
# @brief Install files or directories into staging area
#
# @usage SOURCE=source_path DEST=dest_path
# @usage SOURCE=source_path DESTDIR=dest_dir
# @usage DESTDIR=dest_dir sources...
#
# @option MODE=mode
# Specifies the UNIX mode of the destination files
# or directories
# @option DEPS=deps
# Specifies additional dependencies of the destination
# files or directories other than the sources.
#
# Defines targets that copy one or more files or directories into
# the staging area.  In the first form, a single file or directory
# is copied, with the <param>dest_path</param> specifying the complete
# destination path.  In the second form, the <param>dest_dir</param>
# parameter specifies the directory into which the source will be
# copied; the name of the source will be preserved.  In the third form,
# multiple sources are copied into <param>dest_dir</param>.
#
# @example
# # Copy a single file or directory
# mk_stage SOURCE=foobar.conf.example DEST=/etc/foobar.conf
# mk_stage SOURCE=foobar.d.example DEST=/etc/foobar.d
#
# # Copy a single file or directory, specifying destination directory
# Source filename is preserved
# mk_stage SOURCE=foobar.conf DESTDIR=/etc
# mk_stage SOURCE=foobar.d DESTDIR=/etc
#
# # Copy multiple files and directories to destination directory
# Source filenames are preserved
# mk_stage DESTDIR=/etc foo.conf bar.conf
# @endexample
#>
mk_stage()
{
    mk_push_vars SOURCE DEST SOURCEDIR DESTDIR RESULTS MODE DEPS
    mk_parse_params

    if [ -n "$SOURCE" -a -n "$DEST" ]
    then
        mk_quote "$SOURCE"
        SOURCE="$result"
        mk_quote "$DEST"
        mk_target \
            TARGET="$DEST" \
            DEPS="$SOURCE $DEPS" \
            _mk_core_stage "&$result" "&$SOURCE" "$MODE"
    elif [ -n "$SOURCE" -a -n "$DESTDIR" ]
    then
        mk_stage \
            SOURCE="$SOURCE" \
            DEST="$DESTDIR/${SOURCE##*/}" \
            MODE="$MODE" \
            DEPS="$DEPS"
    elif [ -n "$DESTDIR" ]
    then
        for _file
        do
            mk_stage \
                SOURCE="${SOURCEDIR:+$SOURCEDIR/}$_file" \
                DEST="$DESTDIR/$_file" \
                MODE="$MODE" \
                DEPS="$DEPS"
            mk_quote "$result"
            RESULTS="$RESULTS $result"
        done
        result="$RESULTS"
    else
        mk_fail "invalid parameters to mk_stage"
    fi

    mk_pop_vars
}

#<
# @brief Output a file with variable substitutions
#
# @usage INPUT=source_path OUTPUT=dest_path
# @usage dest_path
#
# Processes the given <param>source_path</param>, substituting
# anything of the form <lit>@<var>VAR</var>@</lit> with the value
# of <var>VAR</var>, where <var>VAR</var> is an output variable
# as declared by <cmd><funcref>mk_declare</funcref> -o</cmd>.
#
# Both paths are resolved according to the usual target notation
# (see <funcref>mk_resolve_target</funcref>).  In the most common
# usage, the source will be in the source directory and the
# destination will be in the object directory.
#
# If the second form is used, the input file is assumed to be
# the same as the output file suffixed with <lit>.in</lit>.
#>
mk_output_file()
{
    mk_push_vars INPUT OUTPUT
    mk_parse_params

    [ -z "$OUTPUT" ] && OUTPUT="$1"
    [ -z "$INPUT" ] && INPUT="${OUTPUT}.in"

    # Emit an awk script that will perform replacements
    {
        echo "{"
        
        for _export in ${_MK_OUTPUT_VARS}
        do
            mk_get "$_export"
            mk_quote_c_string "$result"

            echo "    gsub(\"@${_export}@\", $result);"
        done

        echo "    print \$0;"
        echo "}"
    } > ".awk.$$"

    mk_resolve_file "${INPUT}"
    _input="$result"
    mk_resolve_file "${OUTPUT}"
    _output="$result"

    mk_mkdir "${_output%/*}"
    # First copy the file to preserve mode bits.
    # There ought to be a better way to do this...
    mk_run_or_fail cp "$_input" "${_output}.new"
    # Now overwrites its contents
    # We don't use mk_run_or_fail because it can log
    # output if MK_VERBOSE is set
    ${AWK} -f ".awk.$$" < "$_input" > "${_output}.new" || mk_fail "could not run awk"
    mk_run_or_fail rm -f ".awk.$$"

    if [ -f "${_output}" ] && diff "${_output}" "${_output}.new" >/dev/null 2>&1
    then
        mk_run_or_fail rm -f "${_output}.new"
    else
        mk_run_or_fail mv "${_output}.new" "${_output}"
    fi

    mk_add_configure_output "@${_output}"
    mk_add_configure_input "@${_input}"

    result="@$_output"

    mk_pop_vars
}

mk_add_clean_target()
{
    mk_push_vars result
    mk_quote "$1"
    MK_CLEAN_TARGETS="$MK_CLEAN_TARGETS $result"
    mk_pop_vars
}

mk_add_scrub_target()
{
    mk_push_vars result
    mk_quote "$1"
    MK_SCRUB_TARGETS="$MK_SCRUB_TARGETS $result"
    mk_pop_vars
}

mk_add_nuke_target()
{
    mk_push_vars result
    mk_quote "${1#@}"
    MK_NUKE_TARGETS="$MK_NUKE_TARGETS $result"
    mk_pop_vars
}

mk_add_all_target()
{
    mk_push_vars result
    mk_quote "$1"
    MK_ALL_TARGETS="$MK_ALL_TARGETS $result"
    mk_pop_vars
}

mk_add_phony_target()
{
    mk_push_vars result
    mk_quote "$1"
    MK_PHONY_TARGETS="$MK_PHONY_TARGETS $result"
    mk_pop_vars
}

mk_add_subdir_target()
{
    mk_push_vars result
    mk_quote "$1"
    MK_SUBDIR_TARGETS="$MK_SUBDIR_TARGETS $result"
    mk_pop_vars
}

mk_add_configure_output()
{
    mk_push_vars result
    mk_resolve_target "$1"
    mk_quote "$result"
    MK_CONFIGURE_OUTPUTS="$MK_CONFIGURE_OUTPUTS $result"
    mk_pop_vars
}

mk_add_configure_input()
{
    mk_push_vars result
    mk_resolve_target "$1"
    mk_quote "$result"
    MK_CONFIGURE_INPUTS="$MK_CONFIGURE_INPUTS $result"
    mk_pop_vars
}

mk_msg_checking()
{
    if [ "$MK_SYSTEM" = "host" ]
    then
        mk_msg_begin "$*: "
    else
        mk_msg_begin "$* ($MK_SYSTEM): "
    fi
}

mk_msg_result()
{
    mk_msg_end "$*"
}

#<
# @brief Check for cached test result
# @usage var
#
# Checks if a variable has been previously
# cached by <funcref>mk_cache</funcref>.  On success,
# both <param>var</param> and <var>result</var> are
# set the the cached value, and true is returned.
# Otherwise, false is returned.
#>
mk_check_cache()
{
    mk_declare -s -i "$1"

    _mk_define_name "CACHED_$MK_CANONICAL_SYSTEM"
    if mk_is_set "${1}__${result}"
    then
        if [ "${MK_SYSTEM%/*}" = "$MK_SYSTEM" ]
        then
            mk_get "${1}__${result}"
            mk_set_all_isas "$1" "$result"
            return 0
        else
            mk_get "${1}__${result}"
            mk_set "$1" "$result"
            return 0
        fi
    else
        return 1
    fi
}

#<
# @brief Cache test result
# @usage var value
#
# Sets <param>var</param> to <param>value</param> and
# stores the result in the cache.
#>
mk_cache()
{
    __systems=""
    if [ "${MK_SYSTEM%/*}" = "$MK_SYSTEM" ]
    then
        for __isa in ${MK_ISAS}
            do
                __systems="$__systems $MK_SYSTEM/$__isa"
        done
    else
        __systems="$MK_CANONICAL_SYSTEM"
    fi    
    for __system in ${__systems}
    do
        _mk_define_name "CACHED_$MK_CANONICAL_SYSTEM"
        MK_CACHE_VARS="$MK_CACHE_VARS ${1}__${result}"
        mk_set "${1}__${result}" "$2"
        mk_set_system_var SYSTEM="$__system" "$1" "$2"
    done

    result="$2"
}

_mk_save_cache()
{
    {
        mk_quote "$MK_OPTIONS"
        echo "__OPTIONS=$result"
        echo 'if [ "$MK_OPTIONS" = "$__OPTIONS" ]; then'
        for __var in ${MK_CACHE_VARS}
        do
            mk_get "$__var"
            mk_quote "$result"
            echo "$__var=$result"
        done
        echo "MK_CACHE_VARS='${MK_CACHE_VARS# }'"
        echo 'fi'
    } > .MakeKitCache
}

_mk_load_cache()
{
    mk_safe_source "./.MakeKitCache"
}

option()
{
    mk_option \
        OPTION="log-dir" \
        VAR="MK_LOG_DIR" \
        PARAM="dir" \
        DEFAULT="log" \
        HELP="Directory where misc. logs should be placed"

    mk_option \
        OPTION="warn-deprecated" \
        VAR="MK_WARN_DEPRECATED" \
        PARAM="yes|no" \
        DEFAULT="no" \
        HELP="Warn about deprecated function use"

    mk_option \
        OPTION="fail-on-warn" \
        VAR="MK_FAIL_ON_WARN" \
        PARAM="yes|no" \
        DEFAULT="no" \
        HELP="Fail on warnings"

    mk_option \
        OPTION="debug" \
        VAR="MK_DEBUG" \
        PARAM="yes|no" \
        DEFAULT="no" \
        HELP="Build in debug mode"
}

configure()
{
    mk_declare -i _MK_OUTPUT_VARS
    mk_declare -e MK_LOG_DIR MK_DEBUG

    # Check for best possible awk
    mk_check_program VAR=AWK FAIL=yes gawk awk

    MK_CLEAN_TARGETS=""
    MK_SCRUB_TARGETS=""
    MK_NUKE_TARGETS=""    
    MK_BUILD_FILES=""

    for _module in ${MK_MODULE_LIST}
    do
        _mk_find_resource "module/${_module}.sh" || mk_fail "internal error"
        mk_quote "@$result"
        MK_BUILD_FILES="$MK_BUILD_FILES $result"
    done

    mk_add_configure_prehook _mk_core_configure_pre

    # Add a post-make() hook to write out a rule
    # to build all relevant targets in that subdirectory
    mk_add_make_posthook _mk_core_write_subdir_rule

    # Add post-make() hook to write out file with summary
    # of targets generated in the current directory
    mk_add_make_posthook _mk_core_write_targets_file
   
    # Emit the default target
    mk_target \
        TARGET="@default" \
        DEPS="@all"
    
    mk_add_phony_target "$result"

    # Load configure check cache if there is one
    _mk_load_cache
}

make()
{
    MK_CLEAN_TARGETS="$MK_CLEAN_TARGETS @$MK_LOG_DIR @$MK_RUN_DIR"
    MK_NUKE_TARGETS="$MK_NUKE_TARGETS $MK_OBJECT_DIR $MK_STAGE_DIR Makefile config.log .MakeKitCache .MakeKitBuild .MakeKitExports .MakeKitDeps"

    mk_target \
        TARGET="@clean" \
        mk_run_script clean '$(SUBDIR)' "*$MK_CLEAN_TARGETS"

    mk_add_phony_target "$result"

    mk_target \
        TARGET="@scrub" \
        DEPS="@clean" \
        mk_run_script scrub '$(SUBDIR)' "*$MK_SCRUB_TARGETS"

    mk_add_phony_target "$result"

    mk_target \
        TARGET="@nuke" \
        mk_run_script nuke "*$MK_CLEAN_TARGETS" "*$MK_SCRUB_TARGETS" "*$MK_NUKE_TARGETS"

    mk_add_phony_target "$result"

    mk_target \
        TARGET="@install" \
        DEPS="@all" \
        _mk_core_install '$(DESTDIR)'
    
    mk_add_phony_target "$result"

    mk_target \
        TARGET="@uninstall" \
        _mk_core_uninstall
    
    mk_add_phony_target "$result"

    mk_target \
        TARGET="@.PHONY" \
        DEPS="$MK_PHONY_TARGETS"

    mk_comment "Rule to regenerate Makefile"

    mk_target \
        TARGET="@Makefile" \
        DEPS="$MK_BUILD_FILES $MK_CONFIGURE_INPUTS" \
        mk_msg "regenerating Makefile" '%;' \
        export MK_HOME MK_SHELL MK_SOURCE_DIR PATH '%;' \
        "$MK_SHELL" "$MK_HOME/command/configure.sh" "#$MK_OPTIONS" '%;' \
        exit 0

    mk_comment "Dummy targets for files needed by configure"

    mk_unquote_list "$MK_BUILD_FILES" "$MK_CONFIGURE_INPUTS"
    for _target
    do
        mk_target TARGET="$_target"
    done

    mk_comment "Targets for files output by configure"

    mk_unquote_list "$MK_CONFIGURE_OUTPUTS"
    for _target
    do
        mk_target TARGET="$_target" DEPS="@Makefile"
    done

    _mk_emit ""
    _mk_emit "sinclude .MakeKitDeps/*.dep"
    _mk_emit ""

    # Save configure check cache
    _mk_save_cache
}

_mk_core_configure_pre()
{
    mk_quote "@$MK_CURRENT_FILE"
    MK_BUILD_FILES="$MK_BUILD_FILES $result"
}

_mk_core_write_subdir_rule()
{
    if [ "$MK_SUBDIR" != ":" ]
    then
        _targets=""
        mk_unquote_list "$SUBDIRS"
        for __subdir in "$@"
        do
            if [ "$__subdir" != "." ]
            then
                mk_quote "@${MK_OBJECT_DIR}${MK_SUBDIR}/$__subdir/.subdir_stamp"
                _targets="$_targets $result"
            fi
        done
        mk_comment "virtual target for subdir ${MK_SUBDIR#/}"

        mk_target \
            TARGET=".subdir_stamp" \
            DEPS="$MK_SUBDIR_TARGETS $_targets" \
            mk_run_or_fail touch "&.subdir_stamp"

        if [ "$MK_SUBDIR" = "" ]
        then
            _target="@all"
        else
            _target="@${MK_SUBDIR#/}"
        fi

        mk_quote "$result"

        mk_target \
            TARGET="$_target" \
            DEPS="$result"

        mk_add_phony_target "$result"
    fi

    unset MK_SUBDIR_TARGETS
}

_mk_core_write_targets_file()
{
    if [ "$MK_SUBDIR" != ":" ]
    then
        {
            mk_quote "$SUBDIRS"
            echo "TARGET_SUBDIRS=$result"
            mk_quote "$MK_STAGE_TARGETS"
            echo "STAGE_TARGETS=$result"
            mk_quote "$MK_CLEAN_TARGETS"
            echo "CLEAN_TARGETS=$result"
        } > "${MK_OBJECT_DIR}${MK_SUBDIR}/.MakeKitTargets" ||
        mk_fail "could not write ${MK_OBJECT_DIR}${MK_SUBDIR}/.MakeKitTargets"

        unset MK_STAGE_TARGETS MK_CLEAN_TARGETS
    fi
}

### section build

_mk_core_copy()
{
    mk_mkdir "${DESTDIR}${1%/*}"
    
    if [ -d "${MK_STAGE_DIR}${1}" ]
    then
        find "${MK_STAGE_DIR}${1}" -type f -o -type l |
        while read -r _file
        do
            _mk_core_copy "${_file#$MK_STAGE_DIR}"
        done || exit 1
    else
        mk_msg "$1"
        if [ -f "${DESTDIR}${1}" -o -h "${DESTDIR}${1}" ]
        then
            mk_run_or_fail rm -f -- "${DESTDIR}${1}"
        fi
        mk_mkdirname "${DESTDIR}${1}"
        ${CP_CMD} "${MK_STAGE_DIR}${1}" "${DESTDIR}${1}"
    fi
}

_mk_core_solaris_cp()
{
    if [ -h "$1" ]
    then
        _dest=`file -h "$1"`
        _dest=${_dest#*"symbolic link to "}
        mk_run_or_fail ln -s -- "$_dest" "$2"
    else
        mk_run_or_fail cp -f -- "$1" "$2"
    fi
}

_mk_core_install()
{
    DESTDIR="${1%/}"

    # We need to select a command that will preserve symlinks
    case "$MK_BUILD_OS" in
        hpux)
            # HP-UX does not support -P, but behaves correctly by default
            CP_CMD="mk_run_or_fail cp -Rf --"
            ;;
        solaris)
            # Solaris cp cannot preserve symlinks, so improvise
            CP_CMD="_mk_core_solaris_cp"
            ;;
        *)
            # This is works if cp is POSIX-comliant
            CP_CMD="mk_run_or_fail cp -RPf --"
            ;;
    esac

    mk_msg_domain "install"

    mk_get_stage_targets SELECT="*" "@"
    mk_unquote_list "$result"
   
    for _target
    do
        _mk_core_copy "${_target#@$MK_STAGE_DIR}"
    done
}

_mk_core_symlink()
{
    # $1 = target
    # $2 = link
    mk_msg_domain "symlink"

    mk_msg "${2#$MK_STAGE_DIR} -> $1"
    mk_mkdir "${2%/*}"
    # ln -f doesn't work everywhere, so remove the file by hand first
    mk_safe_rm "$2"
    mk_run_or_fail ln -s "$1" "$2"
}

_mk_core_stage()
{
    # $1 = dest
    # $2 = source
    # ($3) = mode
    mk_msg_domain "stage"

    mk_msg "${1#$MK_STAGE_DIR}"
    mk_mkdir "${1%/*}"
    [ -d "$1" ] && mk_safe_rm "$1"
    mk_run_or_fail cp -r "$2" "$1"
    mk_run_or_fail touch "$1"

    if [ -n "$3" ]
    then
        mk_run_or_fail chmod "$3" "$1"
    fi
}

_mk_core_confirm_uninstall()
{
    response=""

    while true
    do
        printf "Remove directory %s (y/n): " "$1"
        read response
        case "$response" in
            y)
                return 0
                ;;
            n)
                return 1
                ;;
        esac
    done
}

_mk_core_uninstall()
{
    mk_msg_domain "uninstall"

    mk_get_stage_targets "@$1"
    mk_unquote_list "$result"

    for _target
    do
        _file="${_target#@$MK_STAGE_DIR}"
        if [ -d "$_file" ]
        then
            if _mk_core_confirm_uninstall "$_file"
            then
                mk_msg "$_file"
                rm -rf "$_file"
            fi
        elif [ -e "$_file" -o -h "$_file" ]
        then
            mk_msg "$_file"
            rm -f "$_file"
        fi
    done
}