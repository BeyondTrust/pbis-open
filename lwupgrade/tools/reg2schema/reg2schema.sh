#!/bin/sh
#
# Copyright (C) Likewise Software. All rights reserved.
#
# Module Name: reg2schema.sh
#
# Abstract:
# Simple script to convert legacy (6.0 and earlier) registry files
# to new schema format. The conversion generates a skeleton schema
# entry for each name=value pair. An empty doc="" entry is added for
# filling in later with descriptive information.
#
# Authors: Adam Bernstein (abernstein@likewise.com)
#

#Temporary sed script file
sedfile=/tmp/reg2schema2$$.sed

# Create sed script file in "$sedfile"
cat <<NNNN>$sedfile
s|=\(.*\)| = { \\
    default = \1 \\
    doc = "" \\
}|
NNNN


# Process all reg files presented on command line
while [ -n "$1" ]; do
  infile=""
  if [ -f "$1" ]; then
    infile="$1"
  else
    echo "ERROR: '$1' not found'; skipping processing of file"
  fi
  if [ -n "$infile" ]; then
    cat "$infile" | sed -f $sedfile > "$infile.s"
    #cat "$infile" | sed -f $sedfile 
    if [ -s "${infile}.s" ]; then
      if [ ! -f "$infile.bak" ]; then
        mv "$infile" "$infile.bak"
        mv "$infile.s" "$infile"
      else
        echo "WARNING: file '$infile' appears to already be converted"
        echo "         file '$infile.bak' exists, remove and try again"
      fi
    else
      echo "WARNING: an empty schema file was generated for '$infile'"
    fi
  fi
  shift
done

rm -f $sedfile
