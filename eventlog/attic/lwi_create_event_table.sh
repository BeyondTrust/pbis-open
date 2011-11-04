#!/bin/sh

PREFIX_DIR=/usr/centeris
EVENT_DB_DIR=/var/cache/centeris/eventlogd
EVENT_DB_FILENAME=lwi_events.db

if [ ! -f $PREFIX_DIR/bin/sqlite3 ]; then
   echo "Error: Cannot find $PREFIX_DIR/bin/sqlite3"
   exit 1
fi

if [ ! -d $EVENT_DB_DIR ]; then
   mkdir -p $EVENT_DB_DIR
   if [ $? -ne 0 ]; then
      echo "Failed to create folder at $EVENT_DB_DIR"
      exit 1
   fi
fi

EVENT_DB=$EVENT_DB_DIR/$EVENT_DB_FILENAME

if [ -f $EVENT_DB ]; then
   echo "Removing current event database at $EVENT_DB"
   /bin/rm -f $EVENT_DB
   if [ $? -ne 0 ]; then
      echo "Failed to remove current event database at $EVENT_DB"
      exit 1
   fi
fi

$PREFIX_DIR/bin/sqlite3 $EVENT_DB << EOF

create table lwievents (EventRecordId integer PRIMARY KEY AUTOINCREMENT,
                        EventId       integer,
                        EventType     integer,
                        EventTime     date, 
                        EventSource   varchar(256),
                        EventCategory integer,
                        User          varchar(128),
                        Computer      varchar(128),
                        Description   varchar(256)
                       );
create trigger lwievents_eventtime_trigger after insert on lwievents 
    begin
       update lwievents set EventTime = DATETIME('NOW') where rowid = new.rowid;
    end;

EOF

echo "Successfully created events table at $EVENT_DB"

