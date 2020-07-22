/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDObject.h"
#include "FUtils/FUUniqueStringMap.h"
#include "FUtils/FUDaeWriter.h"

// 
// FCDObject
//

ImplementObjectType(FCDObject);

FCDObject::FCDObject(FCDocument* _document)
:	FUObject(), document(_document)
,	userHandle(NULL), dirty(true)
{
}

//
// FCDObjectWithId
//

ImplementObjectType(FCDObjectWithId);

FCDObjectWithId::FCDObjectWithId(FCDocument* document, const char* baseId)
:	FCDObject(document)
{
	daeId = baseId;
	hasUniqueId = false;
}

FCDObjectWithId::~FCDObjectWithId()
{
	RemoveDaeId();
}

void FCDObjectWithId::Clone(FCDObjectWithId* clone) const
{
	clone->daeId = daeId;
	clone->hasUniqueId = false;
}

const fm::string& FCDObjectWithId::GetDaeId() const
{
	if (!hasUniqueId)
	{
		// Generate a new id
		FCDObjectWithId* e = const_cast<FCDObjectWithId*>(this);
		FUSUniqueStringMap* names = e->GetDocument()->GetUniqueNameMap();
		FUAssert(!e->daeId.empty(), e->daeId = "unknown_object");
		names->insert(e->daeId);
		e->hasUniqueId = true;
	}
	return daeId;
}

void FCDObjectWithId::SetDaeId(const fm::string& id)
{
	RemoveDaeId();

	// Use this id to enforce a unique id.
	FUSUniqueStringMap* names = GetDocument()->GetUniqueNameMap();
	daeId = FUDaeWriter::CleanId(id);
	names->insert(daeId);
	hasUniqueId = true;
	SetDirtyFlag();
}

void FCDObjectWithId::SetDaeId(fm::string& id)
{
	RemoveDaeId();

	// Use this id to enforce a unique id.
	FUSUniqueStringMap* names = GetDocument()->GetUniqueNameMap();
	daeId = FUDaeWriter::CleanId(id);
	names->insert(daeId);
	id = daeId;
	hasUniqueId = true;
	SetDirtyFlag();
}

void FCDObjectWithId::RemoveDaeId()
{
	if (hasUniqueId)
	{
		FUSUniqueStringMap* names = GetDocument()->GetUniqueNameMap();
		names->erase(daeId);
		hasUniqueId = false;
		SetDirtyFlag();
	}
}