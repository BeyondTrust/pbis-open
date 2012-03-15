#include "config.h"
#include "eventsys.h"
#include <locale.h>
#include "eventlog.h"
#include "eventdefs.h"
#include "eventutils.h"

#include "wc16printf.h"
#include <lwstr.h>
#include <lwmem.h>
#include <lwerror.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "defs.h"
#include "evtparser.h"
#include "xmllookup.h"
