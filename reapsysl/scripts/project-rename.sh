#!/bin/bash
show_help()
{
    echo "$0 [--old-macro <name> --new-macro <name>] [--old-file <name> --new-file <name>] [--old-function <name> --new-function <name>] [--old-description <name> --new-description <name>]"
    echo "macro name:"
    echo "    Capitalized name used in macros such as LWNET. This is also used in struct names."
    echo
    echo "file name:"
    echo "    Lower case name used in the name of files such as lwnet"
    echo
    echo "function name:"
    echo "    Camel case name used to prefix functions such as LWNet. This is also used in enum names."
    echo
    echo "description name:"
    echo "    Long name used in the comment header of source files, such as \"Likewise Security and Authentication Subsystem (LSASS)\""
}

while [ -n "$*" ]; do
    case "$1" in
        --old-macro)
            OLD_MACRO_NAME="$2"
            shift 2 || { echo "missing argument to $1"; exit 1; }
            ;;
        --new-macro)
            NEW_MACRO_NAME="$2"
            shift 2 || { echo "missing argument to $1"; exit 1; }
            ;;
        --old-file)
            OLD_FILE_NAME="$2"
            shift 2 || { echo "missing argument to $1"; exit 1; }
            ;;
        --new-file)
            NEW_FILE_NAME="$2"
            shift 2 || { echo "missing argument to $1"; exit 1; }
            ;;
        --old-function)
            OLD_FUNCTION_NAME="$2"
            shift 2 || { echo "missing argument to $1"; exit 1; }
            ;;
        --new-function)
            NEW_FUNCTION_NAME="$2"
            shift 2 || { echo "missing argument to $1"; exit 1; }
            ;;
        --old-description)
            OLD_DESCRIPTION_NAME="$2"
            shift 2 || { echo "missing argument to $1"; exit 1; }
            ;;
        --new-description)
            NEW_DESCRIPTION_NAME="$2"
            shift 2 || { echo "missing argument to $1"; exit 1; }
            ;;
        -h|--help)
            show_help
            exit 1
            ;;
        *)
            echo "Unknown option $1"
            show_help
            exit 1
            ;;
    esac
done

WHITE_SPACE=$' \\|\t'
TOKEN_SEPARATOR=$"^\\|$WHITE_SPACE\\|*\\|,\\|\$\\|;\\|(\\|)\\|\\[\\|\\]\\|{\\|}\\|!\\|&\\|~"

MACRO_EXPRESSION=""
FUNCTION_EXPRESSION=""
DESCRIPTION_EXPRESSION=""
if [ -n "$OLD_MACRO_NAME" ]; then
    if [ -z "$NEW_MACRO_NAME" ]; then
        echo "Error new name is null"
        exit 1
    fi
    MACRO_EXPRESSION="s/\\($TOKEN_SEPARATOR\\|_\\)\\(P\\?\\(FN\\)\\?\\)$OLD_MACRO_NAME\\($TOKEN_SEPARATOR\\|_\\)/\\1\\2$NEW_MACRO_NAME\\4/g"
fi
if [ -n "$OLD_FUNCTION_NAME" ]; then
    if [ -z "$NEW_FUNCTION_NAME" ]; then
        echo "Error new name is null"
        exit 1
    fi
    FUNCTION_EXPRESSION="s/\\(\\($TOKEN_SEPARATOR\\)g\\?p\\?\\)$OLD_FUNCTION_NAME\\([A-Z]\\)/\\1$NEW_FUNCTION_NAME\\3/g"
fi
if [ -n "$OLD_DESCRIPTION_NAME" ]; then
    if [ -z "$NEW_DESCRIPTION_NAME" ]; then
        echo "Error new name is null"
        exit 1
    fi
    DESCRIPTION_EXPRESSION="s/\\($WHTE_SPACE\\)$OLD_DESCRIPTION_NAME\\($WHITE_SPACE\\|\$\\)/\\1$NEW_DESCRIPTION_NAME\\2/g"
fi
if [ -n "$OLD_FILE_NAME" ]; then
    if [ -z "$NEW_FILE_NAME" ]; then
        echo "Error new name is null"
        exit 1
    fi

    # Find the directories that match the old name format
    OLD_FILE_REGEX=".*/$OLD_FILE_NAME"
    OLD_FILE_LIST=`find . -regex "$OLD_FILE_REGEX" -a -type d -exec /bin/bash -c 'printf "%q " "{}"' \;`
    eval set "''" "$OLD_FILE_LIST"
    shift
    # Rename the directories
    for OLD_FILE; do
        NEW_FILE=`echo "$OLD_FILE" | sed "s/$OLD_FILE_NAME/$NEW_FILE_NAME/"`
	echo "Renaming $OLD_FILE to $NEW_FILE"
	svn mv "$OLD_FILE" "$NEW_FILE"
    done

    # Find the .c and .h files that match the old name format
    OLD_FILE_REGEX=".*/$OLD_FILE_NAME\\(-[a-z_-]+\\)?\\.\\(c\\|h\\)"
    OLD_FILE_LIST=`find . -regex "$OLD_FILE_REGEX" -exec /bin/bash -c 'printf "%q " "{}"' \;`
    eval set "''" "$OLD_FILE_LIST"
    shift
    # Rename the files
    for OLD_FILE; do
        NEW_FILE=`echo "$OLD_FILE" | sed "s/$OLD_FILE_NAME/$NEW_FILE_NAME/"`
        if [ -f "$NEW_FILE" ]; then
            echo "Error: $NEW_FILE already exists"
            exit 1
        fi
    done
    for OLD_FILE; do
        NEW_FILE=`echo "$OLD_FILE" | sed "s/$OLD_FILE_NAME/$NEW_FILE_NAME/"`
	echo "Renaming $OLD_FILE to $NEW_FILE"
        # Change the filename in the source header
        sed "s/\\( *\\* *\\)`basename "$OLD_FILE"`/\\1`basename "$NEW_FILE"`/" $OLD_FILE >$NEW_FILE
        mv "$OLD_FILE" "$OLD_FILE.bak"
	# Edit the makefile in the same directory and update it to use the
        # new filename
        MAKEFILE_AM=`dirname $OLD_FILE`/Makefile.am
        if [ -f "$MAKEFILE_AM" ] && grep "$OLD_FILE" "$MAKEFILE_AM" >/dev/null; then
	    echo "Updating $MAKEFILE_AM with the new filename"
            sed "s/^\\(\\($WHITE_SPACE\\)*\\)`basename $OLD_FILE`\\($WHITE_SPACE\\|\\\\\\|\$\\)/\\1`basename $NEW_FILE`\\3/" "$MAKEFILE_AM" "-i.`basename $OLD_FILE`"
        fi
    done
    # Rename the libraries with the old name
    FILE_LIST=`find . \( -name Makefile.am \) -a -exec /bin/bash -c 'printf "%q " "{}"' \;`
    eval set "''" "$FILE_LIST"
    shift
    echo "Renaming libraries and binaries in makefiles to match the new name"
    TAB=$'\t'
    sed -e "s/lib$OLD_FILE_NAME/lib$NEW_FILE_NAME/g" -e "s/\\(^\\| \\|$TAB\\)$OLD_FILE_NAME\\(d\\|[_-][a-z_-]*\\)/\\1$NEW_FILE_NAME\\2/g" -i.bak "$@"
    # Set up an expression to switch the include directives later on
    FILE_EXPRESSION="s/\\(#include\\($WHITE_SPACE\\)\\+[<\"]\\([^>\"]\\+\\/\\)\\?\\)$OLD_FILE_NAME\\(\\(-[^>\"]\\+\\)\\?\\.h[>\"]\\($WHITE_SPACE\\)*\\)$/\\1$NEW_FILE_NAME\\4/"
    FILE_EXPRESSION="$FILE_EXPRESSION;s/\\(#include\\($WHITE_SPACE\\)\\+[<\"]\\([^>\"]\\+\\/\\)\\?\\)$OLD_FILE_NAME\\/\\(.*[>\"]\\($WHITE_SPACE\\)*\\)$/\\1$NEW_FILE_NAME\\/\\4/"
fi
FILE_LIST=`find . \( -name '*.c' -o -name '*.h' -o -name '*.idl' \) -a -exec /bin/bash -c 'printf "%q " "{}"' \;`
eval set "''" "$FILE_LIST"
shift
echo sed -e "$MACRO_EXPRESSION" -e "$FUNCTION_EXPRESSION" -e "$DESCRIPTION_EXPRESSION" -e "$FILE_EXPRESSION" -i.bak "$@"
sed -e "$MACRO_EXPRESSION" -e "$FUNCTION_EXPRESSION" -e "$DESCRIPTION_EXPRESSION" -e "$FILE_EXPRESSION" -i.bak "$@"
