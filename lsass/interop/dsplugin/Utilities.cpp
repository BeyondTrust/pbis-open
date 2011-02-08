/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "LWIPlugIn.h"

#define LWE_MIN(a, b) ((a < b)?(a):(b))

#undef USE_SYSLOG

#ifdef USE_SYSLOG
void LogMessageV(const char *Format, va_list Args)
{
    vsyslog(LOG_ERR, Format, Args);
}
#else
static void LogMessageToDsLog(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    DSDebugLog(format, args);
    va_end(args);
}

void LogMessageV(const char *Format, va_list Args)
{
    char* output = NULL;

    // Note: DSDebugLog eventually calls CString::Vsprintf (inside DirectoryServer's
    // CoreFramework/Private/CString.cpp), which is not compatible with sprintf
    // despite the documentation in Apple's "Open Directory Reference").
    // To avoid compatibility issues, we will do our own formatting first.
    //
    // We also discovered another bug in CString::Vsprintf where a '%' in string
    // that we are formatting with %s can cause a crash.  So we must make sure that
    // there are no '%'s in the string that we are trying to output.
    //

    vasprintf(&output, Format, Args);
    if (output)
    {
        for (char *p = output; *p; p++)
        {
            if ('%' == *p)
            {
                *p = '*';
            }
        }

        LogMessageToDsLog("%s", output);
        free(output);
    }
    else
    {
        LogMessageToDsLog("*** Failed to allocate memory while formatting log message ***");
    }
}
#endif

void LogMessage(const char *Format, ...)
{
    va_list args;

    va_start(args, Format);
    LogMessageV(Format, args);
    va_end(args);
}


static char GetPrintableChar(char x)
{
    return isprint(x) ? x : '.';
}


#define BYTES_PER_LINE 16

void LogBuffer(void* Buffer, int Length)
{
    unsigned char* buffer = (unsigned char*) Buffer;
    const int bytesPerLine = BYTES_PER_LINE;
    char outBuffer[10 + BYTES_PER_LINE * 3 + 3 + BYTES_PER_LINE + 1 + 1];
    int outIndex;
    int bufferIndex;
    int lineIndex;
    int lineLength;
    int i;

    LOG("BUFFER: @ %p => { Length = %d }", Buffer, Length);

    // process each line
    for (bufferIndex = 0; bufferIndex < Length; bufferIndex += bytesPerLine)
    {
        // initialize output buffer for the line
        outIndex = 0;
        outBuffer[0] = 0;

        // figure out whether we have a partial line
        lineLength = LWE_MIN(bytesPerLine, Length - bufferIndex);

        // 10 characters for offset info
        outIndex += snprintf(outBuffer + outIndex, sizeof(outBuffer)-outIndex, "%08x: ", bufferIndex);

        // 3 characters per byte in the line
        for (lineIndex = 0; lineIndex < lineLength; lineIndex++)
        {
            outIndex += snprintf(outBuffer + outIndex, sizeof(outBuffer)-outIndex, " %02x", buffer[bufferIndex + lineIndex]);
        }

        // pad the last line (3 characters per missing byte)
        for (i = 0; i < (bytesPerLine - lineLength); i++)
        {
            outIndex += snprintf(outBuffer + outIndex, sizeof(outBuffer)-outIndex, "   ");
        }

        // add 2 spaces and '[' to separate hex from chars
        outIndex += snprintf(outBuffer + outIndex, sizeof(outBuffer)-outIndex, "  [");

        // 1 character per byte in the line
        for (lineIndex = 0; lineIndex < lineLength; lineIndex++)
        {
            outIndex += snprintf(outBuffer + outIndex, sizeof(outBuffer)-outIndex, "%c", GetPrintableChar(buffer[bufferIndex + lineIndex]));
        }

        // add final ']'
        outIndex += snprintf(outBuffer + outIndex, sizeof(outBuffer)-outIndex, "]");

        outBuffer[sizeof(outBuffer)-1] = 0;

        LOG("%s", outBuffer);
    }
}

#define CASE_RETURN_STRING(token)               \
    case token:                                 \
    return #token

#define DEFAULT_RETURN_UNKNOWN_STRING()         \
    default:                                    \
    return "*UNKNOWN*"

const char* StateToString(unsigned long State)
{
    switch (State)
    {
        CASE_RETURN_STRING(kUnknownState);
        CASE_RETURN_STRING(kActive);
        CASE_RETURN_STRING(kInactive);
        CASE_RETURN_STRING(kInitialized);
        CASE_RETURN_STRING(kUninitialized);
        CASE_RETURN_STRING(kFailedToInit);
        DEFAULT_RETURN_UNKNOWN_STRING();
    }
}

const char* TypeToString(unsigned long Type)
{
    switch (Type)
    {
        CASE_RETURN_STRING(kDSPlugInCallsBegin);
        CASE_RETURN_STRING(kReleaseContinueData);
        CASE_RETURN_STRING(kOpenDirNode);
        CASE_RETURN_STRING(kCloseDirNode);
        CASE_RETURN_STRING(kGetDirNodeInfo);
        CASE_RETURN_STRING(kGetRecordList);
        CASE_RETURN_STRING(kGetRecordEntry);
        CASE_RETURN_STRING(kGetAttributeEntry);
        CASE_RETURN_STRING(kGetAttributeValue);
        CASE_RETURN_STRING(kOpenRecord);
        CASE_RETURN_STRING(kGetRecordReferenceInfo);
        CASE_RETURN_STRING(kGetRecordAttributeInfo);
        CASE_RETURN_STRING(kGetRecordAttributeValueByID);
        CASE_RETURN_STRING(kGetRecordAttributeValueByIndex);
        CASE_RETURN_STRING(kFlushRecord);
        CASE_RETURN_STRING(kCloseRecord);
        CASE_RETURN_STRING(kSetRecordName);
        CASE_RETURN_STRING(kSetRecordType);
        CASE_RETURN_STRING(kDeleteRecord);
        CASE_RETURN_STRING(kCreateRecord);
        CASE_RETURN_STRING(kCreateRecordAndOpen);
        CASE_RETURN_STRING(kAddAttribute);
        CASE_RETURN_STRING(kRemoveAttribute);
        CASE_RETURN_STRING(kAddAttributeValue);
        CASE_RETURN_STRING(kRemoveAttributeValue);
        CASE_RETURN_STRING(kSetAttributeValue);
        CASE_RETURN_STRING(kDoDirNodeAuth);
        CASE_RETURN_STRING(kDoAttributeValueSearch);
        CASE_RETURN_STRING(kDoAttributeValueSearchWithData);
        CASE_RETURN_STRING(kDoPlugInCustomCall);
        CASE_RETURN_STRING(kCloseAttributeList);
        CASE_RETURN_STRING(kCloseAttributeValueList);
        CASE_RETURN_STRING(kHandleNetworkTransition);
        CASE_RETURN_STRING(kServerRunLoop);
        CASE_RETURN_STRING(kDoDirNodeAuthOnRecordType);
        CASE_RETURN_STRING(kCheckNIAutoSwitch);
        CASE_RETURN_STRING(kGetRecordAttributeValueByValue);
        CASE_RETURN_STRING(kDoMultipleAttributeValueSearch);
        CASE_RETURN_STRING(kDoMultipleAttributeValueSearchWithData);
        CASE_RETURN_STRING(kSetAttributeValues);
        CASE_RETURN_STRING(kKerberosMutex);
        CASE_RETURN_STRING(kHandleSystemWillSleep);
        CASE_RETURN_STRING(kHandleSystemWillPowerOn);
        CASE_RETURN_STRING(kDSPlugInCallsEnd);
        DEFAULT_RETURN_UNKNOWN_STRING();
    }
}

const char* MacErrorToString(long MacError)
{
    switch (MacError)
    {
        CASE_RETURN_STRING(eDSNoErr);
        CASE_RETURN_STRING(eDSOpenFailed);
        CASE_RETURN_STRING(eDSCloseFailed);
        CASE_RETURN_STRING(eDSOpenNodeFailed);
        CASE_RETURN_STRING(eDSBadDirRefences);
        CASE_RETURN_STRING(eDSNullRecordReference);
        CASE_RETURN_STRING(eDSMaxSessionsOpen);
        CASE_RETURN_STRING(eDSCannotAccessSession);
        CASE_RETURN_STRING(eDSDirSrvcNotOpened);
        CASE_RETURN_STRING(eDSNodeNotFound);
        CASE_RETURN_STRING(eDSUnknownNodeName);
        CASE_RETURN_STRING(eDSRegisterCustomFailed);
        CASE_RETURN_STRING(eDSGetCustomFailed);
        CASE_RETURN_STRING(eDSUnRegisterFailed);
        CASE_RETURN_STRING(eDSAllocationFailed);
        CASE_RETURN_STRING(eDSDeAllocateFailed);
        CASE_RETURN_STRING(eDSCustomBlockFailed);
        CASE_RETURN_STRING(eDSCustomUnblockFailed);
        CASE_RETURN_STRING(eDSCustomYieldFailed);
        CASE_RETURN_STRING(eDSCorruptBuffer);
        CASE_RETURN_STRING(eDSInvalidIndex);
        CASE_RETURN_STRING(eDSIndexOutOfRange);
        CASE_RETURN_STRING(eDSIndexNotFound);
        CASE_RETURN_STRING(eDSCorruptRecEntryData);
        CASE_RETURN_STRING(eDSRefSpaceFull);
        CASE_RETURN_STRING(eDSRefTableAllocError);
        CASE_RETURN_STRING(eDSInvalidReference);
        CASE_RETURN_STRING(eDSInvalidRefType);
        CASE_RETURN_STRING(eDSInvalidDirRef);
        CASE_RETURN_STRING(eDSInvalidNodeRef);
        CASE_RETURN_STRING(eDSInvalidRecordRef);
        CASE_RETURN_STRING(eDSInvalidAttrListRef);
        CASE_RETURN_STRING(eDSInvalidAttrValueRef);
        CASE_RETURN_STRING(eDSInvalidContinueData);
        CASE_RETURN_STRING(eDSInvalidBuffFormat);
        CASE_RETURN_STRING(eDSInvalidPatternMatchType);
        CASE_RETURN_STRING(eDSRefTableError);
        CASE_RETURN_STRING(eDSRefTableNilError);
        CASE_RETURN_STRING(eDSRefTableIndexOutOfBoundsError);
        CASE_RETURN_STRING(eDSRefTableEntryNilError);
        CASE_RETURN_STRING(eDSRefTableCSBPAllocError);
        CASE_RETURN_STRING(eDSRefTableFWAllocError);
        CASE_RETURN_STRING(eDSAuthFailed);
        CASE_RETURN_STRING(eDSAuthMethodNotSupported);
        CASE_RETURN_STRING(eDSAuthResponseBufTooSmall);
        CASE_RETURN_STRING(eDSAuthParameterError);
        CASE_RETURN_STRING(eDSAuthInBuffFormatError);
        CASE_RETURN_STRING(eDSAuthNoSuchEntity);
        CASE_RETURN_STRING(eDSAuthBadPassword);
        CASE_RETURN_STRING(eDSAuthContinueDataBad);
        CASE_RETURN_STRING(eDSAuthUnknownUser);
        CASE_RETURN_STRING(eDSAuthInvalidUserName);
        CASE_RETURN_STRING(eDSAuthCannotRecoverPasswd);
        CASE_RETURN_STRING(eDSAuthFailedClearTextOnly);
        CASE_RETURN_STRING(eDSAuthNoAuthServerFound);
        CASE_RETURN_STRING(eDSAuthServerError);
        CASE_RETURN_STRING(eDSInvalidContext);
        CASE_RETURN_STRING(eDSBadContextData);
        CASE_RETURN_STRING(eDSPermissionError);
        CASE_RETURN_STRING(eDSReadOnly);
        CASE_RETURN_STRING(eDSInvalidDomain);
        CASE_RETURN_STRING(eNetInfoError);
        CASE_RETURN_STRING(eDSInvalidRecordType);
        CASE_RETURN_STRING(eDSInvalidAttributeType);
        CASE_RETURN_STRING(eDSInvalidRecordName);
        CASE_RETURN_STRING(eDSAttributeNotFound);
        CASE_RETURN_STRING(eDSRecordAlreadyExists);
        CASE_RETURN_STRING(eDSRecordNotFound);
        CASE_RETURN_STRING(eDSAttributeDoesNotExist);
        CASE_RETURN_STRING(eDSNoStdMappingAvailable);
        CASE_RETURN_STRING(eDSInvalidNativeMapping);
        CASE_RETURN_STRING(eDSSchemaError);
        CASE_RETURN_STRING(eDSAttributeValueNotFound);
        CASE_RETURN_STRING(eDSVersionMismatch);
        CASE_RETURN_STRING(eDSPlugInConfigFileError);
        CASE_RETURN_STRING(eDSInvalidPlugInConfigData);
        CASE_RETURN_STRING(eDSAuthNewPasswordRequired);
        CASE_RETURN_STRING(eDSAuthPasswordExpired);
        CASE_RETURN_STRING(eDSAuthPasswordQualityCheckFailed);
        CASE_RETURN_STRING(eDSAuthAccountDisabled);
        CASE_RETURN_STRING(eDSAuthAccountExpired);
        CASE_RETURN_STRING(eDSAuthAccountInactive);
        CASE_RETURN_STRING(eDSAuthPasswordTooShort);
        CASE_RETURN_STRING(eDSAuthPasswordTooLong);
        CASE_RETURN_STRING(eDSAuthPasswordNeedsLetter);
        CASE_RETURN_STRING(eDSAuthPasswordNeedsDigit);
        CASE_RETURN_STRING(eDSAuthPasswordChangeTooSoon);
        CASE_RETURN_STRING(eDSAuthInvalidLogonHours);
        CASE_RETURN_STRING(eDSAuthInvalidComputer);
        CASE_RETURN_STRING(eDSAuthMasterUnreachable);
        CASE_RETURN_STRING(eDSNullParameter);
        CASE_RETURN_STRING(eDSNullDataBuff);
        CASE_RETURN_STRING(eDSNullNodeName);
        CASE_RETURN_STRING(eDSNullRecEntryPtr);
        CASE_RETURN_STRING(eDSNullRecName);
        CASE_RETURN_STRING(eDSNullRecNameList);
        CASE_RETURN_STRING(eDSNullRecType);
        CASE_RETURN_STRING(eDSNullRecTypeList);
        CASE_RETURN_STRING(eDSNullAttribute);
        CASE_RETURN_STRING(eDSNullAttributeAccess);
        CASE_RETURN_STRING(eDSNullAttributeValue);
        CASE_RETURN_STRING(eDSNullAttributeType);
        CASE_RETURN_STRING(eDSNullAttributeTypeList);
        CASE_RETURN_STRING(eDSNullAttributeControlPtr);
        CASE_RETURN_STRING(eDSNullAttributeRequestList);
        CASE_RETURN_STRING(eDSNullDataList);
        CASE_RETURN_STRING(eDSNullDirNodeTypeList);
        CASE_RETURN_STRING(eDSNullAutMethod);
        CASE_RETURN_STRING(eDSNullAuthStepData);
        CASE_RETURN_STRING(eDSNullAuthStepDataResp);
        CASE_RETURN_STRING(eDSNullNodeInfoTypeList);
        CASE_RETURN_STRING(eDSNullPatternMatch);
        CASE_RETURN_STRING(eDSNullNodeNamePattern);
        CASE_RETURN_STRING(eDSNullTargetArgument);
        CASE_RETURN_STRING(eDSEmptyParameter);
        CASE_RETURN_STRING(eDSEmptyBuffer);
        CASE_RETURN_STRING(eDSEmptyNodeName);
        CASE_RETURN_STRING(eDSEmptyRecordName);
        CASE_RETURN_STRING(eDSEmptyRecordNameList);
        CASE_RETURN_STRING(eDSEmptyRecordType);
        CASE_RETURN_STRING(eDSEmptyRecordTypeList);
        CASE_RETURN_STRING(eDSEmptyRecordEntry);
        CASE_RETURN_STRING(eDSEmptyPatternMatch);
        CASE_RETURN_STRING(eDSEmptyNodeNamePattern);
        CASE_RETURN_STRING(eDSEmptyAttribute);
        CASE_RETURN_STRING(eDSEmptyAttributeType);
        CASE_RETURN_STRING(eDSEmptyAttributeTypeList);
        CASE_RETURN_STRING(eDSEmptyAttributeValue);
        CASE_RETURN_STRING(eDSEmptyAttributeRequestList);
        CASE_RETURN_STRING(eDSEmptyDataList);
        CASE_RETURN_STRING(eDSEmptyNodeInfoTypeList);
        CASE_RETURN_STRING(eDSEmptyAuthMethod);
        CASE_RETURN_STRING(eDSEmptyAuthStepData);
        CASE_RETURN_STRING(eDSEmptyAuthStepDataResp);
        CASE_RETURN_STRING(eDSEmptyPattern2Match);
        CASE_RETURN_STRING(eDSBadDataNodeLength);
        CASE_RETURN_STRING(eDSBadDataNodeFormat);
        CASE_RETURN_STRING(eDSBadSourceDataNode);
        CASE_RETURN_STRING(eDSBadTargetDataNode);
        CASE_RETURN_STRING(eDSBufferTooSmall);
        CASE_RETURN_STRING(eDSUnknownMatchType);
        CASE_RETURN_STRING(eDSUnSupportedMatchType);
        CASE_RETURN_STRING(eDSInvalDataList);
        CASE_RETURN_STRING(eDSAttrListError);
        CASE_RETURN_STRING(eServerNotRunning);
        CASE_RETURN_STRING(eUnknownAPICall);
        CASE_RETURN_STRING(eUnknownServerError);
        CASE_RETURN_STRING(eUnknownPlugIn);
        CASE_RETURN_STRING(ePlugInDataError);
        CASE_RETURN_STRING(ePlugInNotFound);
        CASE_RETURN_STRING(ePlugInError);
        CASE_RETURN_STRING(ePlugInInitError);
        CASE_RETURN_STRING(ePlugInNotActive);
        CASE_RETURN_STRING(ePlugInFailedToInitialize);
        CASE_RETURN_STRING(ePlugInCallTimedOut);
        CASE_RETURN_STRING(eNoSearchNodesFound);
        CASE_RETURN_STRING(eSearchPathNotDefined);
        CASE_RETURN_STRING(eNotHandledByThisNode);
        CASE_RETURN_STRING(eIPCSendError);
        CASE_RETURN_STRING(eIPCReceiveError);
        CASE_RETURN_STRING(eServerReplyError);
        CASE_RETURN_STRING(eDSTCPSendError);
        CASE_RETURN_STRING(eDSTCPReceiveError);
        CASE_RETURN_STRING(eDSTCPVersionMismatch);
        CASE_RETURN_STRING(eDSIPUnreachable);
        CASE_RETURN_STRING(eDSUnknownHost);
        CASE_RETURN_STRING(ePluginHandlerNotLoaded);
        CASE_RETURN_STRING(eNoPluginsLoaded);
        CASE_RETURN_STRING(ePluginAlreadyLoaded);
        CASE_RETURN_STRING(ePluginVersionNotFound);
        CASE_RETURN_STRING(ePluginNameNotFound);
        CASE_RETURN_STRING(eNoPluginFactoriesFound);
        CASE_RETURN_STRING(ePluginConfigAvailNotFound);
        CASE_RETURN_STRING(ePluginConfigFileNotFound);
        CASE_RETURN_STRING(eCFMGetFileSysRepErr);
        CASE_RETURN_STRING(eCFPlugInGetBundleErr);
        CASE_RETURN_STRING(eCFBndleGetInfoDictErr);
        CASE_RETURN_STRING(eCFDictGetValueErr);
        CASE_RETURN_STRING(eDSServerTimeout);
        CASE_RETURN_STRING(eDSContinue);
        CASE_RETURN_STRING(eDSInvalidHandle);
        CASE_RETURN_STRING(eDSSendFailed);
        CASE_RETURN_STRING(eDSReceiveFailed);
        CASE_RETURN_STRING(eDSBadPacket);
        CASE_RETURN_STRING(eDSInvalidTag);
        CASE_RETURN_STRING(eDSInvalidSession);
        CASE_RETURN_STRING(eDSInvalidName);
        CASE_RETURN_STRING(eDSUserUnknown);
        CASE_RETURN_STRING(eDSUnrecoverablePassword);
        CASE_RETURN_STRING(eDSAuthenticationFailed);
        CASE_RETURN_STRING(eDSBogusServer);
        CASE_RETURN_STRING(eDSOperationFailed);
        CASE_RETURN_STRING(eDSNotAuthorized);
        CASE_RETURN_STRING(eDSNetInfoError);
        CASE_RETURN_STRING(eDSContactMaster);
        CASE_RETURN_STRING(eDSServiceUnavailable);
        CASE_RETURN_STRING(eFWGetDirNodeNameErr1);
        CASE_RETURN_STRING(eFWGetDirNodeNameErr2);
        CASE_RETURN_STRING(eFWGetDirNodeNameErr3);
        CASE_RETURN_STRING(eFWGetDirNodeNameErr4);
        CASE_RETURN_STRING(eParameterSendError);
        CASE_RETURN_STRING(eParameterReceiveError);
        CASE_RETURN_STRING(eServerSendError);
        CASE_RETURN_STRING(eServerReceiveError);
        CASE_RETURN_STRING(eMemoryError);
        CASE_RETURN_STRING(eMemoryAllocError);
        CASE_RETURN_STRING(eServerError);
        CASE_RETURN_STRING(eParameterError);
        CASE_RETURN_STRING(eDataReceiveErr_NoDirRef);
        CASE_RETURN_STRING(eDataReceiveErr_NoRecRef);
        CASE_RETURN_STRING(eDataReceiveErr_NoAttrListRef);
        CASE_RETURN_STRING(eDataReceiveErr_NoAttrValueListRef);
        CASE_RETURN_STRING(eDataReceiveErr_NoAttrEntry);
        CASE_RETURN_STRING(eDataReceiveErr_NoAttrValueEntry);
        CASE_RETURN_STRING(eDataReceiveErr_NoNodeCount);
        CASE_RETURN_STRING(eDataReceiveErr_NoAttrCount);
        CASE_RETURN_STRING(eDataReceiveErr_NoRecEntry);
        CASE_RETURN_STRING(eDataReceiveErr_NoRecEntryCount);
        CASE_RETURN_STRING(eDataReceiveErr_NoRecMatchCount);
        CASE_RETURN_STRING(eDataReceiveErr_NoDataBuff);
        CASE_RETURN_STRING(eDataReceiveErr_NoContinueData);
        CASE_RETURN_STRING(eDataReceiveErr_NoNodeChangeToken);
        CASE_RETURN_STRING(eNoLongerSupported);
        CASE_RETURN_STRING(eUndefinedError);
        CASE_RETURN_STRING(eNotYetImplemented);
        CASE_RETURN_STRING(eDSLastValue);
        /* Begin System errors */
        CASE_RETURN_STRING(EPERM);
        CASE_RETURN_STRING(ENOENT);
        //CASE_RETURN_STRING(ESRCH);
        CASE_RETURN_STRING(EINTR);
        CASE_RETURN_STRING(EIO);
        CASE_RETURN_STRING(ENXIO);
        CASE_RETURN_STRING(E2BIG);
        CASE_RETURN_STRING(ENOEXEC);
        CASE_RETURN_STRING(EBADF);
        CASE_RETURN_STRING(ECHILD);
        //CASE_RETURN_STRING(EDEADLK);
        CASE_RETURN_STRING(ENOMEM);
        CASE_RETURN_STRING(EACCES);
        CASE_RETURN_STRING(EFAULT);
        CASE_RETURN_STRING(ENOTBLK);
        CASE_RETURN_STRING(EBUSY);
        CASE_RETURN_STRING(EEXIST);
        CASE_RETURN_STRING(EXDEV);
        CASE_RETURN_STRING(ENODEV);
        CASE_RETURN_STRING(ENOTDIR);
        CASE_RETURN_STRING(EISDIR);
        CASE_RETURN_STRING(EINVAL);
        CASE_RETURN_STRING(ENFILE);
        CASE_RETURN_STRING(ENOTTY);
        CASE_RETURN_STRING(ETXTBSY);
        CASE_RETURN_STRING(EFBIG);
        CASE_RETURN_STRING(ENOSPC);
        //CASE_RETURN_STRING(ESPIPE);
        CASE_RETURN_STRING(EROFS);
        CASE_RETURN_STRING(EMLINK);
        CASE_RETURN_STRING(EPIPE);
        CASE_RETURN_STRING(EDOM);
        CASE_RETURN_STRING(ERANGE);
        CASE_RETURN_STRING(EAGAIN);
        CASE_RETURN_STRING(EINPROGRESS);
        CASE_RETURN_STRING(EALREADY);
        CASE_RETURN_STRING(ENOTSOCK);
        CASE_RETURN_STRING(EDESTADDRREQ);
        CASE_RETURN_STRING(EMSGSIZE);
        CASE_RETURN_STRING(EPROTOTYPE);
        CASE_RETURN_STRING(ENOPROTOOPT);
        CASE_RETURN_STRING(EPROTONOSUPPORT);
        CASE_RETURN_STRING(ESOCKTNOSUPPORT);
        CASE_RETURN_STRING(ENOTSUP);
        CASE_RETURN_STRING(EPFNOSUPPORT);
        CASE_RETURN_STRING(EAFNOSUPPORT);
        CASE_RETURN_STRING(EADDRINUSE);
        CASE_RETURN_STRING(EADDRNOTAVAIL);
        CASE_RETURN_STRING(ENETDOWN);
        CASE_RETURN_STRING(ENETUNREACH);
        CASE_RETURN_STRING(ENETRESET);
        CASE_RETURN_STRING(ECONNABORTED);
        CASE_RETURN_STRING(ECONNRESET);
        CASE_RETURN_STRING(ENOBUFS);
        CASE_RETURN_STRING(EISCONN);
        CASE_RETURN_STRING(ENOTCONN);
        CASE_RETURN_STRING(ESHUTDOWN);
        CASE_RETURN_STRING(ETOOMANYREFS);
        CASE_RETURN_STRING(ETIMEDOUT);
        CASE_RETURN_STRING(ECONNREFUSED);
        CASE_RETURN_STRING(ELOOP);
        CASE_RETURN_STRING(ENAMETOOLONG);
        CASE_RETURN_STRING(EHOSTDOWN);
        CASE_RETURN_STRING(EHOSTUNREACH);
        CASE_RETURN_STRING(ENOTEMPTY);
        CASE_RETURN_STRING(EPROCLIM);
        CASE_RETURN_STRING(EUSERS);
        CASE_RETURN_STRING(EDQUOT);
        CASE_RETURN_STRING(ESTALE);
        CASE_RETURN_STRING(EREMOTE);
        CASE_RETURN_STRING(EBADRPC);
        CASE_RETURN_STRING(ERPCMISMATCH);
        CASE_RETURN_STRING(EPROGUNAVAIL);
        CASE_RETURN_STRING(EPROGMISMATCH);
        CASE_RETURN_STRING(EPROCUNAVAIL);
        CASE_RETURN_STRING(ENOLCK);
        CASE_RETURN_STRING(ENOSYS);
        CASE_RETURN_STRING(EFTYPE);
        CASE_RETURN_STRING(EAUTH);
        CASE_RETURN_STRING(ENEEDAUTH);
        CASE_RETURN_STRING(EPWROFF);
        CASE_RETURN_STRING(EDEVERR);
        CASE_RETURN_STRING(EOVERFLOW);
        CASE_RETURN_STRING(EBADEXEC);
        CASE_RETURN_STRING(EBADARCH);
        //CASE_RETURN_STRING(ESHLIBVERS);
        CASE_RETURN_STRING(EBADMACHO);
        CASE_RETURN_STRING(ECANCELED);
        CASE_RETURN_STRING(EIDRM);
        CASE_RETURN_STRING(ENOMSG);
        CASE_RETURN_STRING(EILSEQ);
        CASE_RETURN_STRING(ENOATTR);
        CASE_RETURN_STRING(EBADMSG);
        CASE_RETURN_STRING(EMULTIHOP);
        CASE_RETURN_STRING(ENODATA);
        CASE_RETURN_STRING(ENOLINK);
        CASE_RETURN_STRING(ENOSR);
        CASE_RETURN_STRING(ENOSTR);
        CASE_RETURN_STRING(EPROTO);
        CASE_RETURN_STRING(ETIME);
        /* End System Errors */
        /* Begin macadutil errors */
        CASE_RETURN_STRING(MAC_AD_ERROR_NOT_IMPLEMENTED);
        CASE_RETURN_STRING(MAC_AD_ERROR_INVALID_PARAMETER);
        CASE_RETURN_STRING(MAC_AD_ERROR_NOT_SUPPORTED);
        CASE_RETURN_STRING(MAC_AD_ERROR_LOGON_FAILURE);
        CASE_RETURN_STRING(MAC_AD_ERROR_INVALID_NAME);
        CASE_RETURN_STRING(MAC_AD_ERROR_UPN_NOT_FOUND);
        CASE_RETURN_STRING(MAC_AD_ERROR_NULL_PARAMETER);
        CASE_RETURN_STRING(MAC_AD_ERROR_INVALID_TAG);
        CASE_RETURN_STRING(MAC_AD_ERROR_NO_SUCH_ATTRIBUTE);
        CASE_RETURN_STRING(MAC_AD_ERROR_INVALID_RECORD_TYPE);
        CASE_RETURN_STRING(MAC_AD_ERROR_INVALID_ATTRIBUTE_TYPE);
        CASE_RETURN_STRING(MAC_AD_ERROR_INSUFFICIENT_BUFFER);
        CASE_RETURN_STRING(MAC_AD_ERROR_IPC_FAILED);
        CASE_RETURN_STRING(MAC_AD_ERROR_NO_PROC_STATUS);
        CASE_RETURN_STRING(MAC_AD_ERROR_KRB5_PASSWORD_MISMATCH);
        CASE_RETURN_STRING(MAC_AD_ERROR_KRB5_PASSWORD_EXPIRED);
        CASE_RETURN_STRING(MAC_AD_ERROR_KRB5_CLOCK_SKEW);
        CASE_RETURN_STRING(MAC_AD_ERROR_KRB5_ERROR);
        CASE_RETURN_STRING(MAC_AD_ERROR_GSS_API_FAILED);
        CASE_RETURN_STRING(MAC_AD_ERROR_NO_SUCH_POLICY);
        CASE_RETURN_STRING(MAC_AD_ERROR_LDAP_OPEN);
        CASE_RETURN_STRING(MAC_AD_ERROR_LDAP_SET_OPTION);
        CASE_RETURN_STRING(MAC_AD_ERROR_LDAP_QUERY_FAILED);
        /* End macadutil errors */
        /* Begin LSASS errors */
        CASE_RETURN_STRING(LW_ERROR_INVALID_CACHE_PATH);
        CASE_RETURN_STRING(LW_ERROR_INVALID_CONFIG_PATH);
        CASE_RETURN_STRING(LW_ERROR_INVALID_PREFIX_PATH);
        CASE_RETURN_STRING(LW_ERROR_INSUFFICIENT_BUFFER);
        CASE_RETURN_STRING(LW_ERROR_OUT_OF_MEMORY);
        CASE_RETURN_STRING(LW_ERROR_INVALID_MESSAGE);
        CASE_RETURN_STRING(LW_ERROR_UNEXPECTED_MESSAGE);
        CASE_RETURN_STRING(LW_ERROR_NO_SUCH_USER);
        CASE_RETURN_STRING(LW_ERROR_DATA_ERROR);
        CASE_RETURN_STRING(LW_ERROR_NOT_IMPLEMENTED);
        CASE_RETURN_STRING(LW_ERROR_NO_CONTEXT_ITEM);
        CASE_RETURN_STRING(LW_ERROR_NO_SUCH_GROUP);
        CASE_RETURN_STRING(LW_ERROR_REGEX_COMPILE_FAILED);
        CASE_RETURN_STRING(LW_ERROR_NSS_EDIT_FAILED);
        CASE_RETURN_STRING(LW_ERROR_NO_HANDLER);
        CASE_RETURN_STRING(LW_ERROR_INTERNAL);
        CASE_RETURN_STRING(LW_ERROR_NOT_HANDLED);
        CASE_RETURN_STRING(LW_ERROR_INVALID_DNS_RESPONSE);
        CASE_RETURN_STRING(LW_ERROR_DNS_RESOLUTION_FAILED);
        CASE_RETURN_STRING(LW_ERROR_FAILED_TIME_CONVERSION);
        CASE_RETURN_STRING(LW_ERROR_INVALID_SID);
        CASE_RETURN_STRING(LW_ERROR_PASSWORD_MISMATCH);
        CASE_RETURN_STRING(LW_ERROR_UNEXPECTED_DB_RESULT);
        CASE_RETURN_STRING(LW_ERROR_PASSWORD_EXPIRED);
        CASE_RETURN_STRING(LW_ERROR_ACCOUNT_EXPIRED);
        CASE_RETURN_STRING(LW_ERROR_USER_EXISTS);
        CASE_RETURN_STRING(LW_ERROR_GROUP_EXISTS);
        CASE_RETURN_STRING(LW_ERROR_INVALID_GROUP_INFO_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_INVALID_USER_INFO_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_UNSUPPORTED_USER_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_UNSUPPORTED_GROUP_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_INVALID_LOGIN_ID);
        CASE_RETURN_STRING(LW_ERROR_INVALID_HOMEDIR);
        CASE_RETURN_STRING(LW_ERROR_INVALID_GROUP_NAME);
        CASE_RETURN_STRING(LW_ERROR_NO_MORE_GROUPS);
        CASE_RETURN_STRING(LW_ERROR_NO_MORE_USERS);
        CASE_RETURN_STRING(LW_ERROR_FAILED_ADD_USER);
        CASE_RETURN_STRING(LW_ERROR_FAILED_ADD_GROUP);
        CASE_RETURN_STRING(LW_ERROR_INVALID_LSA_CONNECTION);
        CASE_RETURN_STRING(LW_ERROR_INVALID_AUTH_PROVIDER);
        CASE_RETURN_STRING(LW_ERROR_INVALID_PARAMETER);
        CASE_RETURN_STRING(LW_ERROR_LDAP_NO_PARENT_DN);
        CASE_RETURN_STRING(LW_ERROR_LDAP_ERROR);
        CASE_RETURN_STRING(LW_ERROR_NO_SUCH_DOMAIN);
        CASE_RETURN_STRING(LW_ERROR_LDAP_FAILED_GETDN);
        CASE_RETURN_STRING(LW_ERROR_DUPLICATE_DOMAINNAME);
        CASE_RETURN_STRING(LW_ERROR_KRB5_CALL_FAILED);
        CASE_RETURN_STRING(LW_ERROR_GSS_CALL_FAILED);
        CASE_RETURN_STRING(LW_ERROR_FAILED_FIND_DC);
        CASE_RETURN_STRING(LW_ERROR_NO_SUCH_CELL);
        CASE_RETURN_STRING(LW_ERROR_GROUP_IN_USE);
        CASE_RETURN_STRING(LW_ERROR_FAILED_CREATE_HOMEDIR);
        CASE_RETURN_STRING(LW_ERROR_PASSWORD_TOO_WEAK);
        CASE_RETURN_STRING(LW_ERROR_INVALID_SID_REVISION);
        CASE_RETURN_STRING(LW_ERROR_ACCOUNT_LOCKED);
        CASE_RETURN_STRING(LW_ERROR_ACCOUNT_DISABLED);
        CASE_RETURN_STRING(LW_ERROR_USER_CANNOT_CHANGE_PASSWD);
        CASE_RETURN_STRING(LW_ERROR_LOAD_LIBRARY_FAILED);
        CASE_RETURN_STRING(LW_ERROR_LOOKUP_SYMBOL_FAILED);
        CASE_RETURN_STRING(LW_ERROR_INVALID_EVENTLOG);
        CASE_RETURN_STRING(LW_ERROR_INVALID_CONFIG);
        CASE_RETURN_STRING(LW_ERROR_UNEXPECTED_TOKEN);
        CASE_RETURN_STRING(LW_ERROR_LDAP_NO_RECORDS_FOUND);
        CASE_RETURN_STRING(LW_ERROR_DUPLICATE_USERNAME);
        CASE_RETURN_STRING(LW_ERROR_DUPLICATE_GROUPNAME);
        CASE_RETURN_STRING(LW_ERROR_DUPLICATE_CELLNAME);
        CASE_RETURN_STRING(LW_ERROR_STRING_CONV_FAILED);
        CASE_RETURN_STRING(LW_ERROR_INVALID_PASSWORD);
        CASE_RETURN_STRING(LW_ERROR_QUERY_CREATION_FAILED);
        CASE_RETURN_STRING(LW_ERROR_NO_SUCH_OBJECT);
        CASE_RETURN_STRING(LW_ERROR_DUPLICATE_USER_OR_GROUP);
        CASE_RETURN_STRING(LW_ERROR_INVALID_KRB5_CACHE_TYPE);
        CASE_RETURN_STRING(LW_ERROR_FAILED_TO_SET_TIME);
        CASE_RETURN_STRING(LW_ERROR_NO_NETBIOS_NAME);
        CASE_RETURN_STRING(LW_ERROR_INVALID_NETLOGON_RESPONSE);
        CASE_RETURN_STRING(LW_ERROR_INVALID_OBJECTGUID);
        CASE_RETURN_STRING(LW_ERROR_INVALID_DOMAIN);
        CASE_RETURN_STRING(LW_ERROR_NO_DEFAULT_REALM);
        CASE_RETURN_STRING(LW_ERROR_NOT_SUPPORTED);
        CASE_RETURN_STRING(LW_ERROR_LOGON_FAILURE);
        CASE_RETURN_STRING(LW_ERROR_NO_SITE_INFORMATION);
        CASE_RETURN_STRING(LW_ERROR_INVALID_LDAP_STRING);
        CASE_RETURN_STRING(LW_ERROR_INVALID_LDAP_ATTR_VALUE);
        CASE_RETURN_STRING(LW_ERROR_NULL_BUFFER);
        CASE_RETURN_STRING(LW_ERROR_CLOCK_SKEW);
        CASE_RETURN_STRING(LW_ERROR_KRB5_NO_KEYS_FOUND);
        CASE_RETURN_STRING(LW_ERROR_SERVICE_NOT_AVAILABLE);
        CASE_RETURN_STRING(LW_ERROR_INVALID_SERVICE_RESPONSE);
        CASE_RETURN_STRING(LW_ERROR_NSS_ERROR);
        CASE_RETURN_STRING(LW_ERROR_AUTH_ERROR);
        CASE_RETURN_STRING(LW_ERROR_INVALID_LDAP_DN);
        CASE_RETURN_STRING(LW_ERROR_NOT_MAPPED);
        CASE_RETURN_STRING(LW_ERROR_RPC_NETLOGON_FAILED);
        CASE_RETURN_STRING(LW_ERROR_ENUM_DOMAIN_TRUSTS_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_LSABINDING_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_OPENPOLICY_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_LSA_LOOKUPNAME2_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_SET_SESS_CREDS_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_REL_SESS_CREDS_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_CLOSEPOLICY_FAILED);
        CASE_RETURN_STRING(LW_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND);
        CASE_RETURN_STRING(LW_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES);
        CASE_RETURN_STRING(LW_ERROR_NO_TRUSTED_DOMAIN_FOUND);
        CASE_RETURN_STRING(LW_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS);
        CASE_RETURN_STRING(LW_ERROR_DCE_CALL_FAILED);
        CASE_RETURN_STRING(LW_ERROR_FAILED_TO_LOOKUP_DC);
        CASE_RETURN_STRING(LW_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_INVALID_USER_NAME);
        CASE_RETURN_STRING(LW_ERROR_INVALID_LOG_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_INVALID_METRIC_TYPE);
        CASE_RETURN_STRING(LW_ERROR_INVALID_METRIC_PACK);
        CASE_RETURN_STRING(LW_ERROR_INVALID_METRIC_INFO_LEVEL);
        CASE_RETURN_STRING(LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK);
        CASE_RETURN_STRING(LW_ERROR_MAC_FLUSH_DS_CACHE_FAILED);
        CASE_RETURN_STRING(LW_ERROR_LSA_SERVER_UNREACHABLE);
        CASE_RETURN_STRING(LW_ERROR_INVALID_NSS_ARTEFACT_TYPE);
        CASE_RETURN_STRING(LW_ERROR_INVALID_AGENT_VERSION);
        /* End LSASS errors */
        /* Begin Netlogon errors */
        CASE_RETURN_STRING(DNS_ERROR_BAD_PACKET);
        CASE_RETURN_STRING(ERROR_BAD_CONFIGURATION);
        CASE_RETURN_STRING(ERROR_BAD_DLL_ENTRYPOINT);
        CASE_RETURN_STRING(ERROR_BAD_FORMAT);
        CASE_RETURN_STRING(ERROR_DLL_INIT_FAILED);
        CASE_RETURN_STRING(ERROR_EVENTLOG_CANT_START);
        CASE_RETURN_STRING(ERROR_ILLEGAL_CHARACTER);
        CASE_RETURN_STRING(ERROR_INTERNAL_ERROR);
        CASE_RETURN_STRING(ERROR_INVALID_PARAMETER);
        CASE_RETURN_STRING(ERROR_INVALID_TIME);
        CASE_RETURN_STRING(ERROR_NO_SUCH_DOMAIN);
        CASE_RETURN_STRING(ERROR_NOT_FOUND);
        CASE_RETURN_STRING(ERROR_NOT_JOINED);
        CASE_RETURN_STRING(ERROR_PATH_NOT_FOUND);
        CASE_RETURN_STRING(ERROR_SERVICE_DEPENDENCY_FAIL);
        CASE_RETURN_STRING(ERROR_WRITE_FAULT);
        CASE_RETURN_STRING(NERR_DCNotFound);
        CASE_RETURN_STRING(NERR_SetupNotJoined);
        /* End Netlogon errors */

        DEFAULT_RETURN_UNKNOWN_STRING();
    }
}

long
LWCaptureOutput(
    char* pszCommand,
    char** ppszOutput
    )
{
    long macError = eDSNoErr;
    CHAR szBuf[1000];
    FILE* pFile = NULL;
 
    pFile = popen(pszCommand, "r");
    if (pFile == NULL) {
        macError = LwErrnoToWin32Error(errno);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
 
    while (TRUE) {
        if (NULL == fgets(szBuf, PATH_MAX, pFile)) {
            if (feof(pFile)) { 
                break;
            } else {
                macError = LwErrnoToWin32Error(errno);
                GOTO_CLEANUP_ON_MACERROR(macError);
            }
        }
        LwStripWhitespace(szBuf, TRUE, TRUE);
        if (!IsNullOrEmptyString(szBuf)) {
            macError = LwAllocateString(szBuf, ppszOutput);
            break; 
        }
 
    }

cleanup:

    if (pFile)
        pclose(pFile);

    return macError;
}

static
void
DoubleTheBufferSizeIfItsTooSmall(
    long *              pMacError, 
    tDirNodeReference   hDirRef, 
    tDataBufferPtr *    ppBuffer
)
    // This routine is designed to handle the case where a 
    // Open Directory routine returns eDSBufferTooSmall.  
    // If so, it doubles the size of the buffer, allowing the 
    // caller to retry the Open Directory routine with the 
    // large buffer.
    //
    // errPtr is a pointer to a Open Directory error.  
    // This routine does nothing unless that error is 
    // eDSBufferTooSmall.  In that case it frees the buffer 
    // referenced by *bufPtrPtr, replacing it with a buffer 
    // of twice the size.  It then leaves *errPtr set to 
    // eDSBufferTooSmall so that the caller retries the 
    // call with the larger buffer.
{
    long            macError = eDSNoErr;
    tDirStatus      junk;
    tDataBufferPtr  pBuffer = NULL;
    
    if (*pMacError == eDSBufferTooSmall)
    {
        // If the buffer size is already bigger than 16 MB, don't try to 
        // double it again; something has gone horribly wrong.
        if ( (*ppBuffer)->fBufferSize >= (16 * 1024 * 1024) ) {
            macError = eDSAllocationFailed;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }

        pBuffer = dsDataBufferAllocate(hDirRef, (*ppBuffer)->fBufferSize * 2);
        if (!pBuffer) {
            macError = eDSAllocationFailed;
            GOTO_CLEANUP_ON_MACERROR(macError);
        }
        
        junk = dsDataBufferDeAllocate(hDirRef, *ppBuffer);
        *ppBuffer = pBuffer;
    }
    
cleanup:

    // If err is eDSNoErr, the buffer expansion was successful 
    // so we leave *errPtr set to eDSBufferTooSmall.  If err 
    // is any other value, the expansion failed and we set 
    // *errPtr to that error.
        
    if (macError != eDSNoErr) {
        *pMacError = macError;
    }
}

static
long
dsFindDirNodes_Wrap(
    tDirReference       hDirRef,
    tDataBufferPtr *    ppDataBuffer,
    tDataListPtr        pNodeName,
    tDirPatternMatch    PatternMatchType,
    UInt32              *pulNodeCount,
    tContextData        *inOutContinueData
    )
    // A wrapper for dsFindDirNodes that handles two special cases:
    //
    // o If the routine returns eDSBufferTooSmall, it doubles the 
    //   size of the buffer referenced by *inOutDataBufferPtrPtr 
    //   and retries.
    //
    //   Note that this change requires a change of the function 
    //   prototype; the second parameter is a pointer to a pointer 
    //   to the buffer, rather than just a pointer to the buffer. 
    //   This is so that I can modify the client's buffer pointer.
    //
    // o If the routine returns no nodes but there's valid continue data, 
    //   it retries.
    //
    // In other respects this works just like dsFindDirNodes.
{
    long macError = eDSNoErr;
    
    do {
        do {
            macError = dsFindDirNodes(
                hDirRef, 
                *ppDataBuffer, 
                pNodeName, 
                PatternMatchType, 
                pulNodeCount, 
                inOutContinueData
            );
            DoubleTheBufferSizeIfItsTooSmall(&macError, hDirRef, ppDataBuffer);
        } while (macError == eDSBufferTooSmall);
    } while ( (macError == eDSNoErr) && (*pulNodeCount == 0) && (*inOutContinueData != 0) );

    return macError;
}

enum {
    kDefaultDSBufferSize = 1024
};

static 
long
GetLocalNodePathList(
    tDirReference  hDirRef,
    tDataListPtr * localNodePathListPtr
    )
    // Returns the path to the Open Directory local node. (/NetInfo/root/ or Local/Default/)
    // dirRef is the connection to Open Directory.
    // On success, *searchNodePathListPtr is a data list that 
    // contains the search node's path components.
{
    long                macError = eDSNoErr;
    tDirStatus          junk;
    tDataBufferPtr      pDataBuffer = NULL;
    tDirPatternMatch    patternToFind = eDSLocalNodeNames;
    UInt32              ulNodeCount = 0;
    tContextData        context = NULL;
    
    // Allocate a buffer for the node find results.  We'll grow 
    // this buffer if it proves to be to small.
    
    pDataBuffer = dsDataBufferAllocate(hDirRef, kDefaultDSBufferSize);
    if (!pDataBuffer) {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    // Find the node.  Note that this is a degenerate case because 
    // we're only looking for a single node, the local node, so 
    // we don't need to loop calling dsFindDirNodes, which is the 
    // standard way of using dsFindDirNodes.
    
    macError = dsFindDirNodes_Wrap(
            hDirRef, 
            &pDataBuffer,                       // place results here
            NULL,                               // no pattern, rather...
            patternToFind,                      // ... hardwired search type
            &ulNodeCount, 
            &context
        );
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    // If we didn't find any nodes, that's bad.
    
    if (ulNodeCount < 1) {
        macError = eDSNodeNotFound;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    // Grab the first node from the buffer.  Note that the inDirNodeIndex 
    // parameter to dsGetDirNodeName is one-based, so we pass in the constant 
    // 1.
    // 
    // Also, if we found more than one, that's unusual, but not enough to 
    // cause us to error.
    macError = dsGetDirNodeName(hDirRef, pDataBuffer, 1, localNodePathListPtr);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
cleanup:

    // Clean up.
    
    if (context != 0)
    {
        junk = dsReleaseContinueData(hDirRef, context);
    }
    
    if (pDataBuffer)
    {
        junk = dsDataBufferDeAllocate(hDirRef, pDataBuffer);
    }
    
    return macError;
}

BOOLEAN
LWIsUserInLocalGroup(
    char* pszUsername,
    const char* pszGroupname
    )
{
    long                macError = eDSNoErr;
    tDirStatus          junk;
    tDirReference       hDirRef = NULL;
    tDirNodeReference   hNodeRef = NULL;
    tRecordReference    hRecordRef = NULL;
    tDataNodePtr        pRecordName = NULL;
    tDataNodePtr        pUsernameToFind = NULL;
    tDataListPtr        pathListToLocalNode = NULL;
    tDataNodePtr        pRecordTypeGroup = NULL;
    tDataNodePtr        pAttrTypeGroupMemberList = NULL;
    tAttributeValueEntryPtr pValueEntry = NULL;
    BOOLEAN             bFound = FALSE;

    macError = dsOpenDirService(&hDirRef);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecordName = dsDataNodeAllocateString(hDirRef, pszGroupname);
    if (!pRecordName)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    pUsernameToFind = dsDataNodeAllocateString(hDirRef, pszUsername);
    if (!pUsernameToFind)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    macError = GetLocalNodePathList(hDirRef, &pathListToLocalNode);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    macError = dsOpenDirNode(hDirRef, pathListToLocalNode, &hNodeRef);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecordTypeGroup = dsDataNodeAllocateString(hDirRef, kDSStdRecordTypeGroups);
    if (!pRecordTypeGroup)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    pAttrTypeGroupMemberList = dsDataNodeAllocateString(hDirRef, kDSNAttrGroupMembership);
    if (!pAttrTypeGroupMemberList)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    macError = dsOpenRecord(hNodeRef,
                            pRecordTypeGroup,
                            pRecordName,
                            &hRecordRef);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    macError = dsGetRecordAttributeValueByValue(hRecordRef,
                                                pAttrTypeGroupMemberList,
                                                pUsernameToFind,
                                                &pValueEntry);
    if (macError)
    {
        goto cleanup;
    }
    
    if (pValueEntry)
    {
        bFound = TRUE;
    }

cleanup:

    if (pValueEntry)
    {
        junk = dsDeallocAttributeValueEntry(hDirRef, pValueEntry);
    }
        
    if (pRecordName)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pRecordName);
    }
    
    if (pathListToLocalNode)
    {
        junk = dsDataListDeallocate(hDirRef, pathListToLocalNode);
    }
    
	if (pRecordTypeGroup)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pRecordTypeGroup);
    }
    
	if (pAttrTypeGroupMemberList)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pAttrTypeGroupMemberList);
    }

    if (pUsernameToFind)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pUsernameToFind);
    }
    
    if (hRecordRef)
    {
        dsCloseRecord(hRecordRef);
    }
    
    if (hNodeRef)
    {
        dsCloseDirNode(hNodeRef);
    }

    if (hDirRef)
    {
        dsCloseDirService(hDirRef);
    }
    
    return bFound;
}

long
LWRemoveUserFromLocalGroup(
    char* pszUsername,
    const char* pszGroupname
    )
{
    long                macError = eDSNoErr;
    tDirStatus          junk;
    tDirReference       hDirRef = NULL;
    tDirNodeReference   hNodeRef = NULL;
    tRecordReference    hRecordRef = NULL;
    tDataNodePtr        pRecordName = NULL;
    tDataNodePtr        pUsernameToRemove = NULL;
    tDataListPtr        pathListToLocalNode = NULL;
    tDataNodePtr        pRecordTypeGroup = NULL;
    tDataNodePtr        pAttrTypeGroupMemberList = NULL;
    tAttributeValueEntryPtr pValueEntry = NULL;

    macError = dsOpenDirService(&hDirRef);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecordName = dsDataNodeAllocateString(hDirRef, pszGroupname);
    if (!pRecordName)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    pUsernameToRemove = dsDataNodeAllocateString(hDirRef, pszUsername);
    if (!pUsernameToRemove)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    macError = GetLocalNodePathList(hDirRef, &pathListToLocalNode);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    macError = dsOpenDirNode(hDirRef, pathListToLocalNode, &hNodeRef);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecordTypeGroup = dsDataNodeAllocateString(hDirRef, kDSStdRecordTypeGroups);
    if (!pRecordTypeGroup)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
	pAttrTypeGroupMemberList = dsDataNodeAllocateString(hDirRef, kDSNAttrGroupMembership);
    if (!pAttrTypeGroupMemberList)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    macError = dsOpenRecord(hNodeRef,
                            pRecordTypeGroup,
                            pRecordName,
                            &hRecordRef);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    macError = dsGetRecordAttributeValueByValue(hRecordRef,
                                                pAttrTypeGroupMemberList,
                                                pUsernameToRemove,
                                                &pValueEntry);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    macError = dsRemoveAttributeValue(hRecordRef,
                                      pAttrTypeGroupMemberList,
                                      pValueEntry->fAttributeValueID);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
cleanup:

    if (pValueEntry)
    {
        junk = dsDeallocAttributeValueEntry(hDirRef, pValueEntry);
    }
        
    if (pRecordName)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pRecordName);
    }
    
    if (pathListToLocalNode)
    {
        junk = dsDataListDeallocate(hDirRef, pathListToLocalNode);
    }
    
	if (pRecordTypeGroup)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pRecordTypeGroup);
    }
    
	if (pAttrTypeGroupMemberList)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pAttrTypeGroupMemberList);
    }

    if (pUsernameToRemove)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pUsernameToRemove);
    }
    
    if (hRecordRef)
    {
        dsCloseRecord(hRecordRef);
    }
    
    if (hNodeRef)
    {
        dsCloseDirNode(hNodeRef);
    }

    if (hDirRef)
    {
        dsCloseDirService(hDirRef);
    }
    
    return macError;
}

long
LWAddUserToLocalGroup(
    char* pszUsername,
    const char* pszGroupname
    )
{
    long                macError = eDSNoErr;
    tDirStatus          junk;
	tDirReference       hDirRef = NULL;
	tDirNodeReference   hNodeRef = NULL;
    tRecordReference    hRecordRef = NULL;
    tDataNodePtr        pRecordName = NULL;
    tDataNodePtr        pUsernameToAdd = NULL;
    tDataListPtr        pathListToLocalNode = NULL;
	tDataNodePtr        pRecordTypeGroup = NULL;
	tDataNodePtr        pAttrTypeGroupMemberList = NULL;

    macError = dsOpenDirService(&hDirRef);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecordName = dsDataNodeAllocateString(hDirRef, pszGroupname);
    if (!pRecordName)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    pUsernameToAdd = dsDataNodeAllocateString(hDirRef, pszUsername);
    if (!pUsernameToAdd)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    macError = GetLocalNodePathList(hDirRef, &pathListToLocalNode);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    macError = dsOpenDirNode(hDirRef, pathListToLocalNode, &hNodeRef);
    GOTO_CLEANUP_ON_MACERROR(macError);

    pRecordTypeGroup = dsDataNodeAllocateString(hDirRef, kDSStdRecordTypeGroups);
    if (!pRecordTypeGroup)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
	pAttrTypeGroupMemberList = dsDataNodeAllocateString(hDirRef, kDSNAttrGroupMembership);
    if (!pAttrTypeGroupMemberList)
    {
        macError = eDSAllocationFailed;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    
    macError = dsOpenRecord(hNodeRef,
                            pRecordTypeGroup,
                            pRecordName,
                            &hRecordRef);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
    macError = dsAddAttributeValue(hRecordRef,
                                   pAttrTypeGroupMemberList,
                                   pUsernameToAdd);
    GOTO_CLEANUP_ON_MACERROR(macError);
    
cleanup:
        
    if (pRecordName)
    {
        junk = dsDataNodeDeAllocate
        (hDirRef, pRecordName);
    }
    
    if (pathListToLocalNode)
    {
        junk = dsDataListDeallocate(hDirRef, pathListToLocalNode);
    }
    
	if (pRecordTypeGroup)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pRecordTypeGroup);
    }
    
	if (pAttrTypeGroupMemberList)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pAttrTypeGroupMemberList);
    }

    if (pUsernameToAdd)
    {
        junk = dsDataNodeDeAllocate(hDirRef, pUsernameToAdd);
    }
    
    if (hRecordRef)
    {
        dsCloseRecord(hRecordRef);
    }
    
    if (hNodeRef)
    {
        dsCloseDirNode(hNodeRef);
    }

    if (hDirRef)
    {
        dsCloseDirService(hDirRef);
    }
    
    return macError;
}

#ifdef __cplusplus
}
#endif
