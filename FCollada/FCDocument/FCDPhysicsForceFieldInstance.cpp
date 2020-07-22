/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDPhysicsForceFieldInstance.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDPhysicsForceFieldInstance);

FCDPhysicsForceFieldInstance::FCDPhysicsForceFieldInstance(
		FCDocument* document, FCDSceneNode* parent, FCDEntity::Type entityType)
:	FCDEntityInstance(document, parent, entityType)
{
}

FCDPhysicsForceFieldInstance::~FCDPhysicsForceFieldInstance()
{
}

FCDEntityInstance* FCDPhysicsForceFieldInstance::Clone(
		FCDEntityInstance* _clone) const
{
	FCDPhysicsForceFieldInstance* clone = NULL;
	if (_clone == NULL) clone = new FCDPhysicsForceFieldInstance(
			const_cast<FCDocument*>(GetDocument()), 
			const_cast<FCDSceneNode*>(GetParent()), GetEntityType());
	else if (!_clone->HasType(FCDPhysicsForceFieldInstance::GetClassType())) 
		return Parent::Clone(_clone);
	else clone = (FCDPhysicsForceFieldInstance*) _clone;

	Parent::Clone(clone);

	// nothing interesting in force field instance to copy

	return clone;
}

bool FCDPhysicsForceFieldInstance::LoadFromXML(xmlNode* instanceNode)
{
	bool status = Parent::LoadFromXML(instanceNode);
	if (!status) return status;

	if (GetEntity() == NULL && !IsExternalReference())
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_INVALID_URI, 
				instanceNode->line);
	}

	// Check for the expected instantiation node type
	if (!IsEquivalent(instanceNode->name, DAE_INSTANCE_FORCE_FIELD_ELEMENT))
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_UNKNOWN_ELEMENT, 
				instanceNode->line);
		status = false;
	}

	// nothing interesting in force field instance to load

	SetDirtyFlag();
	return status;
}

// Write out the instantiation information to the XML node tree
xmlNode* FCDPhysicsForceFieldInstance::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* instanceNode = Parent::WriteToXML(parentNode);

	// nothing interesting in force field instance to write

	Parent::WriteToExtraXML(instanceNode);
	return instanceNode;
}
