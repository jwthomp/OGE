/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidConstraint.h"
#include "FCDocument/FCDPhysicsRigidConstraintInstance.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDPhysicsRigidConstraintInstance);

FCDPhysicsRigidConstraintInstance::FCDPhysicsRigidConstraintInstance(FCDocument* document, FCDPhysicsModelInstance* _parent, FCDPhysicsRigidConstraint* constraint)
:	FCDEntityInstance(document, NULL, FCDEntity::PHYSICS_RIGID_CONSTRAINT), parent(_parent)
{
	if (constraint != NULL)
	{
		SetRigidConstraint(constraint); 
	}
	noUrl = true;
}

FCDPhysicsRigidConstraintInstance::~FCDPhysicsRigidConstraintInstance()
{
	parent = NULL;
}

FCDEntityInstance* FCDPhysicsRigidConstraintInstance::Clone(FCDEntityInstance* _clone) const
{
	FCDPhysicsRigidConstraintInstance* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsRigidConstraintInstance(const_cast<FCDocument*>(GetDocument()), NULL, NULL);
	else if (_clone->HasType(FCDPhysicsRigidConstraintInstance::GetClassType())) clone = (FCDPhysicsRigidConstraintInstance*) _clone;

	Parent::Clone(_clone);

	if (clone != NULL)
	{
		// No interesting data to clone.
	}

	return _clone;
}

bool FCDPhysicsRigidConstraintInstance::LoadFromXML(xmlNode* instanceNode)
{
	bool status = FCDEntityInstance::LoadFromXML(instanceNode);
	if (!status) return status;

	// Check for the expected instantiation node type
	if (!IsEquivalent(instanceNode->name, DAE_INSTANCE_RIGID_CONSTRAINT_ELEMENT)
		|| parent == NULL || parent->GetEntity() == NULL)
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_UNKNOWN_ELEMENT, instanceNode->line);
		status = false;
	}

	FCDPhysicsModel* model = (FCDPhysicsModel*) parent->GetEntity();
	fm::string physicsRigidConstraintSid = ReadNodeProperty(instanceNode, DAE_CONSTRAINT_ATTRIBUTE);
	FCDPhysicsRigidConstraint* rigidConstraint = model->FindRigidConstraintFromSid(physicsRigidConstraintSid);
	if (!rigidConstraint)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_RIGID_CONSTRAINT_MISSING, instanceNode->line);
		return status;
	}
	SetRigidConstraint(rigidConstraint);
	SetDirtyFlag();
	return status;
}

xmlNode* FCDPhysicsRigidConstraintInstance::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* instanceNode = Parent::WriteToXML(parentNode);

	if (GetEntity() != NULL && GetEntity()->GetObjectType() == FCDPhysicsRigidConstraint::GetClassType())
	{
		FCDPhysicsRigidConstraint* constraint = (FCDPhysicsRigidConstraint*) GetEntity();
		AddAttribute(instanceNode, DAE_CONSTRAINT_ATTRIBUTE, constraint->GetSubId());
	}

	Parent::WriteToExtraXML(instanceNode);
	return instanceNode;
}

void FCDPhysicsRigidConstraintInstance::SetRigidConstraint(
		FCDPhysicsRigidConstraint* constraint)
{
	FUAssert(constraint != NULL, ; );

	SetEntity(constraint); 
}
