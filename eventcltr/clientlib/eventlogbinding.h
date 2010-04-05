#ifndef __EVENTLOGBINDING_H__
#define __EVENTLOGBINDING_H__

DWORD
CltrCreateEventLogRpcBinding(
    IN const WCHAR * hostname,
    IN OPTIONAL const WCHAR * pwszServicePrincipal,
    IN WCHAR**       ppszBindingString,
    IN handle_t *   event_binding
    );

DWORD
CltrFreeEventLogRpcBinding(
    handle_t event_binding,
    WCHAR * pszBindingString
    );

#endif
