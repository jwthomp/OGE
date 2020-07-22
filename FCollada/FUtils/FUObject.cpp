/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUtils/FUObject.h"
#include "FUtils/FUObjectType.h"

//
// FUObject
//

FUObject::FUObject()
{
}

FUObject::~FUObject()
{
	// Detach this object from its trackers.
	Detach();
}

void FUObject::Detach()
{
	for (FUObjectTrackerList::iterator itT = trackers.begin(); itT != trackers.end(); ++itT)
	{
		(*itT)->OnObjectReleased(this);
	}
	trackers.clear();
}

// Releases this object. This function essentially calls the destructor.
void FUObject::Release()
{
	delete this;
}

// Manage the list of trackers
void FUObject::AddTracker(FUObjectTracker* tracker)
{
	FUAssert(!trackers.contains(tracker), return);
	trackers.push_back(tracker);
}
void FUObject::RemoveTracker(FUObjectTracker* tracker)
{
	//FUAssert(trackers.contains(tracker), return);
	//trackers.erase(tracker);
	FUAssert(trackers.erase(tracker), );
}
bool FUObject::HasTracker(const FUObjectTracker* tracker) const
{
	return trackers.contains(const_cast<FUObjectTracker*>(tracker));
}

FUObjectType __baseObjectType("FUObject");
FUObjectType* FUObject::baseObjectType = &__baseObjectType;
