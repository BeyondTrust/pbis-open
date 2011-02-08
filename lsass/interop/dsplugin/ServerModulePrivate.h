/*
  File:         ServerModulePrivate.h

  Contains:

  Version:      Directory Services 1.0

  Copyright:    © 1999-2001 by Apple Computer, Inc., all rights reserved.


  ******************** WARNING ********************

  Do _NOT_ edit this file.

  This file is needed by the Directory Services server to load this plug-in module.

  ******************** WARNING ********************

  IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc. ("Apple") in
  consideration of your agreement to the following terms, and your use, installation,
  modification or redistribution of this Apple software constitutes acceptance of these
  terms.  If you do not agree with these terms, please do not use, install, modify or
  redistribute this Apple software.

  In consideration of your agreement to abide by the following terms, and subject to these
  terms, Apple grants you a personal, non-exclusive license, under Apple’s copyrights in
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


#ifndef __ServerModulePrivate_h__
#define __ServerModulePrivate_h__ 1

// System headers
#include <CoreFoundation/CFPlugIn.h>
#if COREFOUNDATION_CFPLUGINCOM_SEPARATE
#include <CoreFoundation/CFPlugInCOM.h>
#endif
#include "PlugInShell.h"
#include <DirectoryService/DirServicesTypes.h>
#include "ServerModule.h"


//-----------------------------------------------------------------------------
// ServerModule Definitions
//-----------------------------------------------------------------------------

/**** Typedefs, enums, and constants. ****/
// The following constants are COM UUID's used to identify the plug-in classes
// in each bundle's Info.plist. See CFBundle / CFPlugIn docs for details.

#define kModuleTypeUUID (CFUUIDGetConstantUUIDWithBytes (NULL, \
                                                         0x69, 0x7B, 0x5D, 0x6C, 0x87, 0xA1, 0x12, 0x26, \
                                                         0x89, 0xCA, 0x00, 0x05, 0x02, 0xC1, 0xC7, 0x36 ))

#define kModuleInterfaceUUID (CFUUIDGetConstantUUIDWithBytes (NULL, \
                                                              0x1A, 0xE9, 0x66, 0x90, /* - */ \
                                                              0x62, 0xCF, /* - */ 0x12, 0x26, /* - */ 0xB4, 0x5C, /* - */ \
                                                              0x00, 0x05, 0x02, 0x07, 0xF7, 0xFD))

// kPlugin and kPluginInterface are used to identify the plug-in type in
// each bundle's Info.plist. See CFBundle / CFPlugIn docs for details.
#define kPluginVersionStr     CFSTR( "CFBundleVersion" )
#define kPluginConfigAvailStr CFSTR( "CFBundleConfigAvail" )
#define kPluginConfigFileStr  CFSTR( "CFBundleConfigFile" )
#define kPluginNameStr        CFSTR( "CFPluginNameString" )


//-----------------------------------------------------------------------------
// Plugin Module Function Table representation for CFPlugin
//-----------------------------------------------------------------------------
// Function table for the com.apple.DSServer.ModuleInterface

typedef struct tagModuleInterfaceFtbl
{
    /**** Required COM header info. ****/
    IUNKNOWN_C_GUTS;

    /**** Instance methods. ****/
    long (*validate)(void *thisp, const char *inVersionStr, const unsigned long inSignature);
    long (*initialize)(void *thisp);
    long (*configure)(void *thisp);
    long (*processRequest)(void *thisp, void *inData);
    long (*setPluginState)(void *thisp, const unsigned long inState);
    long (*periodicTask)(void *thisp);
    long (*shutdown)(void *thisp);
    void (*linkLibFtbl)(void *thisp, SvrLibFtbl *inLinkBack);

    unsigned long mRefCount;
} ModuleFtbl;


#endif // __ServerModulePrivate_h__
