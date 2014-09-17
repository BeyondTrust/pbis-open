#!/bin/bash
LAFILE=$1
PREFIX=$2

get_dependencies()
{
    . $1
    echo $dependency_libs
}

matches()
{
   echo $1 | grep $2 >/dev/null 2>&1
   return $?
}


for candidate in $(get_dependencies $LAFILE)
do
    if matches $candidate "^-L"
    then
	echo $candidate | sed 's/^-L//g'
    fi
done
