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
#include "FCDocument/FCDCamera.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDSceneNode.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDCamera);

FCDCamera::FCDCamera(FCDocument* document) : FCDTargetedEntity(document, "Camera")
{
	isPerspective = true;
	isOrthographic = false;
	viewY = viewX = 60.0f;
	hasAperture = hasHorizontalView = hasVerticalView = false;
	nearZ = 1.0f;
	farZ = 1000.0f;
	aspectRatio = 1.0f;
	horizontalAperture = verticalAperture = lensSqueeze = 1.0f;
}

FCDCamera::~FCDCamera()
{
}

void FCDCamera::SetFovX(float _viewX)
{
	viewX = _viewX;
	if (hasVerticalView && !IsEquivalent(viewX, 0.0f)) aspectRatio = viewX / viewY;
	hasHorizontalView = true;
	SetDirtyFlag(); 
}

void FCDCamera::SetFovY(float _viewY)
{
	viewY = _viewY;
	if (hasHorizontalView && !IsEquivalent(viewX, 0.0f)) aspectRatio = viewX / viewY;
	hasVerticalView = true;
	SetDirtyFlag(); 
}

void FCDCamera::SetAspectRatio(float _aspectRatio)
{
	aspectRatio = _aspectRatio;
	SetDirtyFlag(); 
}

// Load this camera from the given COLLADA document's node
bool FCDCamera::LoadFromXML(xmlNode* cameraNode)
{
	bool status = Parent::LoadFromXML(cameraNode);
	if (!status) return status;
	if (!IsEquivalent(cameraNode->name, DAE_CAMERA_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_CAM_ELEMENT, cameraNode->line);
		return status;
	}

	FCDExtra* extra = GetExtra();

	// COLLADA 1.4: Grab the <optics> element's techniques
	xmlNode* opticsNode = FindChildByType(cameraNode, DAE_OPTICS_ELEMENT);
	xmlNode* commonTechniqueNode = FindChildByType(opticsNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	if (opticsNode != NULL) extra->LoadFromXML(opticsNode); // backward compatibility-only, the extra information has been moved into the <camera><extra> element.

	// Retrieve the <perspective> or <orthographic> element
	xmlNode* cameraContainerNode = FindChildByType(commonTechniqueNode, DAE_CAMERA_ORTHO_ELEMENT);
	isOrthographic = cameraContainerNode != NULL;
	if (!isOrthographic)
	{
		cameraContainerNode = FindChildByType(commonTechniqueNode, DAE_CAMERA_PERSP_ELEMENT);
		isPerspective = cameraContainerNode != NULL;
	}
	else
	{
		isPerspective = false;
	}

	// Check the necessary camera structures
	if (cameraContainerNode == NULL)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_PARAM_ROOT_MISSING, cameraNode->line);
		return status;
	}
	if (!(isPerspective ^ isOrthographic))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_CAM_PROG_TYPE, cameraContainerNode->line);
		return status;
	}

	// Setup the camera according to the type and its parameters
	// Retrieve all the camera parameters
	StringList parameterNames;
	xmlNodeList parameterNodes;
	FindParameters(cameraContainerNode, parameterNames, parameterNodes);

	size_t parameterCount = parameterNodes.size();
	for (size_t i = 0; i < parameterCount; ++i)
	{
		xmlNode* parameterNode = parameterNodes[i];
		const fm::string& parameterName = parameterNames[i];
		const char* parameterValue = ReadNodeContentDirect(parameterNode);

#define COMMON_CAM_PARAMETER(colladaParam, memberFunction, animatedMember) \
		if (parameterName == colladaParam) { \
			memberFunction(FUStringConversion::ToFloat(parameterValue)); \
			FCDAnimatedFloat::Create(GetDocument(), parameterNode, &animatedMember); } else

		// Process the camera parameters
		COMMON_CAM_PARAMETER(DAE_ZNEAR_CAMERA_PARAMETER, SetNearZ, nearZ)
		COMMON_CAM_PARAMETER(DAE_ZFAR_CAMERA_PARAMETER, SetFarZ, farZ)
		COMMON_CAM_PARAMETER(DAE_XFOV_CAMERA_PARAMETER, SetFovX, viewX)
		COMMON_CAM_PARAMETER(DAE_YFOV_CAMERA_PARAMETER, SetFovY, viewY)
		COMMON_CAM_PARAMETER(DAE_XMAG_CAMERA_PARAMETER, SetMagX, viewX)
		COMMON_CAM_PARAMETER(DAE_YMAG_CAMERA_PARAMETER, SetMagY, viewY)
		COMMON_CAM_PARAMETER(DAE_ASPECT_CAMERA_PARAMETER, SetAspectRatio, aspectRatio)
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_CAM_PARAM, parameterNode->line);
		}

#undef COMMON_CAM_PARAMETER
	}

	// Process the known extra parameters
	StringList extraParameterNames;
	FCDENodeList extraParameterNodes;
	size_t extraTechniqueCount = extra->GetDefaultType()->GetTechniqueCount();
	for (size_t i = 0; i < extraTechniqueCount; ++i)
	{
		FCDETechnique* technique = extra->GetDefaultType()->GetTechnique(i);
		technique->FindParameters(extraParameterNodes, extraParameterNames);
	}

	size_t extraParameterCount = extraParameterNodes.size();
	for (size_t p = 0; p < extraParameterCount; ++p)
	{
		FCDENode* parameter = extraParameterNodes[p];
		const fm::string& parameterName = extraParameterNames[p];
		const fchar* parameterValue = parameter->GetContent();

#define EXTRA_CAM_PARAMETER(colladaParam, memberFunction, animatedMember) \
		if (parameterName == colladaParam) { \
			memberFunction(FUStringConversion::ToFloat(parameterValue)); \
			FCDAnimatedFloat::Clone(GetDocument(), &parameter->GetAnimated()->GetDummy(), &animatedMember); } else

		EXTRA_CAM_PARAMETER(DAEMAYA_VAPERTURE_PARAMETER, SetVerticalAperture, verticalAperture)
		EXTRA_CAM_PARAMETER(DAEMAYA_HAPERTURE_PARAMETER, SetHorizontalAperture, horizontalAperture)
		EXTRA_CAM_PARAMETER(DAEMAYA_LENSSQUEEZE_PARAMETER, SetLensSqueeze, lensSqueeze)

		continue; // implicit else from the above macro.

		SAFE_RELEASE(parameter);

#undef EXTRA_CAM_PARAMETER

	}

	SetDirtyFlag(); 
	return status;
}

// Write out this camera to the COLLADA XML document
xmlNode* FCDCamera::WriteToXML(xmlNode* parentNode) const
{
	// Create the base camera node
	xmlNode* cameraNode = WriteToEntityXML(parentNode, DAE_CAMERA_ELEMENT);
	xmlNode* opticsNode = AddChild(cameraNode, DAE_OPTICS_ELEMENT);
	xmlNode* baseNode = AddChild(opticsNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	const char* baseNodeName;
	if (isPerspective) baseNodeName = DAE_CAMERA_PERSP_ELEMENT;
	else if (isOrthographic) baseNodeName = DAE_CAMERA_ORTHO_ELEMENT;
	else baseNodeName = DAEERR_UNKNOWN_ELEMENT;
	baseNode = AddChild(baseNode, baseNodeName);

	// Write out the basic camera parameters
	const char* horizontalViewName = (isPerspective) ? DAE_XFOV_CAMERA_PARAMETER : DAE_XMAG_CAMERA_PARAMETER;
	const char* verticalViewName = (isPerspective) ? DAE_YFOV_CAMERA_PARAMETER : DAE_YMAG_CAMERA_PARAMETER;
	if (hasHorizontalView)
	{
		xmlNode* viewNode = AddChild(baseNode, horizontalViewName, viewX);
		GetDocument()->WriteAnimatedValueToXML(&viewX, viewNode, horizontalViewName);
	}
	if (!hasHorizontalView || hasVerticalView)
	{
		xmlNode* viewNode = AddChild(baseNode, verticalViewName, viewY);
		GetDocument()->WriteAnimatedValueToXML(&viewY, viewNode, verticalViewName);
	}

	// Aspect ratio: can only be exported if one of the vertical or horizontal view ratios is missing.
	if (HasAspectRatio())
	{
		xmlNode* aspectNode = AddChild(baseNode, DAE_ASPECT_CAMERA_PARAMETER, aspectRatio);
		GetDocument()->WriteAnimatedValueToXML(&aspectRatio, aspectNode, "aspect_ratio");
	}

	// Near/Far clip plane distance
	xmlNode* clipNode = AddChild(baseNode, DAE_ZNEAR_CAMERA_PARAMETER, nearZ);
	GetDocument()->WriteAnimatedValueToXML(&nearZ, clipNode, "near_clip");
	clipNode = AddChild(baseNode, DAE_ZFAR_CAMERA_PARAMETER, farZ);
	GetDocument()->WriteAnimatedValueToXML(&farZ, clipNode, "near_clip");

	// Add the application-specific technique/parameters
	FCDENodeList extraParameterNodes;
	FUObjectPtr<FCDETechnique> techniqueNode = NULL;

	// Maya has extra aperture information
	if (hasAperture)
	{
		techniqueNode = const_cast<FCDExtra*>(GetExtra())->GetDefaultType()->AddTechnique(DAE_FCOLLADA_PROFILE);
		FCDENode* apertureNode = techniqueNode->AddParameter(DAEMAYA_VAPERTURE_PARAMETER, verticalAperture);
		apertureNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&verticalAperture));
		extraParameterNodes.push_back(apertureNode);
		apertureNode = techniqueNode->AddParameter(DAEMAYA_HAPERTURE_PARAMETER, horizontalAperture);
		apertureNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&horizontalAperture));
		extraParameterNodes.push_back(apertureNode);
		apertureNode = techniqueNode->AddParameter(DAEMAYA_LENSSQUEEZE_PARAMETER, lensSqueeze);
		apertureNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&lensSqueeze));
		extraParameterNodes.push_back(apertureNode);
	}

	// Export the <extra> elements and release the temporarily-added parameters/technique
	Parent::WriteToExtraXML(cameraNode);
	CLEAR_POINTER_VECTOR(extraParameterNodes);
	if (techniqueNode != NULL && techniqueNode->GetChildNodeCount() == 0) SAFE_RELEASE(techniqueNode);
	return cameraNode;
}
