/*
 * Copyright (C) Likewise Software.  All rights reserved.
 */
#ifndef LWAUTOENROLL_X509_H
#define LWAUTOENROLL_X509_H

#include <lwautoenroll/lwautoenroll.h>

#include <lw/attrs.h>
#include <lw/types.h>

DWORD
GenerateX509Request(
        IN const PLW_AUTOENROLL_TEMPLATE pTemplate,
        IN PCSTR userNetbiosDomain,
        IN PCSTR userSamName,
        IN OPTIONAL X509_NAME *pSubjectName,
        IN OUT OPTIONAL EVP_PKEY **ppKeyPair,
        OUT X509_REQ **ppRequest
        );

#endif /* LWAUTOENROLL_X509_H */
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
