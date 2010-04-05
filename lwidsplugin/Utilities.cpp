/*
  File:         Utilities.c

  Version:      Directory Services 1.0

  Copyright:    © 1999-2001 by Apple Computer, Inc., all rights reserved.

  IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc. ("Apple") in
  consideration of your agreement to the following terms, and your use, installation,
  modification or redistribution of this Apple software constitutes acceptance of these
  terms.  If you do not agree with these terms, please do not use, install, modify or
  redistribute this Apple software.

  In consideration of your agreement to abide by the following terms, and subject to these
  terms, Apple grants you a personal, non-exclusive license, under AppleÕs copyrights in
  this original Apple software (the "Apple Software"), to use, reproduce, modify and
  redistribute the Apple Software, with or without modifications, in source and/or binary
  forms; provided that if you redistribute the Apple Software in its entirety and without
  modifications, you must retain this notice and the following text and disclaimers in all
  such redistributions of the Apple Software.  Neither the name, trademarks, service marks
  or logos of Apple Computer, Inc. may be used to endorse or promote products derived from
  the Apple Software without specific prior written permission from Apple. Except as expressly
  stated in this notice, no other rights or licenses, express or implied, are granted by Apple
  herein, including but not limited to any patent rights that may be infringed by your
  derivative works or by other works in which the Apple Software may be incorporated.

  The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO WARRANTIES,
  EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS
  USE AND OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.

  IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE,
  REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND
  WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR
  OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "LWIPlugIn.h"

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
        lineLength = MIN(bytesPerLine, Length - bufferIndex);

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
        DEFAULT_RETURN_UNKNOWN_STRING();
    }
}

long
LWIAllocateMemory(
    size_t dwSize,
    PVOID * ppMemory
    )
{
    long macError = eDSNoErr;
    PVOID pMemory = NULL;

    pMemory = malloc(dwSize);
    if (!pMemory)
    {
        macError = eMemoryAllocError;
        *ppMemory = NULL;
    }
    else
    {
        memset(pMemory,0, dwSize);
        *ppMemory = pMemory;
    }

    return (macError);
}

long
LWIReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    long dwSize
    )
{
    long macError = eDSNoErr;
    PVOID pNewMemory = NULL;

    if (pMemory == NULL) {
        pNewMemory = malloc(dwSize);
        memset(pNewMemory, 0, dwSize);
    }
    else
    {
        pNewMemory = realloc(pMemory, dwSize);
    }

    if (!pNewMemory)
    {
        macError = eMemoryAllocError;
        *ppNewMemory = NULL;
    }
    else
    {
        *ppNewMemory = pNewMemory;
    }

    return(macError);
}


void
LWIFreeMemory(
    PVOID pMemory
    )
{
    free(pMemory);
    return;
}

long
LWIAllocateString(
    const char* pszInputString,
    char** ppszOutputString
    )
{
    long macError = eDSNoErr;
    int len = 0;
    char* pszOutputString = NULL;

    if (!pszInputString || !*pszInputString){
        macError = eDSNullParameter;
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    len = strlen(pszInputString);
    macError = LWIAllocateMemory(len+1, (PVOID *)&pszOutputString);
    GOTO_CLEANUP_ON_MACERROR(macError);

    strcpy(pszOutputString, pszInputString);

cleanup:

    *ppszOutputString = pszOutputString;

    return(macError);
}

void
LWIFreeString(
    char* pszString
    )
{
    if (pszString) {
        LWIFreeMemory(pszString);
    }
}
