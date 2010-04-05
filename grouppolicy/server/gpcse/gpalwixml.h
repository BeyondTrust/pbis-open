#ifndef GPALWIXML_H
#define GPALWIXML_H

/* A [get] Value function that, given an attribute node, returns the value of the attribute. */
CENTERROR
GPOXmlGetAttributeValue(
    xmlNodePtr Node,
    char** Result );

/* A get optional node value function that, given a node, does a SelectSingleNode on it, and, if the
   result is an attribute node, calls the get value function on the result.  Otherwise, returns NULL. */
CENTERROR
GPOXmlGetOptionalAttributeValue(
    xmlNodePtr Node,
    const char* AttributeQuery,
    char** Result );

CENTERROR
GPOGetPlatformInfo(
    PSTR* os,
    PSTR* distro,
    PSTR* version
    );

#endif
