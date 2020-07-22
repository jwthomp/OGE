/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDExternalReference.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDPlaceHolder.h"
#include "FUtils/FUFileManager.h"

//
// FCDPlaceHolder
//

ImplementObjectType(FCDPlaceHolder);

FCDPlaceHolder::FCDPlaceHolder(FCDocument* document, FCDocument* _target)
:	FCDObject(document)
,	target(_target)
{
	if (target != NULL)
	{
		TrackObject(target);
		fileUrl = target->GetFileUrl();
	}
}

FCDPlaceHolder::~FCDPlaceHolder()
{
	if (target != NULL)
	{
		UntrackObject(target);
		if (target->GetTrackerCount() == 0)
		{
			target->Release();
		}
	}
}

const fstring& FCDPlaceHolder::GetFileUrl() const 
{
	return (target != NULL) ? target->GetFileUrl() : fileUrl;
}

void FCDPlaceHolder::SetFileUrl(const fstring& url)
{
	fileUrl = url;
	SetDirtyFlag();
}

FCDocument* FCDPlaceHolder::GetTarget(bool loadIfMissing)
{
	if (target == NULL && loadIfMissing) LoadTarget(NULL);
	return target;
}

void FCDPlaceHolder::LoadTarget(FCDocument* newTarget)
{
	if (target == NULL)
	{
		if (newTarget == NULL)
		{
			newTarget = new FCDocument();
			fstring filename = GetDocument()->GetFileManager()->GetFilePath(fileUrl);
			loadStatus = newTarget->LoadFromFile(filename);
			if (!loadStatus)
			{
				delete newTarget;
				newTarget = NULL;
			}
		}

		if (newTarget != NULL)
		{
			if (target != NULL)
			{
				fileUrl = target->GetFileUrl();
				UntrackObject(target);
				target = NULL;
			}
			target = newTarget;
			TrackObject(target);
			for (FCDExternalReference** itR = references.begin(); itR != references.end(); ++itR)
			{
				FCDEntityInstance* instance = (*itR)->GetInstance();
				instance->LoadExternalEntity(target, (*itR)->GetEntityId());
			}
		}
		SetDirtyFlag();
	}
}

void FCDPlaceHolder::UnloadTarget()
{
	SAFE_RELEASE(target);
	SetDirtyFlag();
}

void FCDPlaceHolder::OnObjectReleased(FUObject* object)
{
	if (object == target)
	{
		fileUrl = target->GetFileUrl();
		target = NULL;
	}
}
