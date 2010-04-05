#!/bin/bash

PROGRAM=`basename $0 | sed 's/-wrap.sh//'`

dispatch()
{
    exec "$@"
}

for arg in "$@"; do
    if echo "$arg" | grep -- '-Wl,-R' 2>&1 >/dev/null; then
	path=`echo "$arg" | sed 's/-Wl,-R,*//'`
	RPATH="$RPATH:$path"
    elif echo "$arg" | grep -- '-Wl,-bsvr4' 2>&1 >/dev/null; then
	:
    else
	ARGUMENTS=( "${ARGUMENTS[@]}" "$arg" )
    fi
done

if [ -z $RPATH ]; then
    dispatch "$PROGRAM" "${ARGUMENTS[@]}"
else
    dispatch "$PROGRAM" "${ARGUMENTS[@]}" "-Wl,-blibpath:/lib:/usr/lib:/usr/local/lib:$RPATH"
fi