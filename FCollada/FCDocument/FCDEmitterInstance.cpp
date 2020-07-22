/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDEmitter.h"
#include "FCDocument/FCDEmitterInstance.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDMaterialInstance.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDEmitterInstance
//

ImplementObjectType(FCDEmitterInstance);

FCDEmitterInstance::FCDEmitterInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type entityType)
:	FCDEntityInstance(document, parent, entityType)
{
}

FCDEmitterInstance::~FCDEmitterInstance()
{
	// delete all instances, we own them exclusively
#if _DEBUG
	size_t n = GetEmittedInstanceCount();
	for (size_t i = 0; i < n; i++)
	{
		const FCDEntityInstance* geomInst = GetEmittedInstance(i);
		// If this is asserting, the instance is owned in the scene
		// graph somewhere.  All your instances are belong to us!!!
		FUAssert(geomInst == NULL || geomInst->GetParent() == NULL,)
	}
#endif
	//CLEAR_POINTER_STD_CONT(FCDEntityInstanceList, emittedInstances);
}

FCDEntityInstance* FCDEmitterInstance::Clone(FCDEntityInstance* _clone) const
{
	FCDEmitterInstance* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDEmitterInstance(const_cast<FCDocument*>(GetDocument()), NULL, FCDEntity::EMITTER);
	else if (_clone->HasType(FCDEmitterInstance::GetClassType())) clone = (FCDEmitterInstance*) _clone;

	Parent::Clone(clone);

	// Will this do a deep copy?
	clone->emittedInstances = emittedInstances;
	clone->forceInstances = forceInstances;

	clone->SetNewChildFlag();

	return clone;
}

bool FCDEmitterInstance::AddForceFieldInstance(FCDEntityInstance* forceInstance)
{
	FUAssert(forceInstance != NULL && \
			 forceInstance->GetEntity() != NULL && \
			 forceInstance->GetEntityType() == FCDEntity::FORCE_FIELD, return false);

	forceInstances.push_back(forceInstance);

	SetNewChildFlag();
	return true;
}

FCDEntityInstance* FCDEmitterInstance::AddEmittedInstance(FCDEntity* entity)
{

	FCDEntityInstance *instance = FCDEntityInstanceFactory::CreateInstance(GetDocument(), NULL, entity);
	
	instance->SetEntity(entity);
	emittedInstances.push_back(instance);

	SetNewChildFlag();
	return instance;
}

FCDEntityInstance* FCDEmitterInstance::AddEmittedInstance(FCDEntity::Type type)
{
	FCDEntityInstance *instance = FCDEntityInstanceFactory::CreateInstance(GetDocument(), NULL, type);
	
	emittedInstances.push_back(instance);
	SetNewChildFlag();
	return instance;
}


bool FCDEmitterInstance::LoadFromXML(xmlNode* instanceNode)
{
	bool status = Parent::LoadFromXML(instanceNode);
	if (!status) return status;

	// Check for the expected instantiation node type
	if (!IsEquivalent(instanceNode->name, DAE_INSTANCE_EMITTER_ELEMENT))
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_UNKNOWN_ELEMENT, instanceNode->line);
	}


	SetDirtyFlag();
	return status;
}

xmlNode* FCDEmitterInstance::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* instanceNode = Parent::WriteToXML(parentNode);

	
	Parent::WriteToExtraXML(instanceNode);
	return instanceNode;
}


bool FCDEmitterInstance::LinkImport()
{

	return true;
}

bool FCDEmitterInstance::LinkExport()
{
	// Iterate through our emitted instances, and link them too.
	bool success = true;
	for (FCDEntityInstanceContainer::iterator iit = emittedInstances.begin(); iit != emittedInstances.end(); iit++)
	{
		success &= (*iit)->LinkExport();
	}
	return success;
}

void FCDEmitterInstance::ExportEmittedInstanceList(xmlNode* node) const
{
	(void)node;
}

bool FCDEmitterInstance::ImportEmittedInstanceList(xmlNode* node)
{
	bool status = true;
	(void)node;
	return status;
}

