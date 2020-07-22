/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDPhysicsForceFieldInstance.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDPhysicsRigidConstraintInstance.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUUniqueStringMap.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDPhysicsModelInstance);

FCDPhysicsModelInstance::FCDPhysicsModelInstance(FCDocument* document)
:	FCDEntityInstance(document, NULL, FCDEntity::PHYSICS_MODEL)
{
}

FCDPhysicsModelInstance::~FCDPhysicsModelInstance()
{
}

FCDPhysicsRigidBodyInstance* FCDPhysicsModelInstance::AddRigidBodyInstance(FCDPhysicsRigidBody* rigidBody)
{
	FCDPhysicsRigidBodyInstance* instance = new FCDPhysicsRigidBodyInstance(GetDocument(), this, rigidBody);
	instances.push_back(instance);
	SetDirtyFlag();
	return instance;
}

FCDPhysicsRigidConstraintInstance* FCDPhysicsModelInstance::AddRigidConstraintInstance(FCDPhysicsRigidConstraint* rigidConstraint)
{
	FCDPhysicsRigidConstraintInstance* instance = new FCDPhysicsRigidConstraintInstance(GetDocument(), this, rigidConstraint);
	instances.push_back(instance);
	SetDirtyFlag();
	return instance;
}

FCDPhysicsForceFieldInstance* FCDPhysicsModelInstance::AddForceFieldInstance(FCDForceField* forceField)
{
	FCDEntityInstance* instance = FCDEntityInstanceFactory::CreateInstance(GetDocument(), (FCDSceneNode*) NULL, forceField);
	instances.push_back(instance);
	SetDirtyFlag();
	return (FCDPhysicsForceFieldInstance*)instance;
}

FCDEntityInstance* FCDPhysicsModelInstance::Clone(FCDEntityInstance* _clone) const
{
	FCDPhysicsModelInstance* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsModelInstance(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPhysicsModelInstance::GetClassType())) clone = (FCDPhysicsModelInstance*) _clone;
	
	Parent::Clone(_clone);

	if (clone != NULL)
	{
		for (FCDEntityInstanceContainer::const_iterator it = instances.begin(); it != instances.end(); ++it)
		{
			FCDEntityInstance* clonedInstance = NULL;
			switch ((*it)->GetEntityType())
			{
			case FCDEntity::PHYSICS_RIGID_BODY: clonedInstance = clone->AddRigidBodyInstance(); break;
			case FCDEntity::PHYSICS_RIGID_CONSTRAINT: clonedInstance = clone->AddRigidConstraintInstance(); break;
			case FCDEntity::FORCE_FIELD: clonedInstance = clone->AddForceFieldInstance(); break;
			default: FUFail(break);
			}
			if (clonedInstance != NULL) (*it)->Clone(clonedInstance);
		}
	}
	return _clone;
}

bool FCDPhysicsModelInstance::LoadFromXML(xmlNode* instanceNode)
{
	bool status = FCDEntityInstance::LoadFromXML(instanceNode);
	if (!status) return status;

	if (GetEntity() == NULL)
	{
		FUError::Error(FUError::ERROR, FUError::WARNING_MISSING_URI_TARGET, instanceNode->line);
	}

	// Check for the expected instantiation node type
	if (!IsEquivalent(instanceNode->name, DAE_INSTANCE_PHYSICS_MODEL_ELEMENT))
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_UNKNOWN_ELEMENT, instanceNode->line);
	}

	//this is already done in the FCDSceneNode
//	fm::string physicsModelId = ReadNodeProperty(instanceNode, DAE_TARGET_ATTRIBUTE);
//	entity = GetDocument()->FindPhysicsModel(physicsModelId);
//	if (!entity)	return status.Fail(FS("Couldn't find physics model for instantiation"), instanceNode->line);

	xmlNodeList rigidBodyNodes;
	FindChildrenByType(instanceNode, DAE_INSTANCE_RIGID_BODY_ELEMENT, rigidBodyNodes);
	for (xmlNodeList::iterator itB = rigidBodyNodes.begin(); itB != rigidBodyNodes.end(); ++itB)
	{
		FCDPhysicsRigidBodyInstance* instance = AddRigidBodyInstance(NULL);
		status &= (instance->LoadFromXML(*itB));
	}

	xmlNodeList rigidConstraintNodes;
	FindChildrenByType(instanceNode, DAE_INSTANCE_RIGID_CONSTRAINT_ELEMENT, rigidConstraintNodes);
	for (xmlNodeList::iterator itC = rigidConstraintNodes.begin(); itC != rigidConstraintNodes.end(); ++itC)
	{
		FCDPhysicsRigidConstraintInstance* instance = AddRigidConstraintInstance(NULL);
		status &= (instance->LoadFromXML(*itC));
	}

	xmlNodeList forceFieldNodes;
	FindChildrenByType(instanceNode, DAE_INSTANCE_FORCE_FIELD_ELEMENT, forceFieldNodes);
	for (xmlNodeList::iterator itN = forceFieldNodes.begin(); itN != forceFieldNodes.end(); ++itN)
	{
		FCDPhysicsForceFieldInstance* instance = AddForceFieldInstance(NULL);
		status &= (instance->LoadFromXML(*itN));
	}

	SetDirtyFlag();
	return status;
}

// Write out the instantiation information to the XML node tree
xmlNode* FCDPhysicsModelInstance::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* instanceNode = FCDEntityInstance::WriteToXML(parentNode);

	// The sub-instances must be ordered correctly: force fields first, then rigid bodies; rigid constraints are last.
	for (FCDEntityInstanceContainer::const_iterator it = instances.begin(); it != instances.end(); ++it)
	{
		if ((*it)->GetEntityType() == FCDEntity::FORCE_FIELD)
		{
			(*it)->LetWriteToXML(instanceNode);
		}
	}
	for (FCDEntityInstanceContainer::const_iterator it = instances.begin(); it != instances.end(); ++it)
	{
		if ((*it)->GetEntityType() == FCDEntity::PHYSICS_RIGID_BODY)
		{
			(*it)->LetWriteToXML(instanceNode);
		}
	}
	for (FCDEntityInstanceContainer::const_iterator it = instances.begin(); it != instances.end(); ++it)
	{
		if ((*it)->GetEntityType() == FCDEntity::PHYSICS_RIGID_CONSTRAINT)
		{
			(*it)->LetWriteToXML(instanceNode);
		}
	}

	Parent::WriteToExtraXML(instanceNode);
	return instanceNode;
}

void FCDPhysicsModelInstance::CleanSubId(FUSUniqueStringMap* parentStringMap)
{
	Parent::CleanSubId(parentStringMap);
	FUSUniqueStringMap myStringMap;

	for (FCDEntityInstanceContainer::iterator it = instances.begin(); it != instances.end(); ++it)
	{
		(*it)->CleanSubId(&myStringMap);
	}
}
