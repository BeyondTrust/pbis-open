#!/bin/bash
show_help()
{
    echo "$0 [--old <name> --new <name>]"
    echo "This program takes the old and new name of a function to rename inside of all source code"
}

while [ -n "$*" ]; do
    case "$1" in
        --old)
            OLD_NAME="$2"
            shift 2 || { echo "missing argument to $1"; exit 1; }
            ;;
        --new)
            NEW_NAME="$2"
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

if [ -z "$NEW_NAME" ]; then
	echo "Error new name is null"
	show_help
	exit 1
fi
if [ -z "$OLD_NAME" ]; then
	echo "Error old name is null"
	show_help
	exit 1
fi
FILE_LIST=`find . \( -name '*.c' -o -name '*.h' -o -name '*.idl' \) -a -exec /bin/bash -c 'printf "%q " "{}"' \;`
eval set "''" "$FILE_LIST"
shift
sed -e "s/$OLD_NAME\([ ]*(\)/$NEW_NAME\1/" -i.bak "$@"
