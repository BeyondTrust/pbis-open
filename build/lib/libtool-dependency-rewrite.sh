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
    $SED "s:^dependency_libs=.*$:dependency_libs='$replace':" <${file} >${file}.rewrite || return $?
    mv ${file}.rewrite ${file}
}

matches()
{
   echo "$1" | grep "$2" >/dev/null 2>&1
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
    echo "$1" | $SED 's://*:/:g'
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
	echo $found
    else
	echo $candidate
    fi
}

rewrite_final_la_dep()
{
    if matches "$1" "^-L"
    then
	echo ""
    elif matches "$1" "${FINALPATH}/.*\.la$"
    then
	echo "$1" | sed "s:^.*${FINALPATH}:${FINALPATH}:"
    else
	echo "$1"
    fi
}

deps=""

if test "$MODE" = "-staging"
then
    echo "$(basename $LAFILE): Rewriting dependencies to reference staging directories"
    for candidate in $(get_dependencies $LAFILE)
    do
	case "$candidate" in
            ${FINALPATH}/*.la)
                rewrite=$(rewrite_staging_la_dep $candidate)
                echo " - Rewriting $candidate -> $rewrite"
                deps="$deps $rewrite"
                ;;
            *)
                deps="$deps $candidate"
                ;;
        esac
    done
    deps="$(pathchomp "$deps")"
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
    installed_value=`$SED -n <$LAFILE 's/installed=\(.*\)/\1/p'`
    if test $installed_value != yes
    then
	deps="-L$(dirname "${LAFILE}" | sed "s:^.*${FINALPATH}:${FINALPATH}:")"
	echo "$(basename $LAFILE): Rewriting dependencies to reference final directory"
	for candidate in $(get_dependencies $LAFILE)
	do
	    rewrite="$(rewrite_final_la_dep $candidate)"
	    if [ "$candidate" != "$rewrite" ]
	    then
		echo " - Rewriting $candidate -> $rewrite"
	    fi
	    deps="$deps $rewrite"
	done
	echo "$(basename $LAFILE): Switching installed from $installed_value to yes"
	$SED <$LAFILE >$LAFILE.rewrite 's/installed=\(.*\)/installed=yes/'
	mv $LAFILE.rewrite $LAFILE

	objdir=`dirname $LAFILE/.libs`
	if test -d $objdir; then
	    echo "$(basename $LAFILE): Removing $objdir"
	    rm $objdir
	fi
    fi
fi

replace_dependencies "$LAFILE" "$deps"
exit $?
