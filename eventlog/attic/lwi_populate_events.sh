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

insert into lwievents (EventId,
                       EventType,
                       EventSource,
                       EventCategory,
                       User,
                       Computer,
                       Description)
       values (        100,
                       1,
                       "lwi-evt-src-1",
                       1,
                       "lwi-user-1",
                       "lwi-computer-1",
                       "Mary had a little lamb(1)" 
              );
insert into lwievents (EventId,
                       EventType,
                       EventSource,
                       EventCategory,
                       User,
                       Computer,
                       Description)
       values (        100,
                       1,
                       "lwi-evt-src-2",
                       1,
                       "lwi-user-2",
                       "lwi-computer-2",
                       "Mary had a little lamb(2)" 
              );
insert into lwievents (EventId,
                       EventType,
                       EventSource,
                       EventCategory,
                       User,
                       Computer,
                       Description)
       values (        100,
                       1,
                       "lwi-evt-src-3",
                       1,
                       "lwi-user-3",
                       "lwi-computer-3",
                       "Mary had a little lamb(3)" 
              );
insert into lwievents (EventId,
                       EventType,
                       EventSource,
                       EventCategory,
                       User,
                       Computer,
                       Description)
       values (        100,
                       1,
                       "lwi-evt-src-4",
                       1,
                       "lwi-user-4",
                       "lwi-computer-4",
                       "Mary had a little lamb(4)" 
              );
insert into lwievents (EventId,
                       EventType,
                       EventSource,
                       EventCategory,
                       User,
                       Computer,
                       Description)
       values (        100,
                       1,
                       "lwi-evt-src-5",
                       1,
                       "lwi-user-5",
                       "lwi-computer-5",
                       "Mary had a little lamb(5)" 
              );
insert into lwievents (EventId,
                       EventType,
                       EventSource,
                       EventCategory,
                       User,
                       Computer,
                       Description)
       values (        100,
                       1,
                       "lwi-evt-src-6",
                       1,
                       "lwi-user-6",
                       "lwi-computer-6",
                       "Mary had a little lamb(6)" 
              );
insert into lwievents (EventId,
                       EventType,
                       EventSource,
                       EventCategory,
                       User,
                       Computer,
                       Description)
       values (        100,
                       1,
                       "lwi-evt-src-7",
                       1,
                       "lwi-user-7",
                       "lwi-computer-7",
                       "Mary had a little lamb(7)" 
              );
insert into lwievents (EventId,
                       EventType,
                       EventSource,
                       EventCategory,
                       User,
                       Computer,
                       Description)
       values (        100,
                       1,
                       "lwi-evt-src-8",
                       1,
                       "lwi-user-8",
                       "lwi-computer-8",
                       "Mary had a little lamb(8)" 
              );

EOF

