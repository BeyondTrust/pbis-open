AS_INIT[]dnl                                            -*- shell-script -*-
# autoconf -- create `configure' using m4 macros

# Copyright (C) 1992, 1993, 1994, 1996, 1999, 2000, 2001, 2002, 2003,
# 2004, 2005, 2006 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

usage=["\
Usage: $0 [OPTION] ... [TEMPLATE-FILE]

Generate a configuration script from a TEMPLATE-FILE if given, or
\`configure.ac' if present, or else \`configure.in'.  Output is sent
to the standard output if TEMPLATE-FILE is given, else into
\`configure'.

Operation modes:
  -h, --help                print this help, then exit
  -V, --version             print version number, then exit
  -v, --verbose             verbosely report processing
  -d, --debug               don't remove temporary files
  -f, --force               consider all files obsolete
  -o, --output=FILE         save output in FILE (stdout is the default)
  -W, --warnings=CATEGORY   report the warnings falling in CATEGORY [syntax]

Warning categories include:
  \`cross'         cross compilation issues
  \`obsolete'      obsolete constructs
  \`syntax'        dubious syntactic constructs
  \`all'           all the warnings
  \`no-CATEGORY'   turn off the warnings on CATEGORY
  \`none'          turn off all the warnings
  \`error'         warnings are error

The environment variables \`M4' and \`WARNINGS' are honored.

Library directories:
  -B, --prepend-include=DIR  prepend directory DIR to search path
  -I, --include=DIR          append directory DIR to search path

Tracing:
  -t, --trace=MACRO     report the list of calls to MACRO
  -i, --initialization  also trace Autoconf's initialization process

In tracing mode, no configuration script is created.

Report bugs to <bug-autoconf@gnu.org>."]

version=["\
autoconf (@PACKAGE_NAME@) @VERSION@
Copyright (C) 2006 Free Software Foundation, Inc.
This is free software.  You may redistribute copies of it under the terms of
the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.
There is NO WARRANTY, to the extent permitted by law.

Written by David J. MacKenzie and Akim Demaille."]

help="\
Try \`$as_me --help' for more information."

exit_missing_arg="\
echo \"$as_me: option \\\`\$1' requires an argument\" >&2
echo \"\$help\" >&2
exit 1"

# Variables.
: ${AUTOM4TE='@bindir@/@autom4te-name@'}
autom4te_options=
outfile=
verbose=:

# Parse command line.
while test $# -gt 0 ; do
  option=[`expr "x$1" : 'x\(--[^=]*\)' \| \
	       "x$1" : 'x\(-.\)'`]
  optarg=[`expr "x$1" : 'x--[^=]*=\(.*\)' \| \
	       "x$1" : 'x-.\(.*\)'`]
  case $1 in
    --version | -V )
       echo "$version" ; exit ;;
    --help | -h )
       echo "$usage"; exit ;;

    --verbose | -v )
       verbose=echo
       autom4te_options="$autom4te_options $1"; shift ;;

    # Arguments passed as is to autom4te.
    --debug      | -d   | \
    --force      | -f   | \
    --include=*  | -I?* | \
    --prepend-include=* | -B?* | \
    --warnings=* | -W?* )
       autom4te_options="$autom4te_options '$1'"; shift ;;

    # Options with separated arg passed as is to autom4te.
    --include  | -I | \
    --prepend-include  | -B | \
    --warnings | -W )
       test $# = 1 && eval "$exit_missing_arg"
       autom4te_options="$autom4te_options $option '$2'"
       shift; shift ;;

    --trace=* | -t?* )
       traces="$traces --trace='"`echo "$optarg" | sed "s/'/'\\\\\\\\''/g"`"'"
       shift ;;
    --trace | -t )
       test $# = 1 && eval "$exit_missing_arg"
       traces="$traces --trace='"`echo "$2" | sed "s/'/'\\\\\\\\''/g"`"'"
       shift; shift ;;
    --initialization | -i )
       autom4te_options="$autom4te_options --melt"
       shift;;

    --output=* | -o?* )
       outfile=$optarg
       shift ;;
    --output | -o )
       test $# = 1 && eval "$exit_missing_arg"
       outfile=$2
       shift; shift ;;

    -- )     # Stop option processing
       shift; break ;;
    - )	# Use stdin as input.
       break ;;
    -* )
       exec >&2
       echo "$as_me: invalid option $1"
       echo "$help"
       exit 1 ;;
    * )
       break ;;
  esac
done

# Find the input file.
case $# in
  0)
    if test -f configure.ac; then
      if test -f configure.in; then
	echo "$as_me: warning: both \`configure.ac' and \`configure.in' are present." >&2
	echo "$as_me: warning: proceeding with \`configure.ac'." >&2
      fi
      infile=configure.ac
    elif test -f configure.in; then
      infile=configure.in
    else
      echo "$as_me: no input file" >&2
      exit 1
    fi
    test -z "$traces" && test -z "$outfile" && outfile=configure;;
  1) # autom4te doesn't like `-'.
     test "x$1" != "x-" && infile=$1 ;;
  *) exec >&2
     echo "$as_me: invalid number of arguments."
     echo "$help"
     (exit 1); exit 1 ;;
esac

# Unless specified, the output is stdout.
test -z "$outfile" && outfile=-

# Run autom4te with expansion.
eval set x $autom4te_options \
  --language=autoconf --output=\$outfile "$traces" \$infile
shift
$verbose "$as_me: running $AUTOM4TE $*" >&2
exec "$AUTOM4TE" "$@"
