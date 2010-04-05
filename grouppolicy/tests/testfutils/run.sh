#!/bin/bash

BUILD_ROOT=../..
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BUILD_ROOT/libcentutils
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BUILD_ROOT/libgpcommon
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BUILD_ROOT/libgpldap

exec gdb ./testfutils $*
