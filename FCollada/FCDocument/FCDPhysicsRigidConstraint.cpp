/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsRigidConstraint.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDTransform.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDAnimated.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDPhysicsRigidConstraint);

FCDPhysicsRigidConstraint::FCDPhysicsRigidConstraint(FCDocument* document, FCDPhysicsModel* _parent)
:	FCDEntity(document, "PhysicsRigidConstraint")
{
	enabled = true;
	interpenetrate = false;
	parent = _parent;

	limitsLinearMin = FMVector3(0.f, 0.f, 0.f);
	limitsLinearMax = FMVector3(0.f, 0.f, 0.f);
	limitsSCTMin = FMVector3(0.f, 0.f, 0.f);
	limitsSCTMax = FMVector3(0.f, 0.f, 0.f);

	referenceRigidBody = NULL;
	targetRigidBody = NULL;
	referenceNode = NULL;
	targetNode = NULL;

	springLinearStiffness = 1.f;
	springLinearDamping = 0.f;
	springLinearTargetValue = 0.f;

	springAngularStiffness = 1.f;
	springAngularDamping = 0.f;
	springAngularTargetValue = 0.f;
}

FCDPhysicsRigidConstraint::~FCDPhysicsRigidConstraint()
{
	referenceRigidBody = NULL;
	targetRigidBody = NULL;
	transformsTar.clear();
	transformsRef.clear();
}

FCDTransform* FCDPhysicsRigidConstraint::AddTransformRef(FCDTransform::Type type, size_t index)
{
	FCDTransform* transform = FCDTFactory::CreateTransform(GetDocument(), NULL, type);
	if (transform != NULL)
	{
		if (index > transformsRef.size()) transformsRef.push_back(transform);
		else transformsRef.insert(transformsRef.begin() + index, transform);
	}
	SetDirtyFlag();
	return transform;
}

FCDTransform* FCDPhysicsRigidConstraint::AddTransformTar(FCDTransform::Type type, size_t index)
{
	FCDTransform* transform = FCDTFactory::CreateTransform(GetDocument(), NULL, type);
	if (transform != NULL)
	{
		if (index > transformsTar.size()) transformsTar.push_back(transform);
		else transformsTar.insert(transformsTar.begin() + index, transform);
	}
	SetDirtyFlag();
	return transform;
}

// Create a copy of this physicsRigidConstraint, with the vertices overwritten
FCDEntity* FCDPhysicsRigidConstraint::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPhysicsRigidConstraint* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsRigidConstraint(const_cast<FCDocument*>(GetDocument()), NULL);
	else if (_clone->HasType(FCDPhysicsRigidConstraint::GetClassType())) clone = (FCDPhysicsRigidConstraint*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		clone->enabled = enabled;
		clone->interpenetrate = interpenetrate;
		clone->referenceRigidBody = referenceRigidBody;
		clone->referenceNode = referenceNode;
		clone->targetRigidBody = targetRigidBody;
		clone->targetNode = targetNode;
		clone->limitsLinearMax = limitsLinearMax;
		clone->limitsLinearMin = limitsLinearMin;
		clone->limitsSCTMax = limitsSCTMax;
		clone->limitsSCTMin = limitsSCTMin;
		clone->springAngularDamping = springAngularDamping;
		clone->springAngularStiffness = springAngularStiffness;
		clone->springAngularTargetValue = springAngularTargetValue;
		clone->springLinearDamping = springLinearDamping;
		clone->springLinearStiffness = springLinearStiffness;
		clone->springLinearTargetValue = springLinearTargetValue;

		// Clone the transforms
		for (FCDTransformContainer::const_iterator it = transformsRef.begin(); it != transformsRef.end(); ++it)
		{
			FCDTransform* clonedTransform = clone->AddTransformRef((*it)->GetType());
			(*it)->Clone(clonedTransform);
		}
		for (FCDTransformContainer::const_iterator it = transformsTar.begin(); it != transformsTar.end(); ++it)
		{
			FCDTransform* clonedTransform = clone->AddTransformTar((*it)->GetType());
			(*it)->Clone(clonedTransform);
		}
	}
	return _clone;
}

// Load from a XML node the given physicsRigidConstraint
bool FCDPhysicsRigidConstraint::LoadFromXML(xmlNode* physicsRigidConstraintNode)
{
	bool status = FCDEntity::LoadFromXML(physicsRigidConstraintNode);
	if (!status) return status;
	if (!IsEquivalent(physicsRigidConstraintNode->name, DAE_RIGID_CONSTRAINT_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_RGC_LIB_ELEMENT, physicsRigidConstraintNode->line);
		return status;
	}

	SetSubId(FUDaeParser::ReadNodeSid(physicsRigidConstraintNode));

#define PARSE_TRANSFORM(node, className, nodeName, transforms) { \
	xmlNodeList transformNodes; \
	FindChildrenByType(node, nodeName, transformNodes); \
	for (xmlNodeList::iterator itT = transformNodes.begin(); itT != transformNodes.end(); ++itT) \
	{ \
		if (IsEquivalent((*itT)->name, nodeName)) { \
			className* transform = new className(GetDocument(), NULL); \
			transforms.push_back(transform); \
			status = transform->LoadFromXML(*itT); \
			if (!status) { \
				FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_NODE_TRANSFORM, (*itT)->line);} \
		} \
	}  }



	//Reference-frame body
	xmlNode* referenceBodyNode = FindChildByType(physicsRigidConstraintNode, DAE_REF_ATTACHMENT_ELEMENT);
	if (referenceBodyNode == NULL)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_RF_NODE_MISSING, physicsRigidConstraintNode->line);
	}
	fm::string strRigidBody = ReadNodeProperty(referenceBodyNode, DAE_RIGID_BODY_ELEMENT);
	referenceRigidBody = parent->FindRigidBodyFromSid(strRigidBody);
	if (referenceRigidBody == NULL)
	{
		referenceNode = GetDocument()->FindSceneNode(strRigidBody);
		if (referenceNode == NULL)
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_RF_REF_NODE_MISSING, physicsRigidConstraintNode->line);
		}
	}
	// Parse the node's transforms: <rotate>, <translate>
	PARSE_TRANSFORM(referenceBodyNode, FCDTRotation, DAE_ROTATE_ELEMENT, transformsRef)
	PARSE_TRANSFORM(referenceBodyNode, FCDTTranslation, DAE_TRANSLATE_ELEMENT, transformsRef)

	// target body
	xmlNode* bodyNode = FindChildByType(physicsRigidConstraintNode, DAE_ATTACHMENT_ELEMENT);
	if (bodyNode == NULL)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_TARGET_BS_NODE_MISSING, physicsRigidConstraintNode->line);
	}
	strRigidBody = ReadNodeProperty(bodyNode, DAE_RIGID_BODY_ELEMENT);
	targetRigidBody = parent->FindRigidBodyFromSid(strRigidBody);
	if (targetRigidBody == NULL)
	{
		targetNode = GetDocument()->FindSceneNode(strRigidBody);
		if (targetNode == NULL)
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_TARGE_BS_REF_NODE_MISSING, bodyNode->line);
		}
	}
	// Parse the node's transforms: <rotate>, <scale>, <translate>
	PARSE_TRANSFORM(bodyNode, FCDTRotation, DAE_ROTATE_ELEMENT, transformsTar)
	PARSE_TRANSFORM(bodyNode, FCDTTranslation, DAE_TRANSLATE_ELEMENT, transformsTar)

#undef PARSE_TRANSFORM

	//technique_common
	xmlNode* techniqueNode = FindChildByType(physicsRigidConstraintNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	if (techniqueNode == NULL)
	{
		//return status.Fail(FS("Technique node not specified in rigid_constraint ") + TO_FSTRING(GetDaeId()), physicsRigidConstraintNode->line);
		FUError::Error(FUError::ERROR, FUError::ERROR_TECHNIQUE_NODE_MISSING, physicsRigidConstraintNode->line);
		return status;
	}

	xmlNode* enabledNode = FindChildByType(techniqueNode, DAE_ENABLED_ELEMENT);
	if (enabledNode != NULL)
	{
		enabled = FUStringConversion::ToBoolean(ReadNodeContentDirect(enabledNode));
		FCDAnimatedFloat::Create(GetDocument(), enabledNode, (float*)&enabled);
	}
	xmlNode* interpenetrateNode = FindChildByType(techniqueNode, DAE_INTERPENETRATE_ELEMENT);
	if (interpenetrateNode != NULL)
	{
		interpenetrate = FUStringConversion::ToBoolean(ReadNodeContentDirect(interpenetrateNode));
		FCDAnimatedFloat::Create(GetDocument(), interpenetrateNode, 
				(float*)&interpenetrate);
	}

	xmlNode* limitsNode = FindChildByType(techniqueNode, DAE_LIMITS_ELEMENT);
	if (limitsNode != NULL)
	{
		xmlNode* linearNode = FindChildByType(limitsNode, DAE_LINEAR_ELEMENT);
		if (linearNode != NULL)
		{
			xmlNode* linearMinNode = FindChildByType(linearNode, DAE_MIN_ELEMENT);
			if (linearMinNode != NULL)
			{
				const char* min = ReadNodeContentDirect(linearMinNode);
				limitsLinearMin = FUStringConversion::ToVector3(min);
			}
			xmlNode* linearMaxNode = FindChildByType(linearNode, DAE_MAX_ELEMENT);
			if (linearMaxNode != NULL)
			{
				const char* max = ReadNodeContentDirect(linearMaxNode);
				limitsLinearMax = FUStringConversion::ToVector3(max);
			}
		}

		xmlNode* sctNode = FindChildByType(limitsNode, DAE_SWING_CONE_AND_TWIST_ELEMENT);
		if (sctNode != NULL)
		{
			xmlNode* sctMinNode = FindChildByType(sctNode, DAE_MIN_ELEMENT);
			if (sctMinNode != NULL)
			{
				const char* min = ReadNodeContentDirect(sctMinNode);
				limitsSCTMin = FUStringConversion::ToVector3(min);
			}
			xmlNode* sctMaxNode = FindChildByType(sctNode, DAE_MAX_ELEMENT);
			if (sctMaxNode != NULL)
			{
				const char* max = ReadNodeContentDirect(sctMaxNode);
				limitsSCTMax = FUStringConversion::ToVector3(max);
			}
		}
	}

	xmlNode* spring = FindChildByType(physicsRigidConstraintNode, DAE_SPRING_ELEMENT);
	if (spring)
	{
		xmlNode* linearSpring = FindChildByType(spring, DAE_LINEAR_ELEMENT);
		if (linearSpring)
		{
			xmlNode* param = FindChildByType(linearSpring, DAE_DAMPING_ELEMENT);
			if (param) springLinearDamping = FUStringConversion::ToFloat(ReadNodeContentDirect(param));
			param = FindChildByType(linearSpring, DAE_STIFFNESS_ELEMENT);
			if (param) springLinearStiffness = FUStringConversion::ToFloat(ReadNodeContentDirect(param));
			param = FindChildByType(linearSpring, DAE_TARGET_VALUE_ELEMENT);
			if (!param) param = FindChildByType(linearSpring, DAE_REST_LENGTH_ELEMENT1_3); // COLLADA 1.3 backward compatibility
			if (param) springLinearTargetValue = FUStringConversion::ToFloat(ReadNodeContentDirect(param));
		}

		xmlNode* angularSpring = FindChildByType(spring, DAE_ANGULAR_ELEMENT);
		if (angularSpring)
		{
			xmlNode* param = FindChildByType(angularSpring, DAE_DAMPING_ELEMENT);
			if (param) springAngularDamping = FUStringConversion::ToFloat(ReadNodeContentDirect(param));
			param = FindChildByType(angularSpring, DAE_STIFFNESS_ELEMENT);
			if (param) springAngularStiffness = FUStringConversion::ToFloat(ReadNodeContentDirect(param));
			param = FindChildByType(angularSpring, DAE_TARGET_VALUE_ELEMENT);
			if (!param) param = FindChildByType(angularSpring, DAE_REST_LENGTH_ELEMENT1_3); // COLLADA 1.3 backward compatibility
			if (param) springAngularTargetValue = FUStringConversion::ToFloat(ReadNodeContentDirect(param));
		}
	}

	SetDirtyFlag();
	return status;
}

FCDAnimated* FCDPhysicsRigidConstraint::GetAnimatedEnabled()
{
	// Returns a FCDAnimatedFloat
	return GetDocument()->FindAnimatedValue((float*)&enabled);
}

FCDAnimated* FCDPhysicsRigidConstraint::GetAnimatedInterpenetrate()
{
	// Returns a FCDAnimatedFloat
	return GetDocument()->FindAnimatedValue((float*)&interpenetrate);
}

const FCDAnimated* FCDPhysicsRigidConstraint::GetAnimatedEnabled() const
{
	// Returns a FCDAnimatedFloat
	return GetDocument()->FindAnimatedValue((float*)&enabled);
}

const FCDAnimated* FCDPhysicsRigidConstraint::GetAnimatedInterpenetrate() const
{
	// Returns a FCDAnimatedFloat
	return GetDocument()->FindAnimatedValue((float*)&interpenetrate);
}

// Write out the <physicsRigidConstraint> node
xmlNode* FCDPhysicsRigidConstraint::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* physicsRigidConstraintNode = WriteToEntityXML(parentNode, DAE_RIGID_CONSTRAINT_ELEMENT, false);

	const_cast<FCDPhysicsRigidConstraint*>(this)->SetSubId(AddNodeSid(physicsRigidConstraintNode, sid.c_str()));

	xmlNode* refNode = AddChild(physicsRigidConstraintNode, DAE_REF_ATTACHMENT_ELEMENT);
	fm::string referenceId = (referenceRigidBody != NULL) ? referenceRigidBody->GetSubId() : (referenceNode != NULL) ? referenceNode->GetDaeId() : "";
	AddAttribute(refNode, DAE_RIGID_BODY_ELEMENT, referenceId);
	for (FCDTransformContainer::const_iterator itT = transformsRef.begin(); itT != transformsRef.end(); ++itT)
	{
		(*itT)->LetWriteToXML(refNode);
	}

	xmlNode* tarNode = AddChild(physicsRigidConstraintNode, DAE_ATTACHMENT_ELEMENT);
	fm::string targetId = (targetRigidBody != NULL) ? targetRigidBody->GetSubId() : (targetNode != NULL) ? targetNode->GetDaeId() : "";
	AddAttribute(tarNode, DAE_RIGID_BODY_ELEMENT, targetId);
	for (FCDTransformContainer::const_iterator itT = transformsTar.begin(); itT != transformsTar.end(); ++itT)
	{
		(*itT)->LetWriteToXML(tarNode);
	}

	xmlNode* baseNode = AddChild(physicsRigidConstraintNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	xmlNode* enabledNode = AddChild(baseNode, DAE_ENABLED_ELEMENT, enabled);
	if (GetAnimatedEnabled())
	{
		GetDocument()->WriteAnimatedValueToXML(
				GetAnimatedEnabled(), enabledNode, 
				!GetSubId().empty() ? GetSubId().c_str() : "constrain");
	}
	xmlNode* interpenetrateNode = 
			AddChild(baseNode, DAE_INTERPENETRATE_ELEMENT, interpenetrate);
	if (GetAnimatedInterpenetrate())
	{
		GetDocument()->WriteAnimatedValueToXML(
				GetAnimatedInterpenetrate(), interpenetrateNode, 
				!GetSubId().empty() ? GetSubId().c_str() : "interpenetrate");
	}
	xmlNode* limitsNode = AddChild(baseNode, DAE_LIMITS_ELEMENT);
	
	xmlNode* sctNode = AddChild(limitsNode, DAE_SWING_CONE_AND_TWIST_ELEMENT);
	fm::string s = FUStringConversion::ToString(limitsSCTMin);
	AddChild(sctNode, DAE_MIN_ELEMENT, s);
	s = FUStringConversion::ToString(limitsSCTMax);
	AddChild(sctNode, DAE_MAX_ELEMENT, s);

	xmlNode* linearNode = AddChild(limitsNode, DAE_LINEAR_ELEMENT);
	s = FUStringConversion::ToString(limitsLinearMin);
	AddChild(linearNode, DAE_MIN_ELEMENT, s);
	s = FUStringConversion::ToString(limitsLinearMax);
	AddChild(linearNode, DAE_MAX_ELEMENT, s);

	xmlNode* springNode = AddChild(baseNode, DAE_SPRING_ELEMENT);
	xmlNode* sAngularNode = AddChild(springNode, DAE_ANGULAR_ELEMENT);
	AddChild(sAngularNode, DAE_STIFFNESS_ELEMENT, springAngularStiffness);
	AddChild(sAngularNode, DAE_DAMPING_ELEMENT, springAngularDamping);
	AddChild(sAngularNode, DAE_TARGET_VALUE_ELEMENT, springAngularTargetValue);

	xmlNode* sLinearNode = AddChild(springNode, DAE_LINEAR_ELEMENT);
	AddChild(sLinearNode, DAE_STIFFNESS_ELEMENT, springLinearStiffness);
	AddChild(sLinearNode, DAE_DAMPING_ELEMENT, springLinearDamping);
	AddChild(sLinearNode, DAE_TARGET_VALUE_ELEMENT, springLinearTargetValue);
	
	//FIXME: what about <technique> and <extra>?
	FCDEntity::WriteToExtraXML(physicsRigidConstraintNode);
	return physicsRigidConstraintNode;
}