#!/bin/sh
autoreconf -fi || exit $?
cd libedit && autoreconf -fi && cd $ROOT || exit $?
