#!/bin/bash
###########################################################################
# This is a temporary script, will not be needed after rpcgen is integrated
# into the make system.
###########################################################################

ls . | grep "\.x" | cut -d'.' -f1 | perl -nle 'system("rpcgen -h $_.x > include/$_.h");'
ls . | grep "\.x" | cut -d'.' -f1 | perl -nle 'system("rpcgen -c $_.x > $_\_xdr.c");'
