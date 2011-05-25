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
DEPENDS="core platform path"

##
#
# compiler.sh -- support for building C source projects
#
##

#<
# @var MK_CC
# @brief C compiler
# @export
# @system
#
# Set to the executable and arguments used to invoke the C compiler.
# Note that this variable is system-dependent and may have separate
# values for each target system and ISA.
#>

#<
# @var MK_CXX
# @brief C++ compiler
# @export
# @system
#
# Set to the executable and arguments used to invoke the C++ compiler.
# Note that this variable is system-dependent and may have separate
# values for each target system and ISA.
#>

#<
# @var MK_CPPFLAGS
# @brief Global C preprocessor flags
# @export
# @inherit
#
# The flags in this variable will be passed to any compiler operation
# involving the C or C++ preprocessor.
#>

#<
# @var MK_CFLAGS
# @brief Global C compiler flags
# @export
# @inherit
#
# The flags in this variable will be passed to any compiler operation
# involving C source code.
#>

#<
# @var MK_CXXFLAGS
# @brief Global C++ compiler flags
# @export
# @inherit
#
# The flags in this variable will be passed to any compiler operation
# involving C++ source code.
#>

#<
# @var MK_LDFLAGS
# @brief Global linker compiler flags
# @export
# @inherit
#
# The flags in this variable will be passed to any compiler operation
# involving linking an executable, library, or dynamically-loadable object.
#>

#<
# @var MK_ISA_CPPFLAGS
# @brief Per-ISA C preprocessor flags
# @export
# @inherit
# @system
#
# The flags in this variable will be passed to any compiler operation
# involving the C or C++ preprocessor.  Because it is a system-dependent
# variable, it may be used to set flags that apply to only one particular
# target system and ISA.
#>

#<
# @var MK_ISA_CFLAGS
# @brief Per-ISA C compiler flags
# @export
# @inherit
# @system
#
# The flags in this variable will be passed to any compiler operation
# involving C source code.  Because it is a system-dependent
# variable, it may be used to set flags that apply to only one particular
# target system and ISA.
#>

#<
# @var MK_ISA_CXXFLAGS
# @brief Per-ISA C++ compiler flags
# @export
# @inherit
# @system
#
# The flags in this variable will be passed to any compiler operation
# involving C++ source code.  Because it is a system-dependent
# variable, it may be used to set flags that apply to only one particular
# target system and ISA.
#>

#<
# @var MK_ISA_LDFLAGS
# @brief Per-ISA linker flags
# @export
# @inherit
# @system
#
# The flags in this variable will be passed to any compiler operation
# involving linking an executable, library, or dynamically-loadable object.
# Because it is a system-dependent variable, it may be used to set flags
# that apply to only one particular target system and ISA.
#>

#<
# @var MK_CC_STYLE
# @brief Style of C compiler
# @export
# @system
# @value gcc The C compiler is gcc or gcc-compatible
# @value unknown The C compiler style is unknown
#
# Defines the style of the C compiler.  This does not indicate the precise
# vendor of the compiler but is an abstract classification of its supported
# parameters, extensions, etc.  For example, <lit>clang</lit> is highly
# compatible with <lit>gcc</lit> and would be classified as the same style.
#>

#<
# @var MK_CXX_STYLE
# @brief Style of C++ compiler
# @export
# @system
# @value gcc The C++ compiler is g++ or g++-compatible
# @value unknown The C++ compiler style is unknown
#
# Like <varref>MK_CC_STYLE</varref>, but for the C++ compiler.
#>

#<
# @var MK_CC_LD_STYLE
# @brief Linker style for C
# @export
# @system
# @value gnu GNU ld or compatible
# @value native Native OS linker
#
# Defines the style of the linker used for linking objects
# derived from C code.  This is an abstract classification which
# encompasses the behavior and supported parameters of the linker.
#>

#<
# @var MK_CXX_LD_STYLE
# @brief Linker style for C++
# @export
# @system
# @value gnu GNU ld or compatible
# @value native Native OS linker
#
# Like <varref>MK_CC_LD_STYLE</varref>, but for C++-derived objects.
#>

#<
# @var MK_HEADERDEPS
# @brief Common header dependencies
# @inherit
#
# All headers listed in this variable will be implicitly added to the
# <lit>HEADERDEPS</lit> list of compiler functions such as
# <funcref>mk_program</funcref>.  It does not affect configure tests.
#
# This helps avoid repetition of consistently-used header dependencies
# across a larger project.  Care should be taken to avoid abusing it,
# however, as bloated dependency lists needlessly slow down
# <lit>make</lit>.
#>

#<
# @var MK_LIBDEPS
# @brief Common library dependencies
# @inherit
#
# All libraries listed in this variable will be implicitly added to the
# <lit>LIBDEPS</lit> list of compiler functions such as
# <funcref>mk_program</funcref>.  It does not affect configure tests.
#
# This can help avoid repetition when building multiple libraries, programs,
# etc. that share common library dependencies.  Take special care to
# avoid abusing it as linking extraneous libraries slows down the linker
# and incurs startup time and memory overhead at runtime.
#>

### section configure

#
# Utility functions
#
mk_resolve_header()
{
    case "$1" in
        /*)
            result="@${MK_STAGE_DIR}$1"
            ;;
        *)
            result="@${MK_STAGE_DIR}${MK_INCLUDEDIR}/$1"
            ;;
    esac
}

#
# Helper functions for make() stage
#

_mk_is_fat()
{
    [ "$MK_SYSTEM" = "host" -a "$MK_HOST_MULTIARCH" = "combine" ]
}

_mk_do_fat()
{
    # $1 = TARGET prefix
    # $2 = TARGET suffix
    # ... = command
    # sets result to quoted list of results

    _fat_parts=""
    _fat_prefix="$1"
    _fat_suffix="$2"
    shift 2

    for _isa in ${MK_HOST_ISAS}
    do
        SYSTEM="host/$_isa"
        CANONICAL_SYSTEM="$SYSTEM"
        TARGET="$_fat_prefix.host.$_isa$_fat_suffix"
        "$@"
        mk_quote "$result"
        _fat_parts="$_fat_parts $result"    
    done

    result="${_fat_parts# }"
    unset _fat_parts _fat_prefix _fat_suffix
}

_mk_process_headerdeps()
{
    for _header in ${MK_HEADERDEPS} ${HEADERDEPS}
    do
        if _mk_contains "$_header" ${MK_INTERNAL_HEADERS}
        then
            mk_resolve_header "$_header"
            mk_quote "$result"
            DEPS="$DEPS $result"
        else
            _mk_define_name "HAVE_${_header}"
            mk_get "$result"
            
            if [ "$result" = "no" ]
            then
                mk_fail "$1 depends on missing header $_header"
            elif [ -z "$result" ]
            then
                mk_warn "$1 depends on unchecked header $_header"
            fi
        fi
    done
}

_mk_compile()
{
    _object="${SOURCE%.*}${OSUFFIX}.${CANONICAL_SYSTEM%/*}.${CANONICAL_SYSTEM#*/}.o"
    
    mk_resolve_target "${SOURCE}"
    _res="$result"
    mk_quote "$_res"
    
    mk_target \
        TARGET="$_object" \
        DEPS="$DEPS $_INT_DEPS $result" \
        SYSTEM="$SYSTEM" \
        mk_run_script compile %INCLUDEDIRS %CPPFLAGS %CFLAGS %CXXFLAGS %COMPILER %PIC '$@' "$_res"
}

_mk_compile_detect()
{
    # Invokes _mk_compile after autodetecting COMPILER
    case "${SOURCE##*.}" in
        c|s)
            COMPILER="c"
            ;;
        [cC][pP]|[cC][pP][pP]|[cC][xX][xX]|[cC][cC]|C)
            COMPILER="c++"
            ;;
        *)
            mk_fail "unsupport source file type: .${SOURCE##*.}"
            ;;
    esac
    
    _mk_compile
}


mk_compile()
{
    mk_push_vars SOURCE HEADERDEPS DEPS INCLUDEDIRS CPPFLAGS CFLAGS CXXFLAGS PIC OSUFFIX COMPILER SYSTEM="$MK_SYSTEM" CANONICAL_SYSTEM
    mk_parse_params
    
    mk_canonical_system "$SYSTEM"
    CANONICAL_SYSTEM="$result"

    _mk_process_headerdeps "$SOURCE"
    _mk_compile_detect
    
    mk_pop_vars
}

_mk_verify_libdeps()
{
    for __dep in ${2}
    do
        _mk_contains "$__dep" ${MK_INTERNAL_LIBS} && continue
        _mk_define_name "HAVE_LIB_${__dep}"
        
        mk_get "$result"

        if [ "$result" = "no" ]
        then
            mk_fail "$1 depends on missing library $__dep ($MK_SYSTEM)"
        elif [ -z "$result" ]
        then
            mk_warn "$1 depends on unchecked library $__dep ($MK_SYSTEM)"
        fi
    done
}

_mk_process_symfile_gnu_ld()
{
    mk_resolve_file "$SYMFILE"
    __input="$result"
    mk_resolve_file "$SYMFILE.ver"
    __output="$result"

    {
        echo "{ global:"
        awk '{ print $0, ";"; }' < "$__input"
        echo "local: *; };"
    } > "$__output.new"

    if ! [ -f "$__output" ] || ! diff -q "$__output" "$__output.new" >/dev/null 2>&1
    then
        mv -f "$__output.new" "$__output"
    else
        rm -f "$__output.new"
    fi

    mk_add_configure_input "@$__input"
    mk_add_configure_output "@$__output"

    LDFLAGS="$LDFLAGS -Wl,-version-script,$__output"
    DEPS="$DEPS @$__output"
}

_mk_process_symfile_aix()
{
    mk_resolve_file "$SYMFILE"

    LDFLAGS="$LDFLAGS -Wl,-bexport:$result"
    DEPS="$DEPS @$result"
}

_mk_process_symfile()
{
    case "$MK_OS:$MK_CC_LD_STYLE" in
        *:gnu)
            _mk_process_symfile_gnu_ld "$@"
            ;;
        aix:native)
            _mk_process_symfile_aix
            ;;
        *)
            ;;
    esac   
}

_mk_library_form_name()
{
    # $1 = name
    # $2 = version
    # $3 = ext
    case "$MK_OS" in
        darwin)
            result="lib${1}${2:+.$2}${3}"
            ;;
        *)
            result="lib${1}${3}${2:+.$2}"
            ;;
    esac
}

_mk_library_process_version()
{
    if [ "$VERSION" != "no" ]
    then
        case "$VERSION" in
            *:*)
                _rest="${VERSION}:"
                _cur="${_rest%%:*}"
                _rest="${_rest#*:}"
                _rev="${_rest%%:*}"
                _rest="${_rest#*:}"
                _age="${_rest%:}"
                case "$MK_OS" in
                    hpux)
                        MAJOR="$_cur"
                        MINOR="$_rev"
                        MICRO=""
                        ;;
                    freebsd)
                        MAJOR="$_cur"
                        MINOR=""
                        MICRO=""
                        ;;
                    darwin)
                        MAJOR="$(($_cur - $_age))"
                        MINOR=""
                        MICRO=""
                        ;;
                    *)
                        MAJOR="$(($_cur - $_age))"
                        MINOR="$(($_age))"
                        MICRO="$_rev"
                        ;;
                esac
                ;;
            *)
                _rest="${VERSION}."
                MAJOR="${_rest%%.*}"
                _rest="${_rest#*.}"
                MINOR="${_rest%%.*}"
                _rest="${_rest#*.}"
                MICRO="${_rest%.}"
                case "$MK_OS" in
                    freebsd|darwin)
                        MINOR=""
                        MICRO=""
                        ;;
                    hpux)
                        MICRO=""
                        ;;
                esac
                ;;
        esac
    fi

    _mk_library_form_name "$LIB" "" "$EXT"
    SONAME="$result"
    LINKS="$result"

    if [ -n "$MAJOR" ]
    then
        _mk_library_form_name "$LIB" "$MAJOR" "$EXT"
        SONAME="$result"
        mk_quote "$SONAME"
        LINKS="$result $LINKS"
    fi
    
    if [ -n "$MINOR" ]
    then
        _mk_library_form_name "$LIB" "$MAJOR.$MINOR${MICRO:+.$MICRO}" "$EXT"
        mk_quote "$result"
        LINKS="$result $LINKS"
    fi
}

_mk_library()
{
    unset _deps _objects

    mk_comment "library ${LIB} ($SYSTEM) from ${MK_SUBDIR#/}"

    # Create object prefix based on library name
    _mk_slashless_name "$LIB"
    OSUFFIX=".$result"

    # Perform pathname expansion on SOURCES
    mk_expand_pathnames "${SOURCES}" "${MK_SOURCE_DIR}${MK_SUBDIR}"
    
    mk_unquote_list "$result"
    for SOURCE
    do
        _mk_compile_detect
        mk_quote "$result"
        _deps="$_deps $result"
        _objects="$_objects $result"
        [ "$COMPILER" = "c++" ] && IS_CXX=true
    done
    
    mk_unquote_list "${GROUPS}"
    for _group in "$@"
    do
        mk_quote "$_group.${CANONICAL_SYSTEM%/*}.${CANONICAL_SYSTEM#*/}.og"
        _deps="$_deps $result"
    done
    
    for result in ${LIBDEPS} ${MK_LIBDEPS}
    do
        if _mk_contains "$result" ${MK_INTERNAL_LIBS}
        then
            mk_quote "$MK_LIBDIR/lib${result}.la"
            _deps="$_deps $result"
        fi
    done
    
    ${IS_CXX} && COMPILER="c++"

    mk_target \
        SYSTEM="$SYSTEM" \
        TARGET="$TARGET" \
        DEPS="${_deps}" \
        mk_run_script link \
        MODE=library \
        %GROUPS LIBDEPS="$LIBDEPS $MK_LIBDEPS" %LIBDIRS %LDFLAGS %SONAME %EXT %COMPILER \
        '$@' "*${OBJECTS} ${_objects}"
}

#<
# @brief Build a library
# @usage LIB=name options...
# @option VERSION=major.minor.micro Sets the version
# information on the created library.  ABI compatibility
# should be maintained within a given major version,
# backwards compatibility should be maintained between
# minor versions, and micro versions should involve
# only a change in implementation, not interface.  Defaults
# to 0.0.0
# @option SYMFILE=file Specifies a file which contains a list
# of symbol names, one per line, which should be exported by
# the library.  If this is option is not used, it defaults
# to the behavior of the compiler and linker, which typically
# export all non-static symbols.  This option will be silently
# ignored on platforms where it is not supported.
# 
# Defines a target to build a C/C++ shared library.
# See <topicref ref="compiler"/> for a list of common
# options.
#
# A libtool-compatible .la file will also be generated.
# This is actually the canonical representation of a library
# in MakeKit and is the target which is placed in <var>result</var>.
#>
mk_library()
{
    mk_push_vars \
        INSTALLDIR="$MK_LIBDIR" LIB SOURCES SOURCE GROUPS CPPFLAGS CFLAGS CXXFLAGS LDFLAGS LIBDEPS \
        HEADERDEPS LIBDIRS INCLUDEDIRS VERSION=0:0:0 DEPS OBJECTS \
        SYMFILE SONAME LINKS COMPILER=c IS_CXX=false EXT="${MK_LIB_EXT}" PIC=yes \
        SYSTEM="$MK_SYSTEM" CANONICAL_SYSTEM TARGET
    mk_parse_params

    mk_canonical_system "$SYSTEM"
    CANONICAL_SYSTEM="$result"

    [ "$COMPILER" = "c++" ] && IS_CXX=true
    
    _mk_verify_libdeps "lib$LIB${EXT}" "$LIBDEPS $MK_LIBDEPS"
    _mk_process_headerdeps "lib$LIB${EXT}"

    if [ -n "$SYMFILE" ]
    then
        _mk_process_symfile
    fi

    _mk_library_process_version

    if _mk_is_fat
    then
        _mk_do_fat "lib$LIB" "$EXT" _mk_library "$@"
        _PARTS="$result"

        mk_unquote_list "$LINKS"
        TARGET="${INSTALLDIR:+$INSTALLDIR/}$1"

        mk_comment "library ${LIB} (host) from ${MK_SUBDIR#/}"

        mk_resolve_target "$TARGET"

        mk_target \
            TARGET="$result" \
            DEPS="$_PARTS" \
            _mk_compiler_multiarch_combine "$result" "*$_PARTS"
    else
        mk_unquote_list "$LINKS"
        TARGET="${INSTALLDIR:+$INSTALLDIR/}$1"

        mk_canonical_system "$SYSTEM"
        CANONICAL_SYSTEM="$result"

        _mk_library "$@"
    fi

    mk_unquote_list "$LINKS"
    _lib="$1"
    _links=""
    shift
    
    for _link
    do
        mk_symlink \
            TARGET="$_lib" \
            LINK="${INSTALLDIR}/$_link"
        mk_quote "$result"
        _links="$_links $result"
    done
    
    mk_quote "$INSTALLDIR/$_lib"

    mk_target \
        TARGET="${INSTALLDIR}/lib${LIB}.la" \
        DEPS="$_links $result" \
        mk_run_script link MODE=la \
        LIBDEPS="$LIBDEPS $MK_LIBDEPS" %LIBDIRS %GROUPS %COMPILER %LINKS %SONAME %EXT \
        '$@'

    MK_INTERNAL_LIBS="$MK_INTERNAL_LIBS $LIB"
    
    mk_pop_vars
}

_mk_dlo()
{
    unset _deps _objects
    
    mk_comment "dlo ${DLO} ($MK_SYSTEM) from ${MK_SUBDIR#/}"
    
    # Create object prefix based on dlo name
    _mk_slashless_name "$DLO"
    OSUFFIX=".$result"

    # Perform pathname expansion on SOURCES
    mk_expand_pathnames "${SOURCES}"
    mk_unquote_list "$result"
    for SOURCE
    do
        _mk_compile_detect
        
        mk_quote "$result"
        _deps="$_deps $result"
        _objects="$_objects $result"
        [ "$COMPILER" = "c++" ] && IS_CXX=true
    done
    
    mk_unquote_list "${GROUPS}"
    for _group in "$@"
    do
        mk_quote "$_group.${CANONICAL_SYSTEM%/*}.${CANONICAL_SYSTEM#*/}.og"
        _deps="$_deps $result"
    done
    
    for _lib in ${LIBDEPS} ${MK_LIBDEPS}
    do
        if _mk_contains "$_lib" ${MK_INTERNAL_LIBS}
        then
            mk_quote "${MK_LIBDIR}/lib${_lib}.la"
            _deps="$_deps $result"
        fi
    done
    
    ${IS_CXX} && COMPILER="c++"

    mk_target \
        SYSTEM="$SYSTEM" \
        TARGET="$TARGET" \
        DEPS="$_deps" \
        mk_run_script link \
        MODE=dlo \
        %GROUPS LIBDEPS="$LIBDEPS $MK_LIBDEPS" %LIBDIRS %LDFLAGS %EXT %COMPILER \
        '$@' "*${OBJECTS} ${_objects}"
}

#<
# @brief Build a dynamically loadable object
# @usage DLO=name options...
# @option SYMFILE=file Specifies an exported symbol file
# in the same manner as <funcref>mk_library</funcref>.
# 
# Defines a target to build a C/C++ dynamically loadable object --
# that is, an object suitable for loading with dlopen() or similar
# functions at runtime.  On some systems, Darwin in particular, this
# is not the same thing as a shared library.  See
# <topicref ref="compiler"/> for a list of common options.
#
# A libtool-compatible .la file will also be generated and is
# the target which is placed in <var>result</var>.
#>
mk_dlo()
{
    mk_push_vars \
        DLO SOURCES SOURCE GROUPS CPPFLAGS CFLAGS CXXFLAGS \
        LDFLAGS LIBDEPS HEADERDEPS LIBDIRS INCLUDEDIRS VERSION \
        OBJECTS DEPS INSTALLDIR EXT="${MK_DLO_EXT}" SYMFILE COMPILER=c \
        IS_CXX=false OSUFFIX PIC=yes SYSTEM="$MK_SYSTEM" CANONICAL_SYSTEM
    mk_parse_params

    [ -z "$INSTALLDIR" ] && INSTALLDIR="${MK_LIBDIR}"

    mk_canonical_system "$SYSTEM"
    CANONICAL_SYSTEM="$result"

    [ "$COMPILER" = "c++" ] && IS_CXX=true
    
    _mk_verify_libdeps "$DLO${EXT}" "$LIBDEPS $MK_LIBDEPS"
    _mk_process_headerdeps "$DLO${EXT}"

    if [ -n "$SYMFILE" ]
    then
        _mk_process_symfile
    fi

    if _mk_is_fat
    then
        _mk_do_fat "$DLO" "$EXT" _mk_dlo "$@"
        _PARTS="$result"

        mk_comment "library ${LIB} (host) from ${MK_SUBDIR#/}"

        TARGET="${INSTALLDIR:+$INSTALLDIR/}${DLO}${EXT}"
        mk_resolve_target "$TARGET"

        mk_target \
            TARGET="$result" \
            DEPS="$_PARTS" \
            _mk_compiler_multiarch_combine "$result" "*$_PARTS"
    else
        TARGET="${INSTALLDIR:+$INSTALLDIR/}${DLO}${EXT}"

        mk_canonical_system "$SYSTEM"
        CANONICAL_SYSTEM="$result"

        _mk_dlo "$@"
    fi

    mk_quote "$result"

    mk_target \
        TARGET="${INSTALLDIR}/${DLO}.la" \
        DEPS="$result" \
        mk_run_script link MODE=la \
        LIBDEPS="$LIBDEPS $MK_LIBDEPS" %LIBDIRS %GROUPS %COMPILER %EXT \
        '$@'

    mk_pop_vars
}

_mk_group()
{
    unset _deps _objects
    
    mk_comment "group ${GROUP} ($SYSTEM) from ${MK_SUBDIR#/}"

    # Create object prefix based on group name
    _mk_slashless_name "$GROUP"
    OSUFFIX=".$result"

    # Perform pathname expansion on SOURCES
    mk_expand_pathnames "${SOURCES}" "${MK_SOURCE_DIR}${MK_SUBDIR}"
    
    mk_unquote_list "$result"
    for SOURCE in "$@"
    do
        _mk_compile_detect     
        mk_quote "$result"
        _deps="$_deps $result"
        _objects="$_objects $result"
        [ "$COMPILER" = "c++" ] && IS_CXX=true
    done
    
    mk_unquote_list "${GROUPDEPS}"
    for _group in "$@"
    do
        mk_quote "$_group.${CANONICAL_SYSTEM%/*}.${CANONICAL_SYSTEM#*/}.og"
        _deps="$_deps $result"
    done
    
    for _lib in ${LIBDEPS} ${MK_LIBDEPS}
    do
        if _mk_contains "$_lib" ${MK_INTERNAL_LIBS}
        then
            mk_quote "${MK_LIBDIR}/lib${_lib}.la"
            _deps="$_deps $result"
        fi
    done
    
    ${IS_CXX} && COMPILER="c++"

    mk_target \
        SYSTEM="$SYSTEM" \
        TARGET="$TARGET" \
        DEPS="$_deps" \
        mk_run_script group %GROUPDEPS LIBDEPS="$LIBDEPS $MK_LIBDEPS" \
        %LIBDIRS %LDFLAGS %COMPILER '$@' "*${OBJECTS} ${_objects}"    
}

#<
# @brief Build an object group
# @usage GROUP=name options...
#
# Defines a target to build a C/C++ "object group", which combines sources
# files and other object groups into a logical unit which can be referenced
# from <funcref>mk_program</funcref> and friends.  This feature is
# similar to "convenience libraries" with GNU libtool.
#
# See <topicref ref="compiler"/> for common options or <topicref
# ref="c-projects-object-groups"/> in the MakeKit guide for usage examples.
#>
mk_group()
{
    mk_push_vars \
        GROUP SOURCES SOURCE CPPFLAGS CFLAGS CXXFLAGS LDFLAGS LIBDEPS \
        HEADERDEPS GROUPDEPS LIBDIRS INCLUDEDIRS OBJECTS DEPS \
        COMPILER=c IS_CXX=false PIC=yes SYSTEM="$MK_SYSTEM" CANONICAL_SYSTEM
    mk_parse_params

    _mk_verify_libdeps "$GROUP" "$LIBDEPS $MK_LIBDEPS"
    _mk_process_headerdeps "$GROUP"

    [ "$COMPILER" = "c++" ] && IS_CXX=true

    if _mk_is_fat
    then
        _mk_do_fat "$GROUP" ".og" _mk_group "$@"

        mk_target \
            TARGET="$GROUP.host.og" \
            DEPS="$result" \
            mk_run_or_fail touch '$@'
    else
        mk_canonical_system "$SYSTEM"
        CANONICAL_SYSTEM="$result"
        TARGET="$GROUP.${CANONICAL_SYSTEM%/*}.${CANONICAL_SYSTEM#*/}.og"

        _mk_group "$@"
    fi

    mk_pop_vars
}

_mk_program()
{
    unset _deps _objects
    
    if [ "${CANONICAL_SYSTEM%/*}" = "build" ]
    then
        _libdir="@${MK_RUNMK_LIBDIR}"
    else
        _libdir="$MK_LIBDIR"
    fi
    
    mk_comment "program ${PROGRAM} ($SYSTEM) from ${MK_SUBDIR#/}"

    # Create object prefix based on program name
    _mk_slashless_name "$PROGRAM"
    OSUFFIX=".$result"
    
    # Perform pathname expansion on SOURCES
    mk_expand_pathnames "${SOURCES}" "${MK_SOURCE_DIR}${MK_SUBDIR}"
    
    mk_unquote_list "$result"
    for SOURCE
    do
        _mk_compile_detect
        mk_quote "$result"
        _deps="$_deps $result"
        _objects="$_objects $result"
        [ "$COMPILER" = "c++" ] && IS_CXX=true
    done
    
    mk_unquote_list "${GROUPS}"
    for _group in "$@"
    do
        mk_quote "$_group.${CANONICAL_SYSTEM%/*}.${CANONICAL_SYSTEM#*/}.og"
        _deps="$_deps $result"
    done
    
    for _lib in ${LIBDEPS} ${MK_LIBDEPS}
    do
        if _mk_contains "$_lib" ${MK_INTERNAL_LIBS}
        then
            mk_quote "${_libdir}/lib${_lib}.la"
            _deps="$_deps $result"
        fi
    done

    ${IS_CXX} && COMPILER="c++"
    
    mk_target \
        SYSTEM="$SYSTEM" \
        TARGET="$TARGET" \
        DEPS="$_deps" \
        mk_run_script link MODE=program %GROUPS LIBDEPS="$LIBDEPS $MK_LIBDEPS" \
        %LDFLAGS %COMPILER '$@' "*${OBJECTS} ${_objects}"
}

#<
# @brief Build a program
# @usage PROGRAM=name options...
# 
# Defines a target to build a C/C++ executable program.
# See <topicref ref="compiler"/> for a list of common
# options.
#>
mk_program()
{
    mk_push_vars \
        PROGRAM SOURCES SOURCE OBJECTS GROUPS CPPFLAGS CFLAGS CXXFLAGS \
        LDFLAGS LIBDEPS HEADERDEPS DEPS LIBDIRS INCLUDEDIRS INSTALLDIR \
        COMPILER=c IS_CXX=false PIC=yes OSUFFIX SYSTEM="$MK_SYSTEM" \
        CANONICAL_SYSTEM EXT=""
    mk_parse_params

    if [ -z "$INSTALLDIR" ]
    then
        # Default to installing programs in bin dir
        if [ "${MK_CANONICAL_SYSTEM%/*}" = "build" ]
        then
            INSTALLDIR="@${MK_RUN_BINDIR}"
        else
            INSTALLDIR="$MK_BINDIR"
        fi
    fi

    [ "$COMPILER" = "c++" ] && IS_CXX=true

    _mk_verify_libdeps "$PROGRAM" "$LIBDEPS $MK_LIBDEPS"
    _mk_process_headerdeps "$PROGRAM"

    if _mk_is_fat
    then
        _mk_do_fat "$PROGRAM" "$EXT" _mk_program "$@"
        _deps="$result"

        mk_resolve_target "$INSTALLDIR/$PROGRAM$EXT"

        mk_target \
            TARGET="$result" \
            DEPS="$_deps" \
            _mk_compiler_multiarch_combine "$result" "*$_deps"
    else
        mk_canonical_system "$SYSTEM"
        CANONICAL_SYSTEM="$result"
        TARGET="$INSTALLDIR/$PROGRAM$EXT"

        _mk_program "$@"
    fi
    
    if [ "${MK_CANONICAL_SYSTEM%/*}" = "build" ]
    then
        MK_INTERNAL_PROGRAMS="$MK_INTERNAL_PROGRAMS $PROGRAM"
    fi

    mk_pop_vars
}

#<
# @brief Install headers
# @usage headers...
# @usage MASTER=master headers...
# @option INSTALLDIR=dir Specifies the location to install
# the headers.  By default, this is <var>MK_INCLUDEDIR</var>.
# @option ... See <topicref ref="compiler"/> for common options.
#
# Installs each header in <param>headers</param> into the system
# header directory.  If <param>master</param> is specified, it is
# also installed and marked as depending on all the other headers
# in the list.  This is useful when using HEADERDEPS elsewhere in
# the project, as depending on <param>master</param> will depend
# on all of the listed headers.
#>
mk_headers()
{
    mk_push_vars HEADERS MASTER INSTALLDIR HEADERDEPS DEPS
    INSTALLDIR="${MK_INCLUDEDIR}"
    mk_parse_params
    
    _mk_process_headerdeps "header" "$HEADERDEPS $MK_HEADERDEPS"

    unset _all_headers
    
    mk_comment "headers from ${MK_SUBDIR#/}"
    
    for _header in ${HEADERDEPS} ${MK_HEADERDEPS}
    do
        if _mk_contains "$_header" ${MK_INTERNAL_HEADERS}
        then
            DEPS="$DEPS '${MK_INCLUDEDIR}/${_header}'"
        fi
    done
    
    mk_expand_pathnames "${HEADERS} $*"
    mk_unquote_list "$result"

    mk_stage \
        DESTDIR="$INSTALLDIR" \
        DEPS="$DEPS" \
        "$@"
    DEPS="$DEPS $result"

    for _header in "$@"
    do
        _rel="${INSTALLDIR#$MK_INCLUDEDIR/}"
        
        if [ "$_rel" != "$INSTALLDIR" ]
        then
            _rel="$_rel/$_header"
        else
            _rel="$_header"
        fi
        
        MK_INTERNAL_HEADERS="$MK_INTERNAL_HEADERS $_rel"
    done

    mk_expand_pathnames "${MASTER}"   
    mk_unquote_list "$result"

    mk_stage \
        DESTDIR="$INSTALLDIR" \
        DEPS="$DEPS" \
        "$@"

    for _header in "$@"
    do
        _rel="${INSTALLDIR#$MK_INCLUDEDIR/}"
        
        if [ "$_rel" != "$INSTALLDIR" ]
        then
            _rel="$_rel/$_header"
        else
            _rel="$_header"
        fi
        
        MK_INTERNAL_HEADERS="$MK_INTERNAL_HEADERS $_rel"
    done
    
    mk_pop_vars
}

mk_declare_internal_header()
{
    MK_INTERNAL_HEADERS="$MK_INTERNAL_HEADERS $1"
}

mk_declare_internal_library()
{
    MK_INTERNAL_LIBS="$MK_INTERNAL_LIBS $1"
}

#
# Helper functions for configure() stage
# 

#<
# @brief Define a macro in config header
# @usage def value
#
# Defines the C preprocessor macro <param>def</param> to
# <param>value</param> in the current config
# header created by <funcref>mk_config_header</funcref>.
#>
mk_define()
{
    mk_push_vars cond
    mk_parse_params
    
    if [ -n "$MK_CONFIG_HEADER" ]
    then
        _name="$1"
        
        _mk_define_name "$MK_SYSTEM"
        cond="_MK_$result"
        
        if [ "$#" -eq '2' ]
        then
            result="$2"
        else
            mk_get "$_name"
        fi
        
        mk_write_config_header "#if defined($cond) && !defined($_name)"
        mk_write_config_header "#define $_name $result"
        mk_write_config_header "#endif"
    fi
    
    mk_pop_vars
}

mk_define_always()
{
    if [ -n "$MK_CONFIG_HEADER" ]
    then
        _name="$1"
        
        if [ "$#" -eq '2' ]
        then
            result="$2"
        else
            mk_get "$_name"
        fi
        mk_write_config_header "#define $_name $result"
    fi
}

mk_write_config_header()
{
    echo "$*" >&5
}

_mk_close_config_header()
{
    if [ -n "${MK_LAST_CONFIG_HEADER}" ]
    then
        cat >&5 <<EOF

#endif
EOF
        exec 5>&-
        
        if [ -f "${MK_LAST_CONFIG_HEADER}" ] && diff "${MK_LAST_CONFIG_HEADER}" "${MK_LAST_CONFIG_HEADER}.new" >/dev/null 2>&1
        then
            # The config header has not changed, so don't touch the timestamp on the file */
            rm -f "${MK_LAST_CONFIG_HEADER}.new"
        else
            mv "${MK_LAST_CONFIG_HEADER}.new" "${MK_LAST_CONFIG_HEADER}"
        fi
        
        MK_LAST_CONFIG_HEADER=""
    fi
}

#<
# @brief Create config header
# @usage header
#
# Creates a config header named <param>header</param>.
# Any subsequent definitions made by <funcref>mk_define</funcref>
# or various configuration tests will be placed in the header
# most recently created with this function.
#>
mk_config_header()
{
    mk_push_vars HEADER
    mk_parse_params
    
    _mk_close_config_header
    
    [ -z "$HEADER" ] && HEADER="$1"
    
    MK_CONFIG_HEADER="${MK_OBJECT_DIR}${MK_SUBDIR}/${HEADER}"
    MK_LAST_CONFIG_HEADER="$MK_CONFIG_HEADER"
    
    mkdir -p "${MK_CONFIG_HEADER%/*}"
    
    mk_msg "config header ${MK_CONFIG_HEADER#${MK_OBJECT_DIR}/}"
    
    exec 5>"${MK_CONFIG_HEADER}.new"
    
    cat >&5 <<EOF
/* Generated by MakeKit */

#ifndef __MK_CONFIG_H__
#define __MK_CONFIG_H__

EOF
    
    mk_add_configure_output "@$MK_CONFIG_HEADER"
    
    mk_pop_vars
}

_mk_build_test()
{
    __test="${2%.*}"
    
    case "${1}" in
        compile|compile-keep)
            (
                eval "exec ${MK_LOG_FD}>&-"
                MK_LOG_FD=""
                mk_run_script compile \
                    COMPILER="$MK_CHECK_LANG" \
                    CONFTEST=yes \
                    CPPFLAGS="$CPPFLAGS" \
                    CFLAGS="$CFLAGS" \
                    "${__test}.o" "${__test}.c"
            ) >&${MK_LOG_FD} 2>&1            
            _ret="$?"
            if [ "${1}" != "compile-keep" ]
            then
                rm -f "${__test}.o"
            fi
            ;;
        link-program|run-program)
            (
                eval "exec ${MK_LOG_FD}>&-"
                MK_LOG_FD=""
                mk_run_script compile \
                    COMPILER="$MK_CHECK_LANG" \
                    CONFTEST=yes \
                    CPPFLAGS="$CPPFLAGS" \
                    CFLAGS="$CFLAGS" \
                    "${__test}.o" "${__test}.c"
                mk_run_script link \
                    COMPILER="$MK_CHECK_LANG" \
                    CONFTEST=yes \
                    MODE=program \
                    LIBDEPS="$LIBDEPS" \
                    LDFLAGS="$LDFLAGS" \
                    "${__test}" "${__test}.o"
            ) >&${MK_LOG_FD} 2>&1
            _ret="$?"
            if [ "$_ret" -eq 0 -a "$1" = "run-program" ]
            then
                ./"${__test}"
                _ret="$?"
            fi
            rm -f "${__test}"
            rm -f "${__test}.o"
            ;;
        *)
            mk_fail "Unsupported build type: ${1}"
            ;;
    esac

    if [ "$_ret" -ne 0 ]
    then
        {
            echo ""
            echo "Failed code:"
            echo ""
            cat "${__test}.c" | awk 'BEGIN { no = 1; } { printf("%3d  %s\n", no, $0); no++; }'
            echo ""
        } >&${MK_LOG_FD}
    fi

    rm -f "${__test}.c"

    return "$_ret"
}

_mk_c_check_prologue()
{
    if [ -n "$MK_CONFIG_HEADER" ]
    then
        cat "${MK_CONFIG_HEADER}.new"
        printf "#endif\n\n"
    fi
}

mk_try_compile()
{
    mk_push_vars CODE HEADERDEPS
    mk_parse_params
    
    {
        _mk_c_check_prologue
        for _header in ${HEADERDEPS}
        do
            mk_might_have_header "$_header" && echo "#include <${_header}>"
        done
        
        cat <<EOF
int main(int argc, char** argv)
{
${CODE}
}
EOF
    } > .check.c

    _mk_build_test compile ".check.c"
    _ret="$?"
    mk_safe_rm .check.c

    mk_pop_vars

    return "$_ret"
}

#<
# @brief Check for a header
# @usage HEADER=header
# @option HEADERDEPS=headers Specifies additional headers
# which might be needed in order for <param>header</param>
# to be compilable.  If a header has been determined to be
# unavailable by a previous <funcref>mk_check_headers</funcref>,
# it will be silently omitted from this list as a convenience.
#
# Checks for the availability of a system header and sets
# <var>result</var> to the result.  If the header was found
# on the system, the result will be "external".  If the header
# is provided by <funcref>mk_headers</funcref> within the current
# project, the result will be "internal".  Otherwise, the result
# will be "no".
#>
mk_check_header()
{
    mk_push_vars HEADER HEADERDEPS CPPFLAGS CFLAGS
    mk_parse_params

    [ -z "$HEADER" ] && HEADER="$1"

    CFLAGS="$CFLAGS -Wall -Werror"

    if _mk_contains "$HEADER" ${MK_INTERNAL_HEADERS}
    then
        result="internal"
    else
        {
            _mk_c_check_prologue
            for _header in ${HEADERDEPS}
            do
                mk_might_have_header "$_header" && echo "#include <${_header}>"
            done

            echo "#include <${HEADER}>"
            echo ""
            
            cat <<EOF
int main(int argc, char** argv)
{
    return 0;
}
EOF
        } > .check.c
        mk_log "running compile test for header: $HEADER"
        if _mk_build_test compile ".check.c"
        then
            result="external"
        else
            result="no"
        fi
    fi

    mk_pop_vars
    [ "$result" != "no" ]
}

#<
# @brief Check for headers
# @usage headers...
#
# For each header in <param>headers</param>, <funcref>mk_check_header</funcref>
# is invoked to check for its availability.  <var>HAVE_<varname>header</varname></var>
# is set to the result, and if the header was available, <def>HAVE_<varname>header</varname></def>
# is defined in the current config header.  A message is printed indicating the result
# of each test.
#>
mk_check_headers()
{
    mk_push_vars HEADERDEPS FAIL CPPFLAGS CFLAGS DEFNAME HEADER
    mk_parse_params
    
    for HEADER
    do
        _mk_define_name "$HEADER"
        DEFNAME="$result"

        mk_msg_checking "header $HEADER"

        if ! mk_check_cache "HAVE_$DEFNAME"
        then
            mk_check_header \
                HEADER="$HEADER" \
                HEADERDEPS="$HEADERDEPS" \
                CPPFLAGS="$CPPFLAGS" \
                CFLAGS="$CFLAGS"

            mk_cache "HAVE_$DEFNAME" "$result"
        fi

        mk_msg_result "$result"

        if [ "$result" != no ]
        then
            mk_define "HAVE_$DEFNAME" "1"
        elif [ "$FAIL" = yes ]
        then
            mk_fail "missing header: $HEADER"
        fi
        
    done

    mk_pop_vars
}

mk_have_header()
{
    _mk_define_name "HAVE_$1"
    mk_get "$result"
    [ "$result" = "external" -o "$result" = "internal" ]
}

mk_might_have_header()
{
    _mk_define_name "HAVE_$1"
    mk_get "$result"
    [ "$result" != "no" ]
}

#<
# @brief Check for a function
# @usage FUNCTION=func
#
# @option HEADERDEPS=headers Specifies any headers
# which might be needed in order for a prototype of
# the function to be available.  Unlike autoconf, MakeKit
# checks that both the function prototype and symbol are
# available, so specifying this option correctly is vital.
# @option LIBDEPS=deps Specifies any libraries which
# might be needed for the function to be available.
#
# Checks for the availability of a function, setting
# <var>result</var> to the result ("yes" or "no").
# If <param>func</param> is specified as a full function prototype,
# the test will only succeed if the function which was found
# had the same prototype.  If <param>func</param> is specified as
# a simple name, the test will succeed as long as the function
# has an available prototype and symbol.
#>
mk_check_function()
{
    mk_push_vars LIBDEPS FUNCTION HEADERDEPS CPPFLAGS LDFLAGS CFLAGS FAIL PROTOTYPE
    mk_parse_params

    CFLAGS="$CFLAGS -Wall -Werror"

    [ -z "$FUNCTION" ] && FUNCTION="$1"
    [ -z "$PROTOTYPE" ] && PROTOTYPE="$FUNCTION"
    
    case "$PROTOTYPE" in
        *'('*)
            _parts="`echo "$PROTOTYPE" | sed 's/^\(.*[^a-zA-Z_]\)\([a-zA-Z_][a-zA-Z0-9_]*\) *(\([^)]*\)).*$/\1|\2|\3/g'`"
            _ret="${_parts%%|*}"
            _parts="${_parts#*|}"
            FUNCTION="${_parts%%|*}"
            _args="${_parts#*|}"
            ;;
        *)
            FUNCTION="$PROTOTYPE"
            _args=""
            ;;
    esac
    
    {
        _mk_c_check_prologue
        for _header in ${HEADERDEPS}
        do
            mk_might_have_header "$_header" && echo "#include <${_header}>"
        done
        
        echo ""
        
        if [ -n "$_args" ]
        then
            cat <<EOF
int main(int argc, char** argv)
{
    $_ret (*__func)($_args) = &$FUNCTION;
    return (char*) __func < (char*) &main ? 0 : 1;
}
EOF
            else
                cat <<EOF
int main(int argc, char** argv)
{
    void* __func = &$FUNCTION;
    return (char*) __func < (char*) &main ? 0 : 1;
}
EOF
            fi
    } >.check.c
    
    mk_log "running link test for $PROTOTYPE"
    if _mk_build_test 'link-program' ".check.c"
    then
        result="yes"
    else
        result="no"
    fi

    mk_pop_vars
    [ "$result" != "no" ]
}

#<
# @brief Check for functions
# @usage funcs...
#
# For each function in <param>funcs</param>, <funcref>mk_check_function</funcref>
# is invoked to check for its availability.  <var>HAVE_<varname>header</varname></var>
# is set to the result, and if the function was available, <def>HAVE_<varname>func</varname></def>
# is defined in the current config header.  <def>HAVE_DECL_<varname>func</varname></def>
# is defined to 1 if the function was available and 0 otherwise.  A message is printed indicating
# the result of each test.
#>
mk_check_functions()
{
    mk_push_vars \
        LIBDEPS HEADERDEPS CPPFLAGS LDFLAGS CFLAGS FAIL \
        PROTOTYPE DEFNAME
    mk_parse_params
    
    for PROTOTYPE
    do
        _mk_define_name "$PROTOTYPE"
        DEFNAME="$result"

        mk_msg_checking "function $PROTOTYPE"

        if ! mk_check_cache "HAVE_$DEFNAME"
        then
            mk_check_function \
                PROTOTYPE="$PROTOTYPE" \
                HEADERDEPS="$HEADERDEPS" \
                CPPFLAGS="$CPPFLAGS" \
                LDFLAGS="$LDFLAGS" \
                CFLAGS="$CFLAGS" \
                LIBDEPS="$LIBDEPS"

            mk_cache "HAVE_$DEFNAME" "$result"
        fi

        mk_msg_result "$result"

        if [ "$result" = "yes" ]
        then
            mk_define "HAVE_$DEFNAME" 1
            mk_define "HAVE_DECL_$DEFNAME" 1
        elif [ "$FAIL" = "yes" ]
        then
            mk_fail "missing function: $PROTOTYPE"
        else
            mk_define "HAVE_DECL_$DEFNAME" 0
        fi
    done

    mk_pop_vars
}

#<
# @brief Check for a library
# @usage LIB=lib
#
# @option LIBDEPS=headers Specifies any additional
# libraries might be needed in order to link against
# <param>lib</param>.  Note that MakeKit will respect
# .la files when checking for linkability, so
# this is generally not necessary if the library in
# question was produced with MakeKit or libtool.
#
# Checks for the availability of a library, setting
# <var>result</var> to the result.  If the library
# was found on the system, the result will be "external".
# If it is produced by <funcref>mk_library</funcref> within
# the current project, the result will be "internal".
# Otherwise, the result will be "no".
#>
mk_check_library()
{
    mk_push_vars LIBDEPS LIB CPPFLAGS LDFLAGS CFLAGS
    mk_parse_params

    [ -z "$LIB" ] && LIB="$1"

    CFLAGS="$CFLAGS -Wall -Werror"
    LIBDEPS="$LIBDEPS $LIB"
    
    if _mk_contains "$LIB" ${MK_INTERNAL_LIBS}
    then
        result="internal"
    else
        {
            _mk_c_check_prologue
            cat <<EOF
int main(int argc, char** argv)
{
    return 0;
}
EOF
        } >.check.c
        mk_log "running link test for library: $LIB"
        if _mk_build_test 'link-program' ".check.c"
        then
            result="external"
        else
            result="no"
        fi
    fi

    mk_pop_vars
    [ "$result" != "no" ]
}

#<
# @brief Check for libraries
# @usage libs...
#
# For each library in <param>libs</param>, <funcref>mk_check_library</funcref>
# is invoked to check for its availability.  <var>HAVE_<varname>lib</varname></var>
# is set to the result.  If the library was available, <def>HAVE_LIB_<varname>lib</varname></def>
# is defined in the current config header and <var>LIB_<varname>lib</varname></var> is
# set to <param>lib</param> (this is useful for conditionally linking to the library
# with LIBDEPS= later on).  A message is printed indicating the result of each test.
#>
mk_check_libraries()
{
    mk_push_vars LIBS LIBDEPS CPPFLAGS LDFLAGS CFLAGS FAIL LIB DEFNAME
    mk_parse_params
    
    for LIB
    do
        _mk_define_name "$LIB"
        DEFNAME="$result"

        mk_declare -s -i "LIB_$DEFNAME"

        mk_msg_checking "library $LIB"

        if ! mk_check_cache "HAVE_LIB_$DEFNAME"
        then
            mk_check_library \
                LIB="$LIB" \
                CPPFLAGS="$CPPFLAGS" \
                LDFLAGS="$LDFLAGS" \
                CFLAGS="$CFLAGS" \
                LIBDEPS="$LIBDEPS"
            
            mk_cache "HAVE_LIB_$DEFNAME" "$result"
        fi

        mk_msg_result "$result"

        if [ "$result" != "no" ]
        then
            mk_set_all_isas "LIB_$DEFNAME" "$LIB"
            mk_define "HAVE_LIB_$DEFNAME" "1"
        elif [ "$FAIL" = "yes" ]
        then
            mk_fail "missing library: $LIB"
        fi
    done

    mk_pop_vars
}

_mk_check_type()
{
    {
        _mk_c_check_prologue
        for _header in ${HEADERDEPS}
        do
            mk_might_have_header "$_header" && echo "#include <${_header}>"
        done
        
        echo ""
        
        cat <<EOF
int main(int argc, char** argv)
{ 
    return (int) sizeof($TYPE);
}
EOF
    } > .check.c
    mk_log "running run test for sizeof($TYPE)"
    if _mk_build_test 'compile' .check.c
    then
        result="yes"
    else
        result="no"
    fi

    [ "$result" != "no" ]
}

#<
# @brief Check for a type
# @usage TYPE=type
# @option HEADERDEPS=headers Specifies any headers that
# are necessary to find a declaration of the type.
#
# Checks if the specified type is declared and sets <var>result</var>
# to the result ("yes" or "no").
#>
mk_check_type()
{
    mk_push_vars TYPE HEADERDEPS CPPFLAGS CFLAGS
    mk_parse_params

    [ -z "$TYPE" ] && TYPE="$1"

    CFLAGS="$CFLAGS -Wall -Werror"

    _mk_check_type

    mk_pop_vars
    [ "$result" != "no" ]
}

#<
# @brief Check for types
# @usage typess...
#
# For each type in <param>types</param>, <funcref>mk_check_type</funcref>
# is invoked to check for its availability.  <var>HAVE_<varname>type</varname></var>
# is set to the result.  If the type was available, <def>HAVE_<varname>type</varname></def>
# is defined in the current config header. A messsage is printed indicating the result of
# each test.
#>
mk_check_types()
{
    mk_push_vars TYPES HEADERDEPS CPPFLAGS CFLAGS TYPE FAIL DEFNAME
    mk_parse_params

    CFLAGS="$CFLAGS -Wall -Werror"

    for TYPE
    do
        _mk_define_name "$TYPE"
        DEFNAME="$result"

        mk_msg_checking "type $TYPE"

        if ! mk_check_cache "HAVE_$DEFNAME"
        then
            _mk_check_type
            
            mk_cache "HAVE_$DEFNAME" "$result"
        fi

        mk_msg_result "$result"

        if [ "$result" = "yes" ]
        then
            mk_define "HAVE_$DEFNAME" "1"
        elif [ "$FAIL" = "yes" ]
        then
            mk_fail "missing type: $TYPE"
        fi
    done

    mk_pop_vars
}

mk_check_static_predicate()
{
    mk_push_vars EXPR
    mk_parse_params

    {
        _mk_c_check_prologue
        for _header in ${HEADERDEPS}
        do
            mk_might_have_header "$_header" && echo "#include <${_header}>"
        done
        echo ""
        cat <<EOF
int main(int argc, char** argv)
{
     int __array[($EXPR) ? 1 : -1] = {0};

     return __array[0];
}
EOF
    } > .check.c

    _mk_build_test 'compile' .check.c
    _res="$?"

    mk_pop_vars

    return "$_res"
}

_mk_check_sizeof()
{
    # Make sure the type actually exists
    _mk_check_type || return 1

    # Algorithm to derive the size of a type even
    # when cross-compiling.  mk_check_static_predicate()
    # lets us evaluate a boolean expression involving
    # compile-time constants.  Using this, we can perform
    # a binary search for the correct size of the type.
    upper="1024"
    lower="0"
        
    while [ "$upper" -ne "$lower" ]
    do
        mid="$((($upper + $lower)/2))"
        if mk_check_static_predicate EXPR="sizeof($TYPE) <= $mid"
        then
            upper="$mid"
        else
            lower="$(($mid + 1))"
        fi
    done

    result="$upper"
    unset upper lower mid
}

#<
# @brief Check size of a type
#
# @usage TYPE=type
# @usage type
#
# Runs a test for the size of <param>type</param> and sets
# <var>result</var> to the result.  If the type cannot be
# found at all, configuration will be aborted.
#
# This test will work in cross-compiling configurations.
# It will not work on types with sizes over 1024 bytes.
#>
mk_check_sizeof()
{
    mk_push_vars TYPE HEADERDEPS CPPFLAGS LDFLAGS CFLAGS LIBDEPS
    mk_parse_params

    [ -z "$TYPE" ] && TYPE="$1"

    CFLAGS="$CFLAGS -Wall -Werror"
    HEADERDEPS="$HEADERDEPS stdio.h"

    _mk_check_sizeof

    mk_pop_vars
}

#<
# @brief Check sizes of several types
#
# @usage types...
#
# Runs <funcref>mk_check_sizeof</funcref> on each type in <param>types</param>.
# For each type, the variable <var>SIZEOF_<varname>type</varname></var> will be
# set to the result, and <def>SIZEOF_<varname>type</varname></def> will be
# similarly defined in the current config header.
#>
mk_check_sizeofs()
{
    mk_push_vars HEADERDEPS CPPFLAGS LDFLAGS CFLAGS LIBDEPS
    mk_parse_params

    CFLAGS="$CFLAGS -Wall -Werror"
    HEADERDEPS="$HEADERDEPS stdio.h"

    for TYPE
    do
        _mk_define_name "$TYPE"
        DEFNAME="$result"

        mk_msg_checking "sizeof $TYPE"

        if ! mk_check_cache "SIZEOF_$DEFNAME"
        then
            _mk_check_sizeof
            
            mk_cache "SIZEOF_$DEFNAME" "$result"
        fi

        mk_msg_result "$result"

	[ "$result" != "no" ] && mk_define "SIZEOF_$DEFNAME" "$result"
    done

    mk_pop_vars
}

#<
# @brief Check endianness of system
# @usage
#
# Checks the endianness of the current system and sets the variable
# <var>ENDIANNESS</var> to the result ("little" or "big").  If the
# result was "big", it also defines <def>WORDS_BIGENDIAN</def> in
# the current config header.
#
# This function will work in cross-compiling configurations.
#>
mk_check_endian()
{
    mk_push_vars CPPFLAGS LDFLAGS CFLAGS LIBDEPS
    mk_parse_params

    CFLAGS="$CFLAGS -Wall -Werror"
    HEADERDEPS="$HEADERDEPS stdio.h"

    mk_msg_checking "endianness"
    
    if mk_check_cache "ENDIANNESS"
    then
        result="$result"
    else
        # Check for endianness in a (hacky) manner that supports
        # cross-compiling. This is done by compiling a C file that
        # contains arrays of 16-bit integers that happen to form
        # ASCII strings under particular byte orders.  The strings
        # are then searched for in the resulting object file.
        #
        # The character sequences were designed to be extremely unlikely
        # to occur otherwise.
        {
            cat <<EOF
#include <stdio.h>

/* Spells "aArDvArKsOaP" on big-endian systems */
static const unsigned short aardvark[] =
{0x6141, 0x7244, 0x7641, 0x724b, 0x734f, 0x6150, 0x0};
/* Spells "zEbRaBrUsH" on little-endian systems */
static const unsigned short zebra[] = 
{0x457a, 0x5262, 0x4261, 0x5572, 0x4873, 0x0};

int main(int argc, char** argv)
{ 
    return (int) aardvark[argc] + zebra[argc];
}
EOF
        } > .check.c
        if _mk_build_test 'compile-keep' .check.c
        then
            if strings .check.o | grep "aArDvArKsOaP" >/dev/null
            then
                result="big"
            elif strings .check.o | grep "zEbRaBrUsH" >/dev/null
            then
                result="little"
            else
                rm -f .check.o
                mk_fail "could not determine endianness"
            fi
        else
            rm -f .check.o
            mk_fail "could not determine endianness"
        fi
        
        mk_cache "ENDIANNESS" "$result"
    fi

    if [ "$ENDIANNESS" = "big" ]
    then
        mk_define WORDS_BIGENDIAN 1
    fi
    
    mk_msg_result "$ENDIANNESS"
    
    mk_pop_vars
}

#<
# @brief Set language for configure checks
# @usage lang
#
# Sets the language used for subsequent configuration checks.
# Valid values are "c" and "c++".
#>
mk_check_lang()
{
    MK_CHECK_LANG="$1"
}

option()
{
    if [ "$MK_DEBUG" = yes ]
    then
        _default_OPTFLAGS="-O0 -g"
    else
        _default_OPTFLAGS="-O2 -g"
    fi

    mk_option \
        VAR="CC" \
        PARAM="program" \
        DEFAULT="gcc" \
        HELP="Default C compiler"

    MK_DEFAULT_CC="$CC"

    mk_option \
        VAR="CXX" \
        PARAM="program" \
        DEFAULT="g++" \
        HELP="Default C++ compiler"

    MK_DEFAULT_CXX="$CXX"

    mk_option \
        VAR="CPPFLAGS" \
        PARAM="flags" \
        DEFAULT="" \
        HELP="C preprocessor flags"

    MK_CPPFLAGS="$CPPFLAGS"

    mk_option \
        VAR="CFLAGS" \
        PARAM="flags" \
        DEFAULT="$_default_OPTFLAGS" \
        HELP="C compiler flags"

    MK_CFLAGS="$CFLAGS"

    mk_option \
        VAR="CXXFLAGS" \
        PARAM="flags" \
        DEFAULT="$_default_OPTFLAGS" \
        HELP="C++ compiler flags"

    MK_CXXFLAGS="$CXXFLAGS"

    mk_option \
        VAR="LDFLAGS" \
        PARAM="flags" \
        DEFAULT="" \
        HELP="Linker flags"

    MK_LDFLAGS="$LDFLAGS"

    unset CC CXX CPPFLAGS CFLAGS CXXFLAGS LDFLAGS

    for _sys in build host
    do
        _mk_define_name "MK_${_sys}_ISAS"
        mk_get "$result"
        
        for _isa in ${result}
        do
            _mk_define_name "$_sys/${_isa}"
            _def="$result"

            _mk_define_name "MK_${_sys}_OS"
            mk_get "$result"
            
            _default_cc="$MK_DEFAULT_CC"
            _default_cxx="$MK_DEFAULT_CXX"

            case "${MK_DEFAULT_CC}-${result}-${_isa}" in
                *-darwin-x86_32)
                    _default_cflags="-arch i386"
                    _default_cxxflags="-arch i386"
                    ;;
                *-darwin-x86_64)
                    _default_cflags="-arch x86_64"
                    _default_cxxflags="-arch x86_64"
                    ;;
                *-darwin-ppc32)
                    _default_cflags="-arch ppc"
                    _default_cxxflags="-arch ppc"
                    ;;
                *-darwin-ppc64)
                    _default_cflags="-arch ppc64"
                    _default_cxxflags="-arch ppc64"
                    ;;
                *-*-x86_32)
                    _default_cflags="-m32"
                    _default_cxxflags="-m32"
                    ;;
                *-*-x86_64)
                    _default_cflags="-m64"
                    _default_cxxflags="-m64"
                    ;;
                *-*-sparc_32)
                    _default_cflags="-m32"
                    _default_cxxflags="-m32"
                    ;;
                *-*-sparc_64)
                    _default_cflags="-m64"
                    _default_cxxflags="-m64"
                    ;;
                *-aix-ppc32)
                    _default_cflags="-maix32"
                    _default_cxxflags="-maix32"
                    ;;
                *-aix-ppc64)
                    _default_cflags="-maix64"
                    _default_cxxflags="-maix64"
                    ;;
                *-hpux-ia64_32)
                    _default_cflags="-milp32"
                    _default_cxxflags="-milp32"
                    ;;
                *-hpux-ia64_64)
                    _default_cflags="-mlp64"
                    _default_cxxflags="-mlp64"
                    ;;
                *)
                    _default_cflags=""
                    _default_cxxflags=""
                    ;;
            esac
            
            mk_option \
                VAR="${_def}_CC" \
                PARAM="program" \
                DEFAULT="$_default_cc" \
                HELP="C compiler ($_sys/$_isa)"

            mk_option \
                VAR="${_def}_CXX" \
                PARAM="program" \
                DEFAULT="$_default_cxx" \
                HELP="C++ compiler ($_sys/$_isa)"
            
            mk_option \
                VAR="${_def}_CPPFLAGS" \
                PARAM="flags" \
                DEFAULT="" \
                HELP="C preprocessor flags ($_sys/$_isa)"
            
            mk_option \
                VAR="${_def}_CFLAGS" \
                PARAM="flags" \
                DEFAULT="$_default_cflags" \
                HELP="C compiler flags ($_sys/$_isa)"

            mk_option \
                VAR="${_def}_CXXFLAGS" \
                PARAM="flags" \
                DEFAULT="$_default_cxxflags" \
                HELP="C++ compiler flags ($_sys/$_isa)"
            
            mk_option \
                VAR="${_def}_LDFLAGS" \
                PARAM="flags" \
                DEFAULT="" \
                HELP="Linker flags ($_sys/$_isa)"
        done
    done
}

_mk_cc_primitive()
{
    cat >.check.c || mk_fail "could not write .check.c"
    ${MK_CC} ${MK_CFLAGS} -o .check.o -c .check.c
    _res="$?"
    rm -f .check.o .check.c
    return "$_res"
}

_mk_cxx_primitive()
{
    cat >.check.cpp || mk_fail "could not write .check.cpp"
    ${MK_CXX} ${MK_CXXFLAGS} -o .check.o -c .check.cpp
    _res="$?"
    rm -f .check.o .check.cpp
    return "$_res"
}

_mk_check_cc_style()
{
    mk_msg_checking "C compiler style"
    if cat <<EOF | _mk_cc_primitive >/dev/null 2>&1
#ifndef __GNUC__
#error nope
#endif
EOF
    then
        result="gcc"
    else
        result="unknown"
    fi
    
    MK_CC_STYLE="$result"
    mk_msg_result "$MK_CC_STYLE"
}

_mk_check_cxx_style()
{
    mk_msg_checking "C++ compiler style"
    if cat <<EOF | _mk_cxx_primitive >/dev/null 2>&1
#ifndef __GNUC__
#error nope
#endif
EOF
    then
        result="gcc"
    else
        result="unknown"
    fi
    
    MK_CXX_STYLE="$result"
    mk_msg_result "$MK_CXX_STYLE"
}

_mk_check_cc_ld_style()
{
    mk_msg_checking "C compiler linker style"

    case "$MK_CC_STYLE" in
        gcc)
            _ld="`${MK_CC} -print-prog-name=ld`"
            case "`ld -v 2>&1`" in
                *"GNU"*)
                    result="gnu"
                    ;;
                *)
                    result="native"
                    ;;
            esac
            ;;
        *)
            result="native"
    esac

    MK_CC_LD_STYLE="$result"
    mk_msg_result "$MK_CC_LD_STYLE"
}

_mk_check_cxx_ld_style()
{
    mk_msg_checking "C++ compiler linker style"

    case "$MK_CXX_STYLE" in
        gcc)
            _ld="`${MK_CXX} -print-prog-name=ld`"
            case "`ld -v 2>&1`" in
                *"GNU"*)
                    result="gnu"
                    ;;
                *)
                    result="native"
                    ;;
            esac
            ;;
        *)
            result="native"
    esac

    MK_CXX_LD_STYLE="$result"
    mk_msg_result "$MK_CXX_LD_STYLE"
}

_mk_compiler_check()
{
    for _sys in build host
    do
        mk_system "$_sys"

        for _isa in ${MK_ISAS}
        do
            mk_system "$_sys/$_isa"

            _mk_check_cc_style
            _mk_check_cxx_style
            _mk_check_cc_ld_style
            _mk_check_cxx_ld_style
        done
    done
}

configure()
{
    mk_declare -i MK_CONFIG_HEADER="" MK_HEADERDEPS="" MK_LIBDEPS=""
    mk_declare -s -e \
        MK_CC MK_CXX MK_CC_STYLE MK_CC_LD_STYLE MK_CXX_STYLE MK_CXX_LD_STYLE
    mk_declare -i -e MK_CPPFLAGS MK_CFLAGS MK_CXXFLAGS MK_LDFLAGS
    mk_declare -s -i -e \
        MK_ISA_CPPFLAGS MK_ISA_CFLAGS MK_ISA_CXXFLAGS MK_ISA_LDFLAGS
    mk_declare -s MK_INTERNAL_LIBS

    mk_msg "default C compiler: $MK_DEFAULT_CC"
    mk_msg "default C++ compiler: $MK_DEFAULT_CXX"
    mk_msg "default C preprocessor flags: $MK_DEFAULT_CPPFLAGS"
    mk_msg "default C compiler flags: $MK_DEFAULT_CFLAGS"
    mk_msg "default C++ compiler flags: $MK_DEFAULT_CXXFLAGS"
    mk_msg "default linker flags: $MK_DEFAULT_LDFLAGS"

    for _sys in build host
    do
        _mk_define_name "MK_${_sys}_ISAS"
        mk_get "$result"
        
        for _isa in ${result}
        do
            _mk_define_name "$_sys/$_isa"
            _def="$result"

            mk_get "${_def}_CC"
            mk_msg "C compiler ($_sys/$_isa): $result"
            mk_set_system_var SYSTEM="$_sys/$_isa" MK_CC "$result"

            mk_get "${_def}_CXX"
            mk_msg "C++ compiler ($_sys/$_isa): $result"
            mk_set_system_var SYSTEM="$_sys/$_isa" MK_CXX "$result"

            mk_get "${_def}_CPPFLAGS"
            mk_msg "C preprocessor flags ($_sys/$_isa): $result"
            mk_set_system_var SYSTEM="$_sys/$_isa" MK_ISA_CPPFLAGS "$result"

            mk_get "${_def}_CFLAGS"
            mk_msg "C compiler flags ($_sys/$_isa): $result"
            mk_set_system_var SYSTEM="$_sys/$_isa" MK_ISA_CFLAGS "$result"

            mk_get "${_def}_CXXFLAGS"
            mk_msg "C++ compiler flags ($_sys/$_isa): $result"
            mk_set_system_var SYSTEM="$_sys/$_isa" MK_ISA_CXXFLAGS "$result"

            mk_get "${_def}_LDFLAGS"
            mk_msg "linker flags ($_sys/$_isa): $result"
            mk_set_system_var SYSTEM="$_sys/$_isa" MK_ISA_LDFLAGS "$result"
        done
    done

    _mk_compiler_check

    # Each invocation of mk_config_header closes and finishes up
    # the previous header.  In order to close the final config
    # header in the project, we register a completion hook as well.
    mk_add_complete_hook _mk_close_config_header

    mk_add_configure_prehook _mk_compiler_preconfigure
}

_mk_compiler_preconfigure()
{
    MK_CHECK_LANG="c"
}

### section build

_mk_compiler_multiarch_combine()
{
    mk_msg_domain "combine"

    mk_msg "${1#$MK_STAGE_DIR} (${MK_SYSTEM})"
    
    mk_mkdir "${1%/*}"

    case "$MK_OS" in
        darwin)
            mk_run_or_fail lipo -create -output "$@"
            ;;
        *)
            mk_fail "unsupported OS"
            ;;
    esac
}