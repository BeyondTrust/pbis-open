#!/bin/bash

if type gsed >/dev/null 2>&1
then
    SED=gsed
else
    SED=sed
fi

MODE=$1
LAFILE=$2
FINALPATH=$3
LAPATH=`echo $4 | $SED 's/::*/ /g'`

get_dependencies()
{
    . $1
    echo $dependency_libs
}

replace_dependencies()
{
    file=$1
    replace=$2
    $SED "s:^dependency_libs=.*$:dependency_libs='$replace':" <${file} >${file}.rewrite
    mv ${file}.rewrite ${file}
}

matches()
{
   echo $1 | grep $2 >/dev/null 2>&1
   return $?
}

contains()
{
    haystack="$1"
    needle="$2"

    for i in ${haystack}
    do
	if [ "$i" = "$needle" ]
	then
	    return 0
	fi
    done

    return 1
}

pathchomp()
{
    echo $1 | $SED 's://*:/:g'
}

rewrite_staging_la_dep()
{
    candidate=$1
    found=""

    for staging in $LAPATH
    do
	if test -f "$staging/$candidate"
	then
	    found="$staging/$candidate"
	fi
    done
    
    if test -n "$found"
    then
	pathchomp $found
    else
	pathchomp $candidate
    fi
}

rewrite_final_la_dep()
{
    candidate=$1
    found=""
    minus_l=""

    if [ "${candidate:0:2}" = "-L" ]
    then
	candidate="${candidate:2}"
	minus_l="-L"
    fi

    for staging in $LAPATH
    do
	if matches $(pathchomp $candidate) "^$(pathchomp $staging)"
	then
	    found="$staging"
	fi
    done
    
    if test -n "$found"
    then
	transform="`pathchomp $(echo $candidate | $SED "s:^$found::")`"
	if ! matches "$transform" "^$FINALPATH"
	then
	    transform="`pathchomp $FINALPATH/$transform`"
	fi
	echo "${minus_l}$transform"
    else
	echo "${minus_l}`pathchomp $candidate`"
    fi
}

deps=""

if test "$MODE" = "-staging"
then
    echo "$(basename $LAFILE): Rewriting dependencies to reference staging directories"
    for candidate in $(get_dependencies $LAFILE)
    do
	if matches $candidate "^${FINALPATH}/.*\.la$"
	then
	    rewrite=$(rewrite_staging_la_dep $candidate)
	    echo " - Rewriting $candidate -> $rewrite"
	    deps="$deps $rewrite"
	else
	    deps="$deps $(pathchomp $candidate)"
	fi
    done
    installed_value=`$SED -n <$LAFILE 's/installed=\(.*\)/\1/p'`
    if test $installed_value != no; then
	echo "$(basename $LAFILE): Switching installed from $installed_value to no"
	$SED <$LAFILE >$LAFILE.rewrite 's/installed=\(.*\)/installed=no/'
	mv $LAFILE.rewrite $LAFILE
    fi
    objdir=`dirname $LAFILE`/.libs
    if test ! -d $objdir; then
	echo "$(basename $LAFILE): Creating $objdir"
	ln -s . $objdir
    fi
elif test "$MODE" = "-final"
then
    seen=""
    echo "$(basename $LAFILE): Rewriting dependencies to reference final directory"
    for candidate in $(get_dependencies $LAFILE)
    do
	if (matches "$candidate" "\.la$" || matches "$candidate" "^-L") && ! matches $candidate "^${FINALPATH}"
	then
	    rewrite=$(rewrite_final_la_dep $candidate)
	    if [ "$candidate" != "$rewrite" ]
	    then
		echo " - Rewriting $candidate -> $rewrite"
	    fi

	    if ! contains "$seen" "$rewrite"
	    then
		deps="$deps $rewrite"
		seen="$seen $rewrite"
	    fi
	else
	    deps="$deps $(pathchomp $candidate)"
	fi
    done
    installed_value=`$SED -n <$LAFILE 's/installed=\(.*\)/\1/p'`
    if test $installed_value != yes; then
	echo "$(basename $LAFILE): Switching installed from $installed_value to yes"
	$SED <$LAFILE >$LAFILE.rewrite 's/installed=\(.*\)/installed=yes/'
	mv $LAFILE.rewrite $LAFILE
    fi
    objdir=`dirname $LAFILE/.libs`
    if test -d $objdir; then
	echo "$(basename $LAFILE): Removing $objdir"
	rm $objdir
    fi
fi

replace_dependencies "$LAFILE" "$deps"
