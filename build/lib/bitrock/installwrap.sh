#!/bin/sh

#
# $1 is directory where everything lives
# $2 is command to perform
# $3 is whether to install compat libs (auto/yes/no)
#

_compat=""
case "$3" in
    yes)
        _compat="--compat"
        ;;
    no)
        _compat="--nocompat"
        ;;
esac

"$1/install.sh" --dir "$1" ${_compat} "$2"
