/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDPhysicsRigidBodyParameters.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDPhysicsMaterial.h"
#include "FCDocument/FCDPhysicsShape.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDPhysicsRigidBodyParameters
//

FCDPhysicsRigidBodyParameters::FCDPhysicsRigidBodyParameters(FCDPhysicsRigidBody* _owner)
:	ownsPhysicsMaterial(false), dynamic(true)
,	mass(1.0f), inertia(FMVector3::Zero)
,	massFrameTranslate(FMVector3::Zero), massFrameRotateAxis(FMVector3::XAxis), massFrameRotateAngle(0.0f)
{
	owner = _owner;
	entityOwner = _owner;
	isDensityMoreAccurate = false;
	density = 0.0f;
}

FCDPhysicsRigidBodyParameters::FCDPhysicsRigidBodyParameters(FCDPhysicsRigidBodyInstance* _owner)
:	ownsPhysicsMaterial(false), dynamic(true)
,	mass(1.0f), inertia(FMVector3::Zero)
,	massFrameTranslate(FMVector3::Zero), massFrameRotateAxis(FMVector3::XAxis), massFrameRotateAngle(0.0f)
{
	owner = _owner;
	instanceOwner = _owner;
}

FCDPhysicsRigidBodyParameters::~FCDPhysicsRigidBodyParameters()
{
	SetPhysicsMaterial(NULL);
	SAFE_RELEASE(instanceMaterialRef);
	if (ownsPhysicsMaterial)
	{
		SAFE_RELEASE(physicsMaterial);
	}
	else
	{
		physicsMaterial = NULL;
	}
}

void FCDPhysicsRigidBodyParameters::CopyFrom(
		const FCDPhysicsRigidBodyParameters& original)
{
	// copy everything except owner since this is set already in constructor

	dynamic = original.dynamic;
	mass = original.mass;
	inertia = original.inertia;
	massFrameTranslate = original.massFrameTranslate;
	massFrameRotateAxis = original.massFrameRotateAxis;
	massFrameRotateAngle = original.massFrameRotateAngle;

	for (FCDPhysicsShapeContainer::const_iterator it = 
			original.physicsShape.begin(); it != original.physicsShape.end();
			++it)
	{
		FCDPhysicsShape* clonedShape = AddPhysicsShape();
		(*it)->Clone(clonedShape);
	}

	if (original.physicsMaterial != NULL)
	{
		if (owner->IsLocal(original.owner))
		{
			SetPhysicsMaterial(const_cast<FCDPhysicsMaterial*>
					(&*original.physicsMaterial));
		}
		else
		{
			FCDPhysicsMaterial* clonedMaterial = AddOwnPhysicsMaterial();
			original.physicsMaterial->Clone(clonedMaterial);
		}
	}

	// Clone the material instance
	if (original.instanceMaterialRef != NULL)
	{
		instanceMaterialRef = original.instanceMaterialRef->Clone();
	}
}

FCDPhysicsShape* FCDPhysicsRigidBodyParameters::AddPhysicsShape()
{
	FCDPhysicsShape* shape = physicsShape.Add(owner->GetDocument());
	owner->SetDirtyFlag();
	return shape;
}

void FCDPhysicsRigidBodyParameters::SetPhysicsMaterial(
		FCDPhysicsMaterial* _physicsMaterial)
{
	if (physicsMaterial && ownsPhysicsMaterial)
	{
		SAFE_RELEASE(physicsMaterial);
	}

	physicsMaterial = _physicsMaterial;
	ownsPhysicsMaterial = false;
	owner->SetDirtyFlag();
}

FCDPhysicsMaterial* FCDPhysicsRigidBodyParameters::AddOwnPhysicsMaterial()
{
	if (physicsMaterial && ownsPhysicsMaterial)
	{
		SAFE_RELEASE(physicsMaterial);
	}

	physicsMaterial = new FCDPhysicsMaterial(owner->GetDocument());
	ownsPhysicsMaterial = true;
	owner->SetDirtyFlag();
	return physicsMaterial;
}

// Load from a XML node the given physicsRigidBody
bool FCDPhysicsRigidBodyParameters::LoadFromXML(xmlNode* techniqueNode,
		FCDPhysicsRigidBodyParameters* defaultParameters)
{
	FCDocument* document = owner->GetDocument();
	bool status = true;

	xmlNode* param = FindChildByType(techniqueNode, DAE_DYNAMIC_ELEMENT);
	if (param)
	{
		dynamic = FUStringConversion::ToBoolean(ReadNodeContentDirect(param));
		FCDAnimatedFloat::Create(document, param, (float*)&dynamic);
	}
	else if (defaultParameters != NULL)
	{
		dynamic = defaultParameters->GetDynamic();
		FCDAnimatedFloat::Create(document, param, (float*)&dynamic);
	}

	xmlNode* massFrame;
	massFrame = FindChildByType(techniqueNode, DAE_MASS_FRAME_ELEMENT);
	if (massFrame)
	{
		param = FindChildByType(massFrame, DAE_TRANSLATE_ELEMENT);
		if (param)
		{
			massFrameTranslate = FUStringConversion::ToVector3(
					ReadNodeContentDirect(param));
		}
		else if (defaultParameters != NULL)
		{
			massFrameTranslate = defaultParameters->GetMassFrameTranslate();
		}
		else
		{
			// no movement
			massFrameTranslate = FMVector3::Zero;
		}
		FCDAnimatedPoint3::Create(document, param, &massFrameTranslate);

		param = FindChildByType(massFrame, DAE_ROTATE_ELEMENT);
		if (param)
		{
			FMVector4 temp = FUStringConversion::ToVector4(
					ReadNodeContentDirect(param));
			massFrameRotateAxis.x = temp.x;
			massFrameRotateAxis.y = temp.y;
			massFrameRotateAxis.z = temp.z;
			massFrameRotateAngle = temp.w;
		}
		else if (defaultParameters != NULL)
		{
			massFrameRotateAxis = defaultParameters->GetMassFrameRotateAxis();
		}
		else
		{
			// no movement
			massFrameRotateAxis = FMVector3::XAxis;
			massFrameRotateAngle = 0.0f;
		}
		FCDAnimatedAngleAxis::Create(document, param, &massFrameRotateAxis, 
				&massFrameRotateAngle);
	}
	else if (defaultParameters != NULL)
	{
		massFrameTranslate = defaultParameters->GetMassFrameTranslate();
		massFrameRotateAxis = defaultParameters->GetMassFrameRotateAxis();
		FCDAnimatedPoint3::Create(document, param, &massFrameTranslate);
		FCDAnimatedAngleAxis::Create(document, param, &massFrameRotateAxis, 
				&massFrameRotateAngle);
	}
	else
	{
		// no movement
		massFrameTranslate = FMVector3::Zero;
		massFrameRotateAxis = FMVector3::XAxis;
		massFrameRotateAngle = 0.0f;
		FCDAnimatedPoint3::Create(document, param, &massFrameTranslate);
		FCDAnimatedAngleAxis::Create(document, param, &massFrameRotateAxis, 
				&massFrameRotateAngle);
	}

	xmlNodeList shapeNodes;
	FindChildrenByType(techniqueNode, DAE_SHAPE_ELEMENT, shapeNodes);
	if (shapeNodes.empty())
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_SHAPE_NODE_MISSING, 
				techniqueNode->line);
	}
	for (xmlNodeList::iterator itS = shapeNodes.begin(); itS != shapeNodes.end(); ++itS)
	{
		FCDPhysicsShape* shape = AddPhysicsShape();
		status &= (shape->LoadFromXML(*itS));
	}
	// shapes are not taken from the default parameters

	param = FindChildByType(techniqueNode, DAE_PHYSICS_MATERIAL_ELEMENT);
	if (param) 
	{
		FCDPhysicsMaterial* material = AddOwnPhysicsMaterial();
		material->LoadFromXML(param);
	}
	else
	{
		param = FindChildByType(techniqueNode, 
				DAE_INSTANCE_PHYSICS_MATERIAL_ELEMENT);
		if (param != NULL)
		{
			instanceMaterialRef = FCDEntityInstanceFactory::CreateInstance(
					owner->GetDocument(), NULL, FCDEntity::PHYSICS_MATERIAL);
			instanceMaterialRef->LoadFromXML(param);
			FCDPhysicsMaterial* material = (FCDPhysicsMaterial*) 
					instanceMaterialRef->GetEntity();
			if (material == NULL)
			{
				FUError::Error(FUError::ERROR, FUError::WARNING_MISSING_URI_TARGET, param->line);
			}
			SetPhysicsMaterial(material);
		}
		else
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_PHYS_MAT_DEF_MISSING, techniqueNode->line);
		}
	}
	// material is not taken fromt he default parameters

	param = FindChildByType(techniqueNode, DAE_MASS_ELEMENT);
	if (param)
	{
		mass = FUStringConversion::ToFloat(ReadNodeContentDirect(param));
		isDensityMoreAccurate = false;
		density = 0.0f;
	}
	else if (defaultParameters != NULL)
	{
		mass = defaultParameters->GetMass();
		density = defaultParameters->GetDensity();
		isDensityMoreAccurate = defaultParameters->IsDensityMoreAccurate();
	}
	else
	{
		/* Default value for mass is density x total shape volume, but 
		   since our shape's mass is already calculated with respect to the
		   volume, we can just read it from there. If the user specified a 
		   mass, then this overrides the calculation of density x volume, 
		   as expected. */
		mass = 0.0f;
		float totalDensity = 0.0f;
		isDensityMoreAccurate = false;
		for (FCDPhysicsShapeContainer::iterator it = 
				physicsShape.begin(); it != physicsShape.end(); it++)
		{
			mass += (*it)->GetMass();
			totalDensity += (*it)->GetDensity();
			isDensityMoreAccurate |= (*it)->IsDensityMoreAccurate(); // common case: 1 shape, density = 1.0f
		}
		density = totalDensity / physicsShape.size();
	}
	FCDAnimatedFloat::Create(document, param, &mass);

	param = FindChildByType(techniqueNode, DAE_INERTIA_ELEMENT);
	if (param) 
	{
		inertia = FUStringConversion::ToVector3(
				ReadNodeContentDirect(param));
	}
	else if (defaultParameters != NULL)
	{
		inertia = defaultParameters->GetInertia();
	}
	else
	{
		/* FIXME: Approximation: sphere shape, with mass distributed 
		   equally across the volume and center of mass is at the center of
		   the sphere. Real moments of inertia call for complex 
		   integration. Sphere it is simply I = k * m * r^2 on all axes. */
		float volume = 0.0f;
		for (FCDPhysicsShapeContainer::iterator it = 
				physicsShape.begin(); it != physicsShape.end(); it++)
		{
			volume += (*it)->CalculateVolume();
		}

		float radiusCubed = 0.75f * volume / (float)FMath::Pi;
		float I = 0.4f * mass * pow(radiusCubed, 2.0f / 3.0f);
		inertia.x = I;
		inertia.y = I;
		inertia.z = I;
	}
	FCDAnimatedPoint3::Create(document, param, &inertia);

	return status;
}

// Write out the <rigid_body> node
void FCDPhysicsRigidBodyParameters::WriteToXML(xmlNode* techniqueNode) const
{	
	AddPhysicsParameter<float>(techniqueNode, DAE_DYNAMIC_ELEMENT, dynamic);
	AddPhysicsParameter<float>(techniqueNode, DAE_MASS_ELEMENT, mass);
	xmlNode* massFrameNode = AddChild(techniqueNode, DAE_MASS_FRAME_ELEMENT);
	AddPhysicsParameter<FMVector3>(massFrameNode, DAE_TRANSLATE_ELEMENT, massFrameTranslate);
	FMVector4 massFrameRotate(massFrameRotateAxis, massFrameRotateAngle);
	AddPhysicsParameter<FMVector4>(massFrameNode, DAE_ROTATE_ELEMENT, massFrameRotate);
	AddPhysicsParameter<FMVector3>(techniqueNode, DAE_INERTIA_ELEMENT, inertia);

	if (physicsMaterial != NULL)
	{
		if (ownsPhysicsMaterial)
		{
			physicsMaterial->LetWriteToXML(techniqueNode);
		}
		else if (instanceMaterialRef != NULL)
		{
			instanceMaterialRef->LetWriteToXML(techniqueNode);
		}
		else
		{
			xmlNode* instanceNode = AddChild(techniqueNode, 
					DAE_INSTANCE_PHYSICS_MATERIAL_ELEMENT);
			AddAttribute(instanceNode, DAE_URL_ATTRIBUTE, 
					fm::string("#") + physicsMaterial->GetDaeId());
		}
	}

	for (FCDPhysicsShapeContainer::const_iterator it = physicsShape.begin(); 
			it != physicsShape.end(); ++it)
	{
		(*it)->LetWriteToXML(techniqueNode);
	}
}

template <typename T>
xmlNode* FCDPhysicsRigidBodyParameters::AddPhysicsParameter(
		xmlNode* parentNode, const char* name, const T& value) const
{
	xmlNode* paramNode = AddChild(parentNode, name);
	AddContent(paramNode, FUStringConversion::ToString(value));
	const FCDAnimated* animated = 
			owner->GetDocument()->FindAnimatedValue((float*) &value);
	if (animated && animated->HasCurve())
	{
		if (entityOwner)
		{
			entityOwner->GetDocument()->WriteAnimatedValueToXML(animated, paramNode, name);
		}
		else // instance owner
		{
			instanceOwner->GetDocument()->WriteAnimatedValueToXML(animated, paramNode, name);
		}
	}
	return paramNode;
}

