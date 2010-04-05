#!/bin/bash

BUILD_ROOT=../..
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BUILD_ROOT/libcentutils
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BUILD_ROOT/libgpcommon
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BUILD_ROOT/libgpclient
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BUILD_ROOT/libgpagent
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BUILD_ROOT/libgpldap
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/centeris/lib

#exec gdb ./ldap $*
./ldap $*
