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

LSASS_BASE=`dirname $0`
LSASS_BASE=`cd "$LSASS_BASE/.."; pwd`
rm -f "$LSASS_BASE/tags"
if [ -x /usr/bin/ctags.emacs22 ]; then
	/usr/bin/ctags.emacs22 --declarations --defines --globals --typedefs-and-c++ -o "$LSASS_BASE/tags" `find $LSASS_BASE -name '*.h' -o -name '*.c' -o -name '*.h.in'` || exit $?
else
	etags --declarations -o "$LSASS_BASE/tags" `find $LSASS_BASE -name '*.h' -o -name '*.c' -o -name '*.h.in'` || exit $?
fi
for dir in `find "$LSASS_BASE" -type d`; do
    if echo "$dir" | grep -v \\.svn >/dev/null &&
	anyfileexists "$dir"/*.h "$dir"/*.c; then
	echo "Updating tags symlink in $dir"
	rm -f "$dir/tags" 2>/dev/null
	ln -s "$LSASS_BASE/tags" "$dir/tags"
    fi
done
