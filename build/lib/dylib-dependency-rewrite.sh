#!/bin/bash

get_dependencies()
{
    otool -L "$1" | tail +2 | awk '{print $1;}'
}

matches()
{
   echo $1 | grep $2 >/dev/null 2>&1
   return $?
}

matches_staging()
{
    for staging in $DYLIBPATH
    do
	case "$1" in
	    ${staging}/*)
		return 0
		;;
	esac
    done
    return 1
}

pathchomp()
{
    echo $1 | sed 's://*:/:g'
}

rewrite_staging_dylib_dep()
{
    candidate=$1
    found=""

    if [ "$(basename $candidate)" = "$candidate" ]
    then
	candidate=$(pathchomp "$FINALPATH/$candidate")
    fi

    for staging in $DYLIBPATH
    do
        if test -e "$staging/$candidate"
	then
	    found="$staging/$candidate"
            break
	fi
    done
    
    if test -n "$found"
    then
	pathchomp $found
    else
	pathchomp $candidate
    fi
}

rewrite_final_dylib_dep()
{
    candidate=$1
    found=""

    for staging in $DYLIBPATH
    do
	if matches $(pathchomp $candidate) "^$(pathchomp $staging/$FINALPATH)"
	then
	    found="$staging"
            break
	fi
    done
    
    if test -n "$found"
    then
	pathchomp $(echo $candidate | sed "s:^$found::")
    else
	pathchomp $candidate
    fi
}

MODE=$1
DYLIBFILE=`pathchomp $2`
FINALPATH=`pathchomp $3`
DYLIBPATH=`echo $4 | sed 's/::*/ /g'`

deps=""

if test "$MODE" = "-staging"
then
    echo "$(basename $DYLIBFILE): Rewriting dependencies to reference staging directories"
    for candidate in $(get_dependencies $DYLIBFILE)
    do
        if (matches $candidate "^${FINALPATH}/.*\.dylib.*$" || 
		[ "$(basename $candidate)" = "$candidate" ] )  && 
	    ! (test "$candidate" = "$DYLIBFILE" || test "$candidate" = "$(basename $DYLIBFILE)")
	then
	    rewrite=$(rewrite_staging_dylib_dep $candidate)
	    echo " - Rewriting $candidate -> $rewrite"
	    install_name_tool -change "$candidate" "$rewrite" "$DYLIBFILE"
	fi
    done
elif test "$MODE" = "-final"
then
    echo "$(basename $DYLIBFILE): Rewriting dependencies to reference final directory"
    for candidate in $(get_dependencies $DYLIBFILE)
    do
	case "$candidate" in
	    */$DYLIBFILE)
		# If the library depends on itself, do not rewrite that
                # dependency
		continue
		;;
	esac
	if matches_staging $candidate &&
	    ! test "$candidate" = "$DYLIBFILE"
	then
	    rewrite=$(rewrite_final_dylib_dep $candidate)
	    echo " - Rewriting $candidate -> $rewrite"
	    rewrites="$rewrites -change `printf %q "$candidate"` `printf %q "$rewrite"`"
	fi
    done
    if [ -n "$rewrites" ]; then
	echo running "install_name_tool $rewrites `printf %q "$DYLIBFILE"`"
	eval "install_name_tool $rewrites `printf %q "$DYLIBFILE"`"
    fi
fi
