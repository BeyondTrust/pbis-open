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
DESTDIR=$5

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
	echo "$1" | sed "s:^.*${FINALPATH}:${DESTDIR}${FINALPATH}:"
    else
	echo "$1"
    fi
}

deps=""

case "$MODE" in
    -staging)
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
	;;
    -final)
	installed_value=`$SED -n <$LAFILE 's/installed=\(.*\)/\1/p'`
	platform_value=`$SED -n <$LAFILE 's/likewise_platform=\(.*\)/\1/p'`
	
	if test "$platform_value" = yes
	then
	    echo "Skipping ${LAFILE}: already processed"
	    exit 0
	fi

	if test $installed_value != yes
	then
	    deps="-L${DESTDIR}$(dirname "${LAFILE}" | sed "s:^.*${FINALPATH}:${FINALPATH}:")"
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

	    if [ -z "${DESTDIR}" ]
	    then
		echo "$(basename $LAFILE): Switching installed from $installed_value to yes"
		$SED <$LAFILE >$LAFILE.rewrite 's/installed=\(.*\)/installed=yes/'
		mv $LAFILE.rewrite $LAFILE
	    else
		echo "likewise_platform=yes" >> "$LAFILE"
	    fi
	    
	    objdir=`dirname $LAFILE/.libs`
	    if test -d $objdir; then
		echo "$(basename $LAFILE): Removing $objdir"
		rm $objdir
	    fi
	fi
	;;
esac

replace_dependencies "$LAFILE" "$deps"
exit $?
