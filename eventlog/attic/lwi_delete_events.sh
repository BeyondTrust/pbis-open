#!/bin/sh

PREFIX_DIR=/usr/centeris
EVENT_DB_DIR=/var/cache/centeris/eventlogd
EVENT_DB_FILENAME=lwi_events.db

if [ ! -f $PREFIX_DIR/bin/sqlite3 ]; then
   echo "Error: Cannot find $PREFIX_DIR/bin/sqlite3"
   exit 1
fi

EVENT_DB=$EVENT_DB_DIR/$EVENT_DB_FILENAME

if [ ! -f $EVENT_DB ]; then
   echo "Failed to find event database at $EVENT_DB"
   exit 1
fi

$PREFIX_DIR/bin/sqlite3 $EVENT_DB << EOF

delete from lwievents;

EOF

