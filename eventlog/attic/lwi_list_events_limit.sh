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

if [ $# -lt 2 ]; then
   echo "Usage: lwi_list_events_limit.sh <last row id> <number of records per page>"
   exit 1
fi

LAST_ROW_ID=$1
LIMIT_NUM_RECORDS=$2

$PREFIX_DIR/bin/sqlite3 $EVENT_DB << EOF

select rowid,
       EventRecordId,
       EventId,
       EventType,
       EventTime,
       EventSource,
       EventCategory,
       User,
       Computer,
       Description
  from lwievents
ORDER BY EventTime DESC
LIMIT $LIMIT_NUM_RECORDS OFFSET $LAST_ROW_ID;

EOF

