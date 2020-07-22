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
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDLight.h"
#include "FCDocument/FCDSceneNode.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDLight);

FCDLight::FCDLight(FCDocument* document) : FCDTargetedEntity(document, "Light")
{
	color = FMVector3(1.0f, 1.0f, 1.0f);
	intensity = aspectRatio = 1.0f;
	lightType = POINT;
	constantAttenuationFactor = 1.0f;
	linearAttenuationFactor = quadracticAttenuationFactor = 0.0f;
	fallOffExponent = 0.0f;
	outerAngle = fallOffAngle = 5.0f;
	penumbraAngle = 0.0f; //not used by default
	dropoff = 0.0f; //not used by default
	overshoots = true;
	defaultTargetDistance = 240.0f;
	hasMayaExtras = false;
	hasMaxExtras = false;
}

FCDLight::~FCDLight()
{
	while (!animatedValues.empty())
	{
		FCDAnimated* animated = animatedValues.back(); animatedValues.pop_back();
		size_t curveCount = animated->GetCurves().size();
		for (size_t count = curveCount; count > 0; count--)
		{
			FCDAnimationCurve* curve = animated->GetCurve(count-1);
			SAFE_RELEASE(curve);
		}
		animated->Release();
	}
}

// Load this light from the given COLLADA document's node
bool FCDLight::LoadFromXML(xmlNode* lightNode)
{
	bool status = Parent::LoadFromXML(lightNode);
	if (!status) return status;
	if (!IsEquivalent(lightNode->name, DAE_LIGHT_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_LIGHT_LIB_ELEMENT, lightNode->line);
		return status;
	}

	// Retrieve the <technique_common> element.
	xmlNode* commonTechniqueNode = FindChildByType(lightNode, DAE_TECHNIQUE_COMMON_ELEMENT);

	// Look for the <point>, <directional>, <spot> or <ambient> element under the common-profile technique
	xmlNode* lightParameterNode = NULL;
	for (xmlNode* child = commonTechniqueNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;
		if (IsEquivalent(child->name, DAE_LIGHT_POINT_ELEMENT)) { lightParameterNode = child; lightType = POINT; break; }
		else if (IsEquivalent(child->name, DAE_LIGHT_SPOT_ELEMENT)) { lightParameterNode = child; lightType = SPOT; break; }
		else if (IsEquivalent(child->name, DAE_LIGHT_AMBIENT_ELEMENT)) { lightParameterNode = child; lightType = AMBIENT; break; }
		else if (IsEquivalent(child->name, DAE_LIGHT_DIRECTIONAL_ELEMENT)) { lightParameterNode = child; lightType = DIRECTIONAL; break; }
		else
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_LT_ELEMENT, child->line);
		}
	}

	// Verify the light's basic structures are found
	if (lightParameterNode == NULL)
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_MISSING_ELEMENT, lightNode->line);
	}

	// Retrieve the common light parameters
	StringList parameterNames;
	xmlNodeList parameterNodes;
	FindParameters(lightParameterNode, parameterNames, parameterNodes);

	// Parse the common light parameters
	size_t parameterCount = parameterNodes.size();
	for (size_t i = 0; i < parameterCount; ++i)
	{
		xmlNode* parameterNode = parameterNodes[i];
		const fm::string& parameterName = parameterNames[i];
		const char* content = ReadNodeContentDirect(parameterNode);
		if (parameterName == DAE_COLOR_LIGHT_PARAMETER)
		{
			color = FUStringConversion::ToVector3(content);
			FCDAnimatedColor::Create(GetDocument(), parameterNode, &color);
		}
		else if (parameterName == DAE_CONST_ATTENUATION_LIGHT_PARAMETER)
		{
			constantAttenuationFactor = FUStringConversion::ToFloat(content);
			FCDAnimatedFloat::Create(GetDocument(), parameterNode, &constantAttenuationFactor);
		}
		else if (parameterName == DAE_LIN_ATTENUATION_LIGHT_PARAMETER)
		{
			linearAttenuationFactor = FUStringConversion::ToFloat(content);
			FCDAnimatedFloat::Create(GetDocument(), parameterNode, &linearAttenuationFactor);
		}
		else if (parameterName == DAE_QUAD_ATTENUATION_LIGHT_PARAMETER)
		{
			quadracticAttenuationFactor = FUStringConversion::ToFloat(content);
			FCDAnimatedFloat::Create(GetDocument(), parameterNode, &quadracticAttenuationFactor);
		}
		else if (parameterName == DAE_FALLOFFEXPONENT_LIGHT_PARAMETER)
		{
			fallOffExponent = FUStringConversion::ToFloat(content);
			FCDAnimatedFloat::Create(GetDocument(), parameterNode, &fallOffExponent);
		}
		else if (parameterName == DAE_FALLOFFANGLE_LIGHT_PARAMETER)
		{
			fallOffAngle = FUStringConversion::ToFloat(content);
			FCDAnimatedAngle::Create(GetDocument(), parameterNode, &fallOffAngle);
		}
		else
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_LIGHT_PROG_PARAM, parameterNode->line);
		}
	}

	// Process and remove the known extra parameters
	StringList extraParameterNames;
	FCDENodeList extraParameters;
	FCDExtra* extra = GetExtra();
	size_t techniqueCount = extra->GetDefaultType()->GetTechniqueCount();
	for (size_t t = 0; t < techniqueCount; ++t)
	{
		FCDETechnique* technique = extra->GetDefaultType()->GetTechnique(t);
		if (IsEquivalent(technique->GetProfile(), DAEMAYA_MAYA_PROFILE))
		{
			hasMayaExtras = true;
		} 
		else if (IsEquivalent(technique->GetProfile(), DAEMAX_MAX_PROFILE))
		{
			hasMaxExtras = true;
		}
		technique->FindParameters(extraParameters, extraParameterNames);
	}

	size_t extraParameterCount = extraParameters.size();
	for (size_t p = 0; p < extraParameterCount; ++p)
	{
		FCDENode* extraParameterNode = extraParameters[p];
		const fm::string& parameterName = extraParameterNames[p];
		const fchar* content = extraParameterNode->GetContent();
		float *fpValue = NULL;

		if (parameterName == DAE_FALLOFFEXPONENT_LIGHT_PARAMETER)
		{
			fpValue = &fallOffExponent;
		}
		else if (parameterName == DAE_FALLOFFANGLE_LIGHT_PARAMETER)
		{
			fpValue = &fallOffAngle;
		}
		else if (parameterName == DAE_CONST_ATTENUATION_LIGHT_PARAMETER)
		{
			fpValue = &constantAttenuationFactor;
		}
		else if (parameterName == DAE_LIN_ATTENUATION_LIGHT_PARAMETER)
		{
			fpValue = &linearAttenuationFactor;
		}
		else if (parameterName == DAE_QUAD_ATTENUATION_LIGHT_PARAMETER)
		{
			fpValue = &quadracticAttenuationFactor;
		}
		else if (parameterName == DAEFC_INTENSITY_LIGHT_PARAMETER)
		{
			fpValue = &intensity;
		}
		else if (parameterName == DAEMAX_OUTERCONE_LIGHT_PARAMETER)
		{
			fpValue = &outerAngle;
		}
		else if (parameterName == DAEMAX_OVERSHOOT_LIGHT_PARAMETER)
		{
			overshoots = FUStringConversion::ToBoolean(content);
		}
		else if (parameterName == DAEMAX_DEFAULT_TARGET_DIST_LIGHT_PARAMETER)
		{
			SetDefaultTargetDistance(FUStringConversion::ToFloat(content));
		}
		else if (parameterName == DAEMAYA_PENUMBRA_LIGHT_PARAMETER)
		{
			fpValue = &penumbraAngle;
		}
		else if (parameterName == DAEMAYA_DROPOFF_LIGHT_PARAMETER)
		{
			fpValue = &dropoff;
		}
		else if (parameterName == DAEMAX_ASPECTRATIO_LIGHT_PARAMETER)
		{
			fpValue = &aspectRatio;
		}
		else continue;

		// If we have requested a value, convert and animate it
		if (fpValue != NULL)
		{
			*fpValue = FUStringConversion::ToFloat(content);
			FCDAnimated* animation = FCDAnimatedFloat::Clone(GetDocument(), &extraParameterNode->GetAnimated()->GetDummy(), fpValue);
			// Only save animation if it is present. We expect values in the animation list to be non-null;
			if (animation != NULL) animatedValues.push_back(animation);
		}

		// We have processed this extra node: remove it from the extra tree.
		SAFE_RELEASE(extraParameterNode);
	}

	SetDirtyFlag();
	return status;
}

// Write out this light to the COLLADA XML document
xmlNode* FCDLight::WriteToXML(xmlNode* parentNode) const
{
	// Create the base light node
	xmlNode* lightNode = WriteToEntityXML(parentNode, DAE_LIGHT_ELEMENT);
	xmlNode* baseNode = AddChild(lightNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	const char* baseNodeName;
	switch (lightType)
	{
	case POINT: baseNodeName = DAE_LIGHT_POINT_ELEMENT; break;
	case SPOT: baseNodeName = DAE_LIGHT_SPOT_ELEMENT; break;
	case AMBIENT: baseNodeName = DAE_LIGHT_AMBIENT_ELEMENT; break;
	case DIRECTIONAL: baseNodeName = DAE_LIGHT_DIRECTIONAL_ELEMENT; break;
	default: baseNodeName = DAEERR_UNKNOWN_INPUT; break;
	}
	baseNode = AddChild(baseNode, baseNodeName);

	// Add the application-specific technique
	// Buffer the extra light parameters so we can remove them after the export.
	FUObjectPtr<FCDETechnique> techniqueNode = const_cast<FCDExtra*>(GetExtra())->GetDefaultType()->AddTechnique(DAE_FCOLLADA_PROFILE);
	FCDENodeList extraParameterNodes;

	// Write out the light parameters
	fm::string colorValue = FUStringConversion::ToString(color);
	xmlNode* colorNode = AddChild(baseNode, DAE_COLOR_LIGHT_PARAMETER, colorValue);
	GetDocument()->WriteAnimatedValueToXML(&color.x, colorNode, "color");
	
	if (lightType == POINT || lightType == SPOT)
	{
		xmlNode* attenuationNode = AddChild(baseNode, DAE_CONST_ATTENUATION_LIGHT_PARAMETER, constantAttenuationFactor);
		GetDocument()->WriteAnimatedValueToXML(&constantAttenuationFactor, attenuationNode, "constant_attenuation");
		attenuationNode = AddChild(baseNode, DAE_LIN_ATTENUATION_LIGHT_PARAMETER, linearAttenuationFactor);
		GetDocument()->WriteAnimatedValueToXML(&linearAttenuationFactor, attenuationNode, "linear_attenuation");
		attenuationNode = AddChild(baseNode, DAE_QUAD_ATTENUATION_LIGHT_PARAMETER, quadracticAttenuationFactor);
		GetDocument()->WriteAnimatedValueToXML(&quadracticAttenuationFactor, attenuationNode, "quadratic_attenuation");
	}
	else if (lightType == DIRECTIONAL)
	{
		FCDENode* attenuationNode = techniqueNode->AddParameter(DAE_CONST_ATTENUATION_LIGHT_PARAMETER, constantAttenuationFactor);
		attenuationNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&constantAttenuationFactor));
		extraParameterNodes.push_back(attenuationNode);
		attenuationNode = techniqueNode->AddParameter(DAE_LIN_ATTENUATION_LIGHT_PARAMETER, linearAttenuationFactor);
		attenuationNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&linearAttenuationFactor));
		extraParameterNodes.push_back(attenuationNode);
		attenuationNode = techniqueNode->AddParameter(DAE_QUAD_ATTENUATION_LIGHT_PARAMETER, quadracticAttenuationFactor);
		attenuationNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&quadracticAttenuationFactor));
		extraParameterNodes.push_back(attenuationNode);
	}

	if (lightType == SPOT)
	{
		xmlNode* falloffNode = AddChild(baseNode, DAE_FALLOFFANGLE_LIGHT_PARAMETER, fallOffAngle);
		GetDocument()->WriteAnimatedValueToXML(&fallOffAngle, falloffNode, "falloff_angle");
		falloffNode = AddChild(baseNode, DAE_FALLOFFEXPONENT_LIGHT_PARAMETER, fallOffExponent);
		GetDocument()->WriteAnimatedValueToXML(&fallOffExponent, falloffNode, "falloff_exponent");
	}
	else if (lightType == DIRECTIONAL)
	{
		FCDENode* falloffNode = techniqueNode->AddParameter(DAE_FALLOFFANGLE_LIGHT_PARAMETER, fallOffAngle);
		falloffNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&fallOffAngle));
		extraParameterNodes.push_back(falloffNode);
		falloffNode = techniqueNode->AddParameter(DAE_FALLOFFEXPONENT_LIGHT_PARAMETER, fallOffExponent);
		falloffNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&fallOffExponent));
		extraParameterNodes.push_back(falloffNode);
	}

	FCDENode* intensityNode = techniqueNode->AddParameter(DAEFC_INTENSITY_LIGHT_PARAMETER, intensity);
	intensityNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&intensity));
	extraParameterNodes.push_back(intensityNode);
		
	if (lightType == DIRECTIONAL || lightType == SPOT)
	{
		FCDENode* outerAngleNode = techniqueNode->AddParameter(DAEMAX_OUTERCONE_LIGHT_PARAMETER, outerAngle);
		outerAngleNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&outerAngle));
		extraParameterNodes.push_back(outerAngleNode);
		FCDENode* aspectRatioNode = techniqueNode->AddParameter(DAEMAX_ASPECTRATIO_LIGHT_PARAMETER, aspectRatio);
		aspectRatioNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&aspectRatio));
		extraParameterNodes.push_back(aspectRatioNode);
	}

	if (lightType == DIRECTIONAL)
	{
		FCDENode* overshootsNode = techniqueNode->AddParameter(DAEMAX_OVERSHOOT_LIGHT_PARAMETER, overshoots);
		extraParameterNodes.push_back(overshootsNode);
	}

	if (lightType == SPOT)
	{
		FCDENode* dropoffNode = techniqueNode->AddParameter(DAEMAYA_DROPOFF_LIGHT_PARAMETER, dropoff);
		dropoffNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&aspectRatio));
		extraParameterNodes.push_back(dropoffNode);
		FCDENode* penumbraAngleNode = techniqueNode->AddParameter(DAEMAYA_PENUMBRA_LIGHT_PARAMETER, penumbraAngle);
		penumbraAngleNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&aspectRatio));
		extraParameterNodes.push_back(penumbraAngleNode);
	}

	if (GetTargetNode() == NULL)
	{
		FCDENode* targetDistanceNode = techniqueNode->AddParameter(DAEMAX_DEFAULT_TARGET_DIST_LIGHT_PARAMETER, defaultTargetDistance);
		extraParameterNodes.push_back(targetDistanceNode);
	}

	// Export the <extra> elements and release the temporarily-added parameters/technique
	Parent::WriteToExtraXML(lightNode);
	CLEAR_POINTER_VECTOR(extraParameterNodes);
	if (techniqueNode != NULL && techniqueNode->GetChildNodeCount() == 0) SAFE_RELEASE(techniqueNode);
	return lightNode;
}
