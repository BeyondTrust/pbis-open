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

BASE=`dirname $0`
BASE=`cd "$BASE"; pwd`
rm -f "$BASE/tags"
/usr/bin/ctags.emacs22 --declarations --defines --globals --typedefs-and-c++ -o "$BASE/tags" `find $BASE -name '*.h' -o -name '*.c'`
for dir in `find "$BASE" -type d`; do
    if [ "$dir" != "$BASE" ] && echo "$dir" | grep -v \\.svn >/dev/null &&
	anyfileexists "$dir"/*.h "$dir"/*.c; then
	echo "Updating tags symlink in $dir"
	rm -f "$dir/tags" 2>/dev/null
	ln -s "$BASE/tags" "$dir/tags"
    fi
done
