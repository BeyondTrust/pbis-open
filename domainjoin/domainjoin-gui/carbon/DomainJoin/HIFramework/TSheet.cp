/*
 *  TSheet.cpp
 *  DomainJoin
 *
 *  Created by Sriram Nambakam on 8/13/07.
 *  Copyright 2007 Centeris Corporation. All rights reserved.
 *
 */

#include "TSheet.h"

TSheet::TSheet(int inAppSignature,
               CFStringRef inName,
			   CFStringRef inNibName,
			   TWindow& parentWindow)
: TWindow(inAppSignature, inName, inNibName),
  _parent(parentWindow)
{
}

void
TSheet::Show()
{
    ::ShowSheetWindow(GetWindowRef(), _parent.GetWindowRef());
	::AdvanceKeyboardFocus(GetWindowRef());
}

void
TSheet::Hide()
{
    if (::IsWindowVisible(GetWindowRef()))
	   ::HideSheetWindow(GetWindowRef());
}
