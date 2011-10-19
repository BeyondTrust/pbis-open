#!/bin/sh

DCE_BASE_DIR=/usr/centeris

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DCE_BASE_DIR/lib
export LD_LIBRARY_PATH

exec ./importevents "$@"

