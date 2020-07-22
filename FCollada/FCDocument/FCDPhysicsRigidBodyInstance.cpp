/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyParameters.h"
#include "FCDocument/FCDSceneNode.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDPhysicsRigidBodyInstance);

FCDPhysicsRigidBodyInstance::FCDPhysicsRigidBodyInstance(FCDocument* document, FCDPhysicsModelInstance* _parent, FCDPhysicsRigidBody* body)
:	FCDEntityInstance(document, NULL, FCDEntity::PHYSICS_RIGID_BODY)
,	parent(_parent), parameters(NULL), angularVelocity(FMVector3::Zero), velocity(FMVector3::Zero)
{
	parameters = new FCDPhysicsRigidBodyParameters(this);

	if (body != NULL)
	{
		SetRigidBody(body);
	}
	noUrl = true;
}

FCDPhysicsRigidBodyInstance::~FCDPhysicsRigidBodyInstance()
{
	parent = NULL;
	SAFE_DELETE(parameters);
}

FCDEntityInstance* FCDPhysicsRigidBodyInstance::Clone(FCDEntityInstance* _clone) const
{
	FCDPhysicsRigidBodyInstance* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsRigidBodyInstance(const_cast<FCDocument*>(GetDocument()), NULL, NULL);
	else if (_clone->HasType(FCDPhysicsRigidBodyInstance::GetClassType())) clone = (FCDPhysicsRigidBodyInstance*) _clone;

	Parent::Clone(_clone);
	
	if (clone != NULL)
	{
		clone->angularVelocity = angularVelocity;
		clone->velocity = velocity;
		clone->GetParameters().CopyFrom(*parameters);

		// Intentionally leave the target scene node as NULL.
	}
	return _clone;
}

bool FCDPhysicsRigidBodyInstance::LoadFromXML(xmlNode* instanceNode)
{
	bool status = FCDEntityInstance::LoadFromXML(instanceNode);
	if (!status) return status;

	// Check for the expected instantiation node type
	if (!IsEquivalent(instanceNode->name, DAE_INSTANCE_RIGID_BODY_ELEMENT) || parent == NULL)
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_UNKNOWN_ELEMENT, instanceNode->line);
		status = false;
	}

	// Find the target scene node/rigid body
	fm::string targetNodeId = ReadNodeProperty(instanceNode, DAE_TARGET_ATTRIBUTE);
	targetNode = GetDocument()->FindSceneNode(SkipPound(targetNodeId));
	if (!targetNode)
	{
		FUError::Error(FUError::ERROR, FUError::WARNING_MISSING_URI_TARGET, instanceNode->line);
	}

	// Find the instantiated rigid body
	FCDPhysicsRigidBody* body = NULL;
	fm::string physicsRigidBodySid = ReadNodeProperty(instanceNode, DAE_BODY_ATTRIBUTE);
	if (parent->GetEntity() != NULL &&  parent->GetEntity()->GetType() == FCDEntity::PHYSICS_MODEL)
	{
		FCDPhysicsModel* model = (FCDPhysicsModel*) parent->GetEntity();
		body = model->FindRigidBodyFromSid(physicsRigidBodySid);
		if (body == NULL)
		{
			FUError::Error(FUError::ERROR, FUError::WARNING_MISSING_URI_TARGET, instanceNode->line);
			return false;
		}
		SetRigidBody(body);
	}

	//Read in the same children as rigid_body + velocity and angular_velocity
	xmlNode* techniqueNode = FindChildByType(instanceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	if (techniqueNode == NULL)
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_TECHNIQUE_NODE_MISSING,
				instanceNode->line);
		return false;
	}

	xmlNode* param = 
			FindChildByType(techniqueNode, DAE_ANGULAR_VELOCITY_ELEMENT);
	if (param != NULL)
	{
		angularVelocity = FUStringConversion::ToVector3(
				ReadNodeContentDirect(param));
	}
	else
	{
		angularVelocity = FMVector3::Zero;
	}

	param = FindChildByType(techniqueNode, DAE_VELOCITY_ELEMENT);
	if (param != NULL)
	{
		velocity = FUStringConversion::ToVector3(ReadNodeContentDirect(param));
	}
	else
	{
		velocity = FMVector3::Zero;
	}

	parameters->LoadFromXML(techniqueNode, &body->GetParameters());

	SetDirtyFlag();
	return status;
}

// Write out the instantiation information to the XML node tree
xmlNode* FCDPhysicsRigidBodyInstance::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* instanceNode = FCDEntityInstance::WriteToXML(parentNode);

	AddAttribute(instanceNode, DAE_TARGET_ATTRIBUTE, fm::string("#") + targetNode->GetDaeId());
	AddAttribute(instanceNode, DAE_BODY_ATTRIBUTE, GetEntity()->GetDaeId());

	//inconsistency in the spec
	RemoveAttribute(instanceNode, DAE_URL_ATTRIBUTE);
	
	xmlNode* techniqueNode = AddChild(instanceNode, DAE_TECHNIQUE_COMMON_ELEMENT);

	// almost same as FCDPhysicsRigidBody
	parameters->AddPhysicsParameter<FMVector3>(techniqueNode, 
			DAE_ANGULAR_VELOCITY_ELEMENT, angularVelocity);
	parameters->AddPhysicsParameter<FMVector3>(techniqueNode, 
			DAE_VELOCITY_ELEMENT, velocity);
	parameters->WriteToXML(techniqueNode);

	Parent::WriteToExtraXML(instanceNode);
	return instanceNode;
}

void FCDPhysicsRigidBodyInstance::SetRigidBody(FCDPhysicsRigidBody* body)
{
	FUAssert(body != NULL, ; );

	SetEntity(body); 

	// copy some of the default values from the body
	FCDPhysicsRigidBodyParameters& bodyParams = body->GetParameters();
	parameters->SetDynamic(bodyParams.IsDynamic());
	parameters->SetMass(bodyParams.GetMass());
	parameters->SetMassFrameTranslate(bodyParams.GetMassFrameTranslate());
	parameters->SetMassFrameRotateAxis(bodyParams.GetMassFrameRotateAxis());
	parameters->SetMassFrameRotateAngle(bodyParams.GetMassFrameRotateAngle());
	parameters->SetInertia(bodyParams.GetInertia());
}
