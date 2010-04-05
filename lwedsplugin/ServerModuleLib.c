/*
  File:         ServerModuleLib.c

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


// System headers
#include <stdio.h> // for vfprintf()
#include <string.h> // for memcpy(), strcpy() and strlen()

// Project headers
#include "ServerModuleLib.h"

// ----------------------------------------------------------------------------
// * Private Globals
// ----------------------------------------------------------------------------

static SvrLibFtbl _Funcs = { nil, nil, nil, 0 };


// ----------------------------------------------------------------------------
// * Skeletal implementations of server library functions.
//
// ----------------------------------------------------------------------------

void SetupLinkTable ( SvrLibFtbl *inTable )
{
    if ( _Funcs.registerNode )
    {
        return;
    }

    _Funcs = *inTable;

} // SetupLinkTable


//--------------------------------------------------------------------------------------------------
// * DSRegisterNode()
//
//--------------------------------------------------------------------------------------------------

long DSRegisterNode ( const unsigned long inToken, tDataList *inNode, eDirNodeType inNodeType )
{
    if ( !_Funcs.registerNode )
    {
        return( -2802 );
    }

    return( (*_Funcs.registerNode)(inToken, inNode, inNodeType) );

} // DSRegisterNode


//--------------------------------------------------------------------------------------------------
// * DSUnregisterNode()
//
//--------------------------------------------------------------------------------------------------

long DSUnregisterNode ( const unsigned long inToken, tDataList *inNode )
{
    if ( !_Funcs.unregisterNode )
    {
        return( -2802 );
    }

    return( (*_Funcs.unregisterNode)(inToken, inNode) );

} // DSUnregisterNode


//--------------------------------------------------------------------------------------------------
// * DSDebugLog()
//
//--------------------------------------------------------------------------------------------------

long DSDebugLog ( const char *inFormat, va_list inArgs )
{
    if ( !_Funcs.debugLog )
    {
        return( -2802 );
    }

    (*_Funcs.debugLog)( inFormat, inArgs );

    return( noErr );

} // DSDebugLog
