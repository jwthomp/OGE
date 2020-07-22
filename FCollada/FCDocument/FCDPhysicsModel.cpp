/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidConstraint.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUUniqueStringMap.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDPhysicsModel);

FCDPhysicsModel::FCDPhysicsModel(FCDocument* document)
:	FCDEntity(document, "PhysicsModel")
{
}

FCDPhysicsModel::~FCDPhysicsModel()
{
}

FCDPhysicsModelInstance* FCDPhysicsModel::AddPhysicsModelInstance(FCDPhysicsModel* model)
{
	FCDPhysicsModelInstance* instance = instances.Add(GetDocument());	
	instance->SetEntity(model);
	SetDirtyFlag(); 
	return instance;
}

FCDPhysicsRigidBody* FCDPhysicsModel::AddRigidBody()
{
	FCDPhysicsRigidBody* rigidBody = rigidBodies.Add(GetDocument());
	SetDirtyFlag(); 
	return rigidBody;
}

FCDPhysicsRigidConstraint* FCDPhysicsModel::AddRigidConstraint()
{
	FCDPhysicsRigidConstraint* constraint = rigidConstraints.Add(GetDocument(), this);
	SetDirtyFlag(); 
	return constraint;
}

FCDEntity* FCDPhysicsModel::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPhysicsModel* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsModel(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPhysicsModel::GetClassType())) clone = (FCDPhysicsModel*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		// Clone the rigid bodies
		for (FCDPhysicsRigidBodyContainer::const_iterator it = rigidBodies.begin(); it != rigidBodies.end(); ++it)
		{
			FCDPhysicsRigidBody* clonedRigidBody = clone->AddRigidBody();
			(*it)->Clone(clonedRigidBody, cloneChildren);
		}

		// Clone the rigid constraints
		for (FCDPhysicsRigidConstraintContainer::const_iterator it = rigidConstraints.begin(); it != rigidConstraints.end(); ++it)
		{
			FCDPhysicsRigidConstraint* clonedConstraint = clone->AddRigidConstraint();
			(*it)->Clone(clonedConstraint, cloneChildren);
		}

		// Clone the model instances
		for (FCDPhysicsModelInstanceContainer::const_iterator it = instances.begin(); it != instances.end(); ++it)
		{
			FCDPhysicsModelInstance* clonedInstance = clone->AddPhysicsModelInstance();
			(*it)->Clone(clonedInstance);
		}
	}
	return _clone;
}

const FCDPhysicsRigidBody* FCDPhysicsModel::FindRigidBodyFromSid(const fm::string& sid) const
{
	for (FCDPhysicsRigidBodyContainer::const_iterator it = rigidBodies.begin(); it!= rigidBodies.end(); ++it)
	{
		if ((*it)->GetSubId() == sid) return (*it);
	}
	return NULL;
}

const FCDPhysicsRigidConstraint* FCDPhysicsModel::FindRigidConstraintFromSid(const fm::string& sid) const
{
	for (FCDPhysicsRigidConstraintContainer::const_iterator it = rigidConstraints.begin(); it!= rigidConstraints.end(); ++it)
	{
		if ((*it)->GetSubId() == sid) return (*it);
	}
	return NULL;
}


bool FCDPhysicsModel::LoadFromXML(xmlNode* physicsModelNode)
{
	bool status = FCDEntity::LoadFromXML(physicsModelNode);
	if (!status) return status;
	if (!IsEquivalent(physicsModelNode->name, DAE_PHYSICS_MODEL_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_PHYS_LIB_ELEMENT, physicsModelNode->line);
		return status;
	}

	// Read in the first valid child element found
	for (xmlNode* child = physicsModelNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_RIGID_BODY_ELEMENT))
		{
			FCDPhysicsRigidBody* rigidBody = AddRigidBody();
			status &= (rigidBody->LoadFromXML(child));

		}
		else if (IsEquivalent(child->name, DAE_RIGID_CONSTRAINT_ELEMENT))
		{
			FCDPhysicsRigidConstraint* rigidConstraint = AddRigidConstraint();
			status &= (rigidConstraint->LoadFromXML(child));
		}
		else if (IsEquivalent(child->name, DAE_INSTANCE_PHYSICS_MODEL_ELEMENT))
		{
			//FIXME: the instantiated physicsModel might not have been parsed yet
			FUUri url = ReadNodeUrl(child);
			if (url.prefix.empty()) 
			{ 
				FCDEntity* entity = GetDocument()->FindPhysicsModel(url.suffix);
				if (entity != NULL) 
				{
					FCDPhysicsModelInstance* instance = AddPhysicsModelInstance((FCDPhysicsModel*) entity);
					status &= (instance->LoadFromXML(child));
				}
				else
				{
					FUError::Error(FUError::WARNING, FUError::WARNING_CORRUPTED_INSTANCE, physicsModelNode->line);
				}
			}
		}
		else if (IsEquivalent(child->name, DAE_ASSET_ELEMENT))
		{
		}
		else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT))
		{
		}
	}

	SetDirtyFlag();
	return status;
}

xmlNode* FCDPhysicsModel::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* physicsModelNode = WriteToEntityXML(parentNode, DAE_PHYSICS_MODEL_ELEMENT);
	for (FCDPhysicsModelInstanceContainer::const_iterator it = instances.begin(); it != instances.end(); ++it)
	{
		(*it)->LetWriteToXML(physicsModelNode);
	}
	for (FCDPhysicsRigidBodyContainer::const_iterator it = rigidBodies.begin(); it != rigidBodies.end(); ++it)
	{
		(*it)->LetWriteToXML(physicsModelNode);
	}
	for (FCDPhysicsRigidConstraintContainer::const_iterator it = rigidConstraints.begin(); it != rigidConstraints.end(); ++it)
	{
		(*it)->LetWriteToXML(physicsModelNode);
	}

	FCDEntity::WriteToExtraXML(physicsModelNode);
	return physicsModelNode;
}

void FCDPhysicsModel::CleanSubId()
{
	FUSUniqueStringMap myStringMap;

	for (FCDPhysicsModelInstanceContainer::iterator it = instances.begin(); it != instances.end(); ++it)
	{
		(*it)->CleanSubId(&myStringMap);
	}
}
