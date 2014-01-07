#!/bin/bash

top=`pwd`

autoheader
autoconf -I "$top"
