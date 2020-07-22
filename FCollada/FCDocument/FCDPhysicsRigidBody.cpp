/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidBodyParameters.h"
#include "FCDocument/FCDPhysicsShape.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDPhysicsRigidBody);

FCDPhysicsRigidBody::FCDPhysicsRigidBody(FCDocument* document) : FCDEntity(document, "RigidBody")
{
	parameters = new FCDPhysicsRigidBodyParameters(this);
}

FCDPhysicsRigidBody::~FCDPhysicsRigidBody()
{
	SAFE_DELETE(parameters);
}

FCDEntity* FCDPhysicsRigidBody::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPhysicsRigidBody* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsRigidBody(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPhysicsRigidBody::GetClassType())) clone = (FCDPhysicsRigidBody*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		clone->GetParameters().CopyFrom(*parameters);
	}
	return _clone;
}

float FCDPhysicsRigidBody::GetShapeMassFactor() const
{
	float shapesMass = 0.0f;
	const FCDPhysicsShapeContainer& physicsShape = 
			parameters->GetPhysicsShapeList();
	for (FCDPhysicsShapeContainer::const_iterator it = physicsShape.begin();
			it != physicsShape.end(); it++)
	{
		shapesMass += (*it)->GetMass();
	}
	return parameters->GetMass() / shapesMass;
}

bool FCDPhysicsRigidBody::LoadFromXML(xmlNode* physicsRigidBodyNode)
{
	bool status = FCDEntity::LoadFromXML(physicsRigidBodyNode);
	if (!status) return status;
	if (!IsEquivalent(physicsRigidBodyNode->name, DAE_RIGID_BODY_ELEMENT)) 
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_PRB_LIB_ELEMENT, physicsRigidBodyNode->line);
		return status;
	}

	SetSubId(FUDaeParser::ReadNodeSid(physicsRigidBodyNode));

	xmlNode* techniqueNode = FindChildByType(physicsRigidBodyNode, 
			DAE_TECHNIQUE_COMMON_ELEMENT);
	if (techniqueNode != NULL)
	{
		parameters->LoadFromXML(techniqueNode);
	}
	else
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_COMMON_TECHNIQUE_MISSING,
				physicsRigidBodyNode->line);
	}

	return status;
}

xmlNode* FCDPhysicsRigidBody::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* physicsRigidBodyNode = WriteToEntityXML(parentNode, DAE_RIGID_BODY_ELEMENT, false);
	const_cast<FCDPhysicsRigidBody*>(this)->SetSubId(AddNodeSid(physicsRigidBodyNode, GetSubId().c_str()));

	xmlNode* baseNode = AddChild(physicsRigidBodyNode, DAE_TECHNIQUE_COMMON_ELEMENT);

	parameters->WriteToXML(baseNode);

	FCDEntity::WriteToExtraXML(physicsRigidBodyNode);
	return physicsRigidBodyNode;
}
