#!/bin/bash

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

#EventDate=$1
#EventTime=$2
#EventSource=$3
#EventType=$4
#EventCategory=$5
#EventId=$6
#User=$7
#Computer=$8
#Description=$9

let iEventRec = 1
awk '{ FS=",";print $6, $4, $3, $5, $7, $8, $9}' app_events.csv |
    while read evt_id evt_type evt_source evt_category evt_user evt_computer evt_description; do
        case $evt_category in
             None)
                 evt_category=0
                 ;;
             Online)
                 evt_category=1
                 ;;
             General)
                 evt_category=2
                 ;;
             Error)
                 evt_category=3
                 ;;
             Hang)
                 evt_category=4
                 ;;
             Logging/Recovery)
                 evt_category=5
                 ;;
             Messenger)
                 evt_category=6
                 ;;
             NAT)
                 evt_category=7
                 ;;
             Search)
                 evt_category=8
                 ;;
             Smart)
                 evt_category=9
                 ;;
             Studio)
                 evt_category=10
                 ;;
             Virtual)
                 evt_category=11
                 ;;
             *)
                 evt_category=12
                 ;;
        esac
        case $evt_type in
             General)
                 evt_type=0
                 ;;
             Information)
                 evt_type=1
                 ;;
             Error)
                 evt_type=2
                 ;;
             *)
                 evt_type=3
                 ;;
        esac
        echo "Adding event record [$iEventRecord]"
        $PREFIX_DIR/bin/sqlite3 $EVENT_DB "insert into lwievents (EventId,
                                  EventType,
                                  EventSource,
                                  EventCategory,
                                  User,
                                  Computer,
                                  Description)
                       values ( $evt_id,
                                $evt_type,
                                \"$evt_source\",
                                $evt_category,
                                \"$evt_user\",
                                \"$evt_computer\",
                                \"$evt_description\")";
        let iEventRecord=$iEventRecord+1
    done
