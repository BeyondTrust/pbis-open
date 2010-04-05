#ifndef _WIN32_WINNT        // Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501    // Change this to the appropriate value to target other versions of Windows.
#endif

#include <windows.h>
#include "cltr-base.h"
#include "defines.h"
#include "sqlite3.h"
#include "cltr-dbstruct.h"
#include "structs.h"
#include "eventlog_h.h"
#include "cltr-server.h"
#include "cltr-db.h"
#include "cltr-pcntl.h"
#include "externs.h"
#include <sddl.h>
#include <iads.h>
#include <shlwapi.h>
#include "settings.h"

#pragma warning(disable : 4996)

#define _POSIX_PTHREAD_SEMANTICS 1
