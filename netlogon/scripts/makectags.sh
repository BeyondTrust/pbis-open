#!/bin/bash

#returns 0 if any of the files passed as a parameter exist
anyfileexists()
{
    for file in "$@"; do
	if [ -e "$file" ]; then
	    return 0
	fi
    done
    return 1
}

PROJECT_BASE=`dirname $0`
PROJECT_BASE=`cd "$PROJECT_BASE/.."; pwd`
rm -f "$PROJECT_BASE/tags"
/usr/bin/ctags.emacs22 --declarations --defines --globals --typedefs-and-c++ -o "$PROJECT_BASE/tags" `find $PROJECT_BASE -name '*.h' -o -name '*.c'`
for dir in `find "$PROJECT_BASE" -type d`; do
    if echo "$dir" | grep -v \\.svn >/dev/null &&
	anyfileexists "$dir"/*.h "$dir"/*.c; then
	echo "Updating tags symlink in $dir"
	rm -f "$dir/tags" 2>/dev/null
	ln -s "$PROJECT_BASE/tags" "$dir/tags"
    fi
done
