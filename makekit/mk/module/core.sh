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

### section common

mk_get_export()
{
    # $1 = directory (relative to MK_SUBDIR)
    # $2 = variable
    result=$(mk_safe_source "${MK_OBJECT_DIR}${MK_SUBDIR}/$1/.MakeKitExports" && 
             mk_get "$2" &&
             echo "$result")
    [ "$?" -eq 0 ] || mk_fail "could not read ${MK_OBJECT_DIR}${MK_SUBDIR}/$1/.MakeKitExports"
}

mk_run_or_fail()
{
    mk_quote_list "$@"
    mk_msg_verbose "+ $result"
    
    if ! "$@"
    then
        mk_msg "FAILED: $result"
        exit 1
    fi
}

mk_run_quiet_or_fail()
{
    mk_quote_list "$@"
    mk_msg_verbose "+(q) $result"
    
    __log="`"$@" 2>&1`"

    if [ "$?" -ne 0 ]
    then
        echo "$__log" >&2
        mk_msg "FAILED: $result"
        exit 1
    fi
}

mk_cd_or_fail()
{
    mk_quote "$1"
    mk_msg_verbose "+ 'cd' $result"
    cd "$1" || mk_fail "FAILED: 'cd' $result"
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

mk_warn()
{
    if [ -n "$MK_FAIL_ON_WARN" ]
    then
        mk_fail "$@"
    else
        mk_msg "WARNING: $*"
        sleep 1
    fi
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

    unset __skip __subdir
}

mk_comment()
{
    _mk_emit ""
    _mk_emit "#"
    _mk_emit "# $*"
    _mk_emit "#"
}

_mk_rule()
{
    __lhs="$1"
    shift
    __command="$1"
    shift

    if [ -n "$__command" ]
    then
        _mk_emitf '\n%s: %s\n\t@$(MK_CONTEXT) "%s"; mk_system "%s"; \\\n\t%s\n' "$__lhs" "${*# }" "$MK_SUBDIR" "$MK_SYSTEM" "${__command# }"
    else
        _mk_emitf '\n%s: %s\n' "$__lhs" "${*# }"
    fi
}

_mk_build_command()
{
    for __param in "$@"
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
    mk_push_vars TARGET DEPS
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
            mk_add_subdir_target "$__target"
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

    mk_add_all_target "$result"

    mk_pop_vars
}

mk_install_files()
{
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

mk_symlink()
{
    mk_push_vars TARGET LINK DEPS
    mk_parse_params
    
    [ -z "$TARGET" ] && TARGET="$1"
    [ -z "$LINK" ] && LINK="$2"

    case "$TARGET" in
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

    mk_add_all_target "$result"

    mk_pop_vars
}

mk_stage()
{
    mk_push_vars SOURCE DEST SOURCEDIR DESTDIR RESULTS MODE DEPS
    mk_parse_params

    if [ -n "$SOURCE" -a -n "$DEST" ]
    then
        mk_quote "$SOURCE"
        mk_target \
            TARGET="$DEST" \
            DEPS="$result $DEPS" \
            _mk_core_stage "&$DEST" "&$SOURCE" "$MODE"
        _result="$result"
        mk_add_all_target "$result"
        result="$_result"
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

mk_output_file()
{
    mk_push_vars INPUT OUTPUT
    mk_parse_params

    [ -z "$OUTPUT" ] && OUTPUT="$1"
    [ -z "$INPUT" ] && INPUT="${OUTPUT}.in"

    # Emit an awk script that will perform replacements
    {
        echo "{"
        
        for _export in ${MK_EXPORTS}
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
    awk -f ".awk.$$" < "$_input" > "${_output}.new" || mk_fail "could not run awk"
    mk_run_or_fail rm -f ".awk.$$"

    if [ -f "${_output}" ] && diff "${_output}" "${_output}.new" >/dev/null 2>&1
    then
        mk_run_or_fail rm -f "${_output}.new"
    else
        mk_run_or_fail mv "${_output}.new" "${_output}"
    fi

    mk_add_configure_output "${_output}"
    mk_add_configure_input "${_input}"

    result="@$_output"

    mk_pop_vars
}

mk_add_clean_target()
{
    mk_quote "$1"
    MK_CLEAN_TARGETS="$MK_CLEAN_TARGETS $result"
}

mk_add_scrub_target()
{
    mk_quote "${1#@}"
    MK_SCRUB_TARGETS="$MK_SCRUB_TARGETS $result"
}

mk_add_all_target()
{
    mk_quote "$1"
    MK_ALL_TARGETS="$MK_ALL_TARGETS $result"
}

mk_add_phony_target()
{
    mk_quote "$1"
    MK_PHONY_TARGETS="$MK_PHONY_TARGETS $result"
}

mk_add_subdir_target()
{
    mk_quote "$1"
    MK_SUBDIR_TARGETS="$MK_SUBDIR_TARGETS $result"
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

mk_check_cache()
{
    mk_declare_system_var "$1"

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
        for __var in ${MK_CACHE_VARS}
        do
            mk_get "$__var"
            mk_quote "$result"
            echo "$__var=$result"
        done
        echo "MK_CACHE_VARS='${MK_CACHE_VARS# }'"
    } > .MakeKitCache
}

_mk_load_cache()
{
    mk_safe_source "./.MakeKitCache"
}

option()
{
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
    # Default clean targets
    MK_CLEAN_TARGETS="${MK_RUN_DIR}"

    # Default scrub targets
    MK_SCRUB_TARGETS="${MK_STAGE_DIR}"

    # Default nuke targets (scrub targets implicitly included)
    MK_NUKE_TARGETS="${MK_OBJECT_DIR} ${MK_RUN_DIR} Makefile config.log .MakeKitCache .MakeKitBuild .MakeKitExports .MakeKitDeps"

    # Add a post-make() hook to write out a rule
    # to build all staging targets in that subdirectory
    mk_add_make_posthook _mk_core_write_subdir_rule
   
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
    mk_target \
        TARGET="@all" \
        DEPS="$MK_ALL_TARGETS"

    mk_add_phony_target "$result"

    mk_target \
        TARGET="@clean" \
        mk_run_script clean '$(SUBDIR)' "*$MK_CLEAN_TARGETS"

    mk_add_phony_target "$result"

    mk_target \
        TARGET="@scrub" \
        DEPS="@clean" \
        mk_run_script scrub "*$MK_SCRUB_TARGETS"

    mk_add_phony_target "$result"

    mk_target \
        TARGET="@nuke" \
        mk_run_script nuke "*$MK_SCRUB_TARGETS" "*$MK_NUKE_TARGETS"

    mk_add_phony_target "$result"

    mk_target \
        TARGET="@install" \
        _mk_core_install '$(DESTDIR)'

    mk_target \
        TARGET="@.PHONY" \
        DEPS="$MK_PHONY_TARGETS"

    # Save configure check cache
    _mk_save_cache
}

_mk_core_write_subdir_rule()
{
    if [ "$MK_SUBDIR" != ":" -a "$MK_SUBDIR" != "" ]
    then
        _targets=""
        mk_unquote_list "$SUBDIRS"
        for __subdir in "$@"
        do
            if [ "$__subdir" != "." ]
            then
                mk_quote "@${MK_SUBDIR#/}/$__subdir"
                _targets="$_targets $result"
            fi
        done
        mk_comment "staging targets in ${MK_SUBDIR#/}"

        mk_target \
            TARGET="@${MK_SUBDIR#/}" \
            DEPS="$MK_SUBDIR_TARGETS $_targets"

        mk_add_phony_target "$result"
    fi

    unset MK_SUBDIR_TARGETS
}

### section build

_mk_core_install()
{
    DESTDIR="${1%/}"

    mk_msg_domain "install"
   
    [ -d "${MK_STAGE_DIR}" ] || return 0

    find "${MK_STAGE_DIR}" -type f -o -type l |
    while read -r _file
    do
        _file="${_file#$MK_STAGE_DIR}"
        mk_msg "$_file"
        mk_mkdir "${DESTDIR}${_file%/*}"
        mk_run_or_fail cp -pPf "${MK_STAGE_DIR}${_file}" "${DESTDIR}${_file}"
    done

    return 0
}

_mk_core_symlink()
{
    # $1 = target
    # $2 = link
    mk_msg_domain "symlink"

    mk_msg "${2#$MK_STAGE_DIR} -> $1"
    mk_mkdir "${2%/*}"
    mk_run_or_fail ln -sf "$1" "$2"
}

_mk_core_stage()
{
    # $1 = dest
    # $2 = source
    # ($3) = mode
    mk_msg_domain "stage"

    mk_msg "${1#$MK_STAGE_DIR}"
    mk_mkdir "${1%/*}"
    mk_run_or_fail cp -r "$2" "$1"

    if [ -n "$3" ]
    then
        mk_run_or_fail chmod "$3" "$1"
    fi
}
