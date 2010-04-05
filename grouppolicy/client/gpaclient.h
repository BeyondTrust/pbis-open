#ifndef __GPACLIENT_H__
#define __GPACLIENT_H__

CENTERROR
GPOClientSendMessage(
    PGPOCLIENTCONNECTIONCONTEXT pContext,
    PGPOMESSAGE pMessage
    );

CENTERROR
GPOClientGetNextMessage(
    PGPOCLIENTCONNECTIONCONTEXT pContext,
    PGPOMESSAGE* ppMessage
    );

CENTERROR
GPOClientVerifyConnection(
    PGPOCLIENTCONNECTIONCONTEXT pContext
    );

#endif /*__GPACLIENT_H__*/
