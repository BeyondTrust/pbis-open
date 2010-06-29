/*
  File:         ServerModule.c

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
#include <stdio.h> // for fprintf()
#include <CoreFoundation/CoreFoundation.h>

// Project Headers
#include "ServerModulePrivate.h"
#include "ServerModuleLib.h"
#include "PlugInShell.h"
#include <DirectoryService/DirServicesTypes.h>

// ----------------------------------------------------------------------------
// * Private Globals
// ----------------------------------------------------------------------------

static CFUUIDRef ModuleFactoryUUID = NULL;
static ModuleFtbl* _VTablePrototype = NULL;


extern void* ModuleFactory(CFAllocatorRef, CFUUIDRef);

// ----------------------------------------------------------------------------
// * COM Support functions
//
// Boilerplate stuff to support COM's three required methods.
// ----------------------------------------------------------------------------

#pragma mark **** COM Support functions ****

//--------------------------------------------------------------------------------------------------
// * _COMQueryInterface ()
//
//--------------------------------------------------------------------------------------------------

static HRESULT _COMQueryInterface ( void *thisp, REFIID iid, LPVOID *ppv )
{
    CFUUIDRef uuidInterface = CFUUIDCreateFromUUIDBytes ( NULL, iid );

    if ( CFEqual ( uuidInterface, kModuleInterfaceUUID ) ||
         CFEqual ( uuidInterface, IUnknownUUID ) )
    {
        // IUnknownVTbl type is used in case the object is not one of our types.
        ((IUnknownVTbl *)thisp)->AddRef(thisp);
        *ppv = thisp;
        CFRelease( uuidInterface );

        return( S_OK );
    }

    CFRelease( uuidInterface );
    *ppv = NULL;

    return( E_NOINTERFACE );

} // _COMQueryInterface


//--------------------------------------------------------------------------------------------------
// * _COMAddRef ()
//
//--------------------------------------------------------------------------------------------------

static ULONG _COMAddRef ( void *thisp )
{
    ModuleFtbl* opThis = thisp;

    if ( opThis == NULL )
    {
        fputs ( "Gak! Bad cast to _ModuleType!\n", stderr );
        return( (ULONG)-1 );
    }

    return( ++(opThis->mRefCount) );

} // _COMAddRef


//--------------------------------------------------------------------------------------------------
// * _COMRelease ()
//
//--------------------------------------------------------------------------------------------------

static ULONG _COMRelease ( void *thisp )
{
    ModuleFtbl* opThis = thisp;

    if ( opThis == NULL )
    {
        fputs ( "Gak! Bad cast to _ModuleType in Release!\n", stderr );
        return( (ULONG)-1 );
    }

    // Dealloc the ref count.
    if ( --(opThis->mRefCount) )
    {
        return( opThis->mRefCount );
    }

    // If there are no ref counts left, dealloc the object for real.

    free( opThis );
    CFPlugInRemoveInstanceForFactory ( ModuleFactoryUUID );

    return( 0 );

} // _COMRelease


// ----------------------------------------------------------------------------
// * ModuleInterface functions
//
// C glue code from COM interface functions to C++ instance methods.
//
// ----------------------------------------------------------------------------

#pragma mark **** ModuleInterface functions ****

//--------------------------------------------------------------------------------------------------
// * _Validate ()
//
//--------------------------------------------------------------------------------------------------

static long _Validate ( void *thisp, const char *inVersionStr, const unsigned long inSignature )
{
    long nResult = eDSNoErr;

    nResult = PlugInShell_Validate( inVersionStr, inSignature );

    return( nResult );

} // _Validate


//--------------------------------------------------------------------------------------------------
// * _Initialize()
//
//--------------------------------------------------------------------------------------------------

static long _Initialize ( void *thisp )
{
    long nResult = eDSNoErr;

    nResult = PlugInShell_Initialize();

    return( nResult );

} // _Initialize


//--------------------------------------------------------------------------------------------------
// * _Configure ()
//
//--------------------------------------------------------------------------------------------------

static long _Configure ( void *thisp )
{
    long nResult = eDSNoErr;

    nResult = PlugInShell_Configure();

    return( nResult );

} // _Configure


//--------------------------------------------------------------------------------------------------
// * _ProcessRequest ()
//
//--------------------------------------------------------------------------------------------------

static long _ProcessRequest ( void *thisp, void *inData )
{
    long nResult = eDSNoErr;

    nResult = PlugInShell_ProcessRequest( inData );

    return( nResult );

} // _ReceiveFromClient


//--------------------------------------------------------------------------------------------------
// * _SetPluginState ()
//
//--------------------------------------------------------------------------------------------------

static long _SetPluginState ( void *thisp, const unsigned long inState )
{
    long nResult = eDSNoErr;

    nResult = PlugInShell_SetPluginState( inState );

    return( nResult );

} // _SetPluginState


//--------------------------------------------------------------------------------------------------
// * _PeriodicTask ()
//
//--------------------------------------------------------------------------------------------------

static long _PeriodicTask ( void *thisp )
{
    long nResult = eDSNoErr;

    nResult = PlugInShell_PeriodicTask();

    return( nResult );

} // _PeriodicTask


//--------------------------------------------------------------------------------------------------
// * _Shutdown ()
//
//--------------------------------------------------------------------------------------------------

static long _Shutdown ( void *thisp )
{
    long nResult = eDSNoErr;

    nResult = PlugInShell_Shutdown();

    return( nResult );

} // _Shutdown


//--------------------------------------------------------------------------------------------------
// * _LinkLibFtbl ()
//
//--------------------------------------------------------------------------------------------------

static void _LinkLibFtbl ( void *thisp, SvrLibFtbl *inLinkBack )
{
    SetupLinkTable( inLinkBack );
} // _LinkLibFtbl


//--------------------------------------------------------------------------------------------------
// * _InitializeModule ()
//
//--------------------------------------------------------------------------------------------------

static void _InitializeModule ( void )
{
    _VTablePrototype = calloc( sizeof( ModuleFtbl ), sizeof( char ) );

    if ( _VTablePrototype != nil )
    {
        // Assign the function pointers.
        _VTablePrototype->QueryInterface = _COMQueryInterface;
        _VTablePrototype->AddRef = _COMAddRef;
        _VTablePrototype->Release = _COMRelease;

        _VTablePrototype->validate = _Validate;
        _VTablePrototype->initialize = _Initialize;
        _VTablePrototype->configure = _Configure;
        _VTablePrototype->processRequest = _ProcessRequest;
        _VTablePrototype->setPluginState = _SetPluginState;
        _VTablePrototype->periodicTask = _PeriodicTask;
        _VTablePrototype->shutdown = _Shutdown;
        _VTablePrototype->linkLibFtbl  = _LinkLibFtbl;

        _VTablePrototype->mRefCount = 1;
    }
    else
    {
        fputs ( "Serious memory allocation error!\n", stderr );
    }

} // _InitializeModule


// ----------------------------------------------------------------------------
// * Factory function
// This is the only exported function in the file.
// ----------------------------------------------------------------------------

void *ModuleFactory ( CFAllocatorRef allocator, CFUUIDRef typeID )
{
    if ( _VTablePrototype == nil )
    {
        _InitializeModule();
    }

    ModuleFactoryUUID = CFUUIDGetConstantUUIDWithBytes ( NULL,
                                                         0x29, 0x4C, 0x59, 0x80, 0xDE, 0xCE, 0x11, 0xDB,
                                                         0x87, 0x41, 0x00, 0x16, 0xCB, 0xA8, 0x54, 0x97 );

#if DEBUG
    puts( "ModuleFactory: loaded and called!\n" );
    fflush( stdout );
#endif

    if ( CFEqual( typeID, kModuleTypeUUID ) )
    {
        ModuleFtbl* opNew = calloc( sizeof( ModuleFtbl ), sizeof( char ) );

        // Set the instance data variables.
        *opNew = *_VTablePrototype;
        CFPlugInAddInstanceForFactory( ModuleFactoryUUID );

        return( opNew );
    }
    else
    {
        return( NULL );
    }

} // ModuleFactory
