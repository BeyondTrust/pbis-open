/*
 *  MainWindow.cpp
 *  UserMigrate
 *
 *  Created by Chuck Mount on 8/7/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "MainWindow.h"
#include "DomainMigrateWindow.h"

//--------------------------------------------------------------------------------------------
Boolean
MainWindow::HandleCommand( const HICommandExtended& inCommand )
{
    switch ( inCommand.commandID )
    {
        // Add your own command-handling cases here
        
        default:
            return false;
    }
	
	return true;
}
