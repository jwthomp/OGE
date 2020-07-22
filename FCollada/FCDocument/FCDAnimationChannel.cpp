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
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationCurveTools.h"
#include "FCDocument/FCDAnimationMultiCurve.h"
#include "FCDocument/FCDAnimationKey.h"
#include "FUtils/FUDaeEnum.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUStringConversion.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDAnimationChannel);

FCDAnimationChannel::FCDAnimationChannel(FCDocument* document, FCDAnimation* _parent)
:	FCDObject(document), parent(_parent)
,	driverQualifier(-1)
{
}

FCDAnimationChannel::~FCDAnimationChannel()
{
	parent = NULL;
	defaultValues.clear();
}

FCDAnimationChannel* FCDAnimationChannel::Clone(FCDAnimationChannel* clone) const
{
	if (clone == NULL) clone = new FCDAnimationChannel(const_cast<FCDocument*>(GetDocument()), NULL);

	// Might not be necessary
	clone->targetPointer = targetPointer;
	clone->targetQualifier = targetQualifier;
	clone->driverPointer = driverPointer;
	clone->driverQualifier = driverQualifier;

	// Clone the curves
	for (FCDAnimationCurveContainer::const_iterator it = curves.begin(); it != curves.end(); ++it)
	{
		FCDAnimationCurve* clonedCurve = clone->AddCurve();
		(*it)->Clone(clonedCurve, false);
	}

	return clone;
}

FCDAnimationCurve* FCDAnimationChannel::AddCurve()
{
	FCDAnimationCurve* curve = curves.Add(GetDocument(), this);
	SetDirtyFlag();
	return curve;
}

// Consider this animated as the curve's driver
bool FCDAnimationChannel::LinkDriver(FCDAnimated* animated)
{
	bool driver = !driverPointer.empty();
	driver &= animated->GetTargetPointer() == driverPointer;
	driver &= driverQualifier >= 0 && (uint32) driverQualifier < animated->GetValueCount();
	if (driver)
	{
		// Retrieve the value pointer for the driver
		for (FCDAnimationCurveContainer::iterator itC = curves.begin(); itC != curves.end(); ++itC)
		{
			(*itC)->SetDriver(animated, driverQualifier);
		}
	}
	return driver;
}

bool FCDAnimationChannel::CheckDriver()
{
	bool status = true;
	if (!driverPointer.empty() && !curves.empty() && !curves.front()->HasDriver())
	{
		status = FUError::Error(FUError::ERROR, FUError::ERROR_ANIM_CURVE_DRIVER_MISSING);
	}
	return status;
}

// Load a Collada animation channel from the XML document
bool FCDAnimationChannel::LoadFromXML(xmlNode* channelNode)
{
	bool status = true;

	// Read the channel-specific ID
	fm::string daeId = ReadNodeId(channelNode);
	fm::string samplerId = ReadNodeSource(channelNode);
	ReadNodeTargetProperty(channelNode, targetPointer, targetQualifier);

	xmlNode* samplerNode = parent->FindChildById(samplerId);
	if (samplerNode == NULL || !IsEquivalent(samplerNode->name, DAE_SAMPLER_ELEMENT))
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_MISSING_ELEMENT, channelNode->line);
		return false;
	}

	// Find and process the sources
	xmlNode* inputSource = NULL, * outputSource = NULL, * inTangentSource = NULL, * outTangentSource = NULL, * tcbSource = NULL, * easeSource = NULL, * interpolationSource = NULL;
	xmlNodeList samplerInputNodes;
	fm::string inputDriver;
	FindChildrenByType(samplerNode, DAE_INPUT_ELEMENT, samplerInputNodes);
	for (size_t i = 0; i < samplerInputNodes.size(); ++i) // Don't use iterator here because we are possibly appending source nodes in the loop
	{
		xmlNode* inputNode = samplerInputNodes[i];
		fm::string sourceId = ReadNodeSource(inputNode);
		xmlNode* sourceNode = parent->FindChildById(sourceId);
		fm::string sourceSemantic = ReadNodeSemantic(inputNode);

		if (sourceSemantic == DAE_INPUT_ANIMATION_INPUT) inputSource = sourceNode;
		else if (sourceSemantic == DAE_OUTPUT_ANIMATION_INPUT) outputSource = sourceNode;
		else if (sourceSemantic == DAE_INTANGENT_ANIMATION_INPUT) inTangentSource = sourceNode;
		else if (sourceSemantic == DAE_OUTTANGENT_ANIMATION_INPUT) outTangentSource = sourceNode;
		else if (sourceSemantic == DAEFC_TCB_ANIMATION_INPUT) tcbSource = sourceNode;
		else if (sourceSemantic == DAEFC_EASE_INOUT_ANIMATION_INPUT) easeSource = sourceNode;
		else if (sourceSemantic == DAE_INTERPOLATION_ANIMATION_INPUT) interpolationSource = sourceNode;
		else if (sourceSemantic == DAEMAYA_DRIVER_INPUT) inputDriver = sourceId;
	}
	if (inputSource == NULL || outputSource == NULL)
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_MISSING_INPUT, samplerNode->line);
		return false;
	}

	// Calculate the number of curves that in contained by this channel
	xmlNode* outputAccessor = FindTechniqueAccessor(outputSource);
	fm::string accessorStrideString = ReadNodeProperty(outputAccessor, DAE_STRIDE_ATTRIBUTE);
	uint32 curveCount = FUStringConversion::ToUInt32(accessorStrideString);
	if (curveCount == 0) curveCount = 1;

	// Create the animation curves
	curves.reserve(curveCount);
	for (uint32 i = 0; i < curveCount; ++i) AddCurve();

	// Read in the animation curves
	// The input keys and interpolations are shared by all the curves
	FloatList inputs;
    ReadSource(inputSource, inputs);
	size_t keyCount = inputs.size();
	if (keyCount == 0) return true; // Valid although very boring channel.

	UInt32List interpolations; interpolations.reserve(keyCount);
	ReadSourceInterpolation(interpolationSource, interpolations);
	if (interpolations.size() < keyCount)
	{
		// Not enough interpolation types provided, so append BEZIER as many times as needed.
		interpolations.insert(interpolations.end(), keyCount - interpolations.size(), FUDaeInterpolation::FromString(""));
	}

	// Read in the interleaved outputs as floats
	fm::vector<FloatList> tempFloatArrays;
	tempFloatArrays.resize(curveCount);
	fm::pvector<FloatList> outArrays(curveCount);
	for (uint32 i = 0; i < curveCount; ++i) outArrays[i] = &tempFloatArrays[i];
	ReadSourceInterleaved(outputSource, outArrays);
	for (uint32 i = 0; i < curveCount; ++i)
	{
		// Fill in the output array with zeroes, if it was not large enough.
		if (tempFloatArrays[i].size() < keyCount)
		{
			tempFloatArrays[i].insert(tempFloatArrays[i].end(), keyCount - tempFloatArrays[i].size(), 0.0f);
		}

		// Create all the keys, on the curves, according to the interpolation types.
		for (size_t j = 0; j < keyCount; ++j)
		{
			FCDAnimationKey* key = curves[i]->AddKey((FUDaeInterpolation::Interpolation) interpolations[j]);
			key->input = inputs[j];
			key->output = tempFloatArrays[i][j];

			// Set the default values for Bezier/TCB interpolations.
			if (interpolations[j] == FUDaeInterpolation::BEZIER)
			{
				FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) key;
				float previousInput = (j == 0) ? inputs[j] - 1.0f : inputs[j-1];
				float nextInput = (j == keyCount - 1) ? inputs[j] + 1.0f : inputs[j+1];
				bkey->inTangent.x = (previousInput + 2.0f * bkey->input) / 3.0f;
				bkey->outTangent.x = (nextInput + 2.0f * bkey->input) / 3.0f;
				bkey->inTangent.y = bkey->outTangent.y = bkey->output;
			}
			else if (interpolations[j] == FUDaeInterpolation::TCB)
			{
				FCDAnimationKeyTCB* tkey = (FCDAnimationKeyTCB*) key;
				tkey->tension = tkey->continuity = tkey->bias = 0.5f;
				tkey->easeIn = tkey->easeOut = 0.0f;
			}
		}
	}
	tempFloatArrays.clear();

	// Read in the interleaved in_tangent source.
	if (inTangentSource != NULL)
	{
		fm::vector<FMVector2List> tempVector2Arrays;
		tempVector2Arrays.resize(curveCount);
		fm::pvector<FMVector2List> arrays(curveCount);
		for (uint32 i = 0; i < curveCount; ++i) arrays[i] = &tempVector2Arrays[i];

		uint32 stride = ReadSourceInterleaved(inTangentSource, arrays);
		if (stride == curveCount)
		{
			// Backward compatibility with 1D tangents.
			// Remove the relativity from the 1D tangents and calculate the second-dimension.
			for (uint32 i = 0; i < curveCount; ++i)
			{
				FMVector2List& inTangents = tempVector2Arrays[i];
				FCDAnimationKey** keys = curves[i]->GetKeys();
				size_t end = min(inTangents.size(), keyCount);
				for (size_t j = 0; j < end; ++j)
				{
					if (keys[j]->interpolation == FUDaeInterpolation::BEZIER)
					{
						FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) keys[j];
						bkey->inTangent.y = bkey->output - inTangents[j].x;
					}
				}
			}
		}
		else if (stride == curveCount * 2)
		{
			// This is the typical, 2D tangent case.
			for (uint32 i = 0; i < curveCount; ++i)
			{
				FMVector2List& inTangents = tempVector2Arrays[i];
				FCDAnimationKey** keys = curves[i]->GetKeys();
				size_t end = min(inTangents.size(), keyCount);
				for (size_t j = 0; j < end; ++j)
				{
					if (keys[j]->interpolation == FUDaeInterpolation::BEZIER)
					{
						FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) keys[j];
						bkey->inTangent = inTangents[j];
					}
				}
			}
		}
	}

	// Read in the interleaved in_tangent source.
	if (outTangentSource != NULL)
	{
		fm::vector<FMVector2List> tempVector2Arrays;
		tempVector2Arrays.resize(curveCount);
		fm::pvector<FMVector2List> arrays(curveCount);
		for (uint32 i = 0; i < curveCount; ++i) arrays[i] = &tempVector2Arrays[i];

		uint32 stride = ReadSourceInterleaved(outTangentSource, arrays);
		if (stride == curveCount)
		{
			// Backward compatibility with 1D tangents.
			// Remove the relativity from the 1D tangents and calculate the second-dimension.
			for (uint32 i = 0; i < curveCount; ++i)
			{
				FMVector2List& outTangents = tempVector2Arrays[i];
				FCDAnimationKey** keys = curves[i]->GetKeys();
				size_t end = min(outTangents.size(), keyCount);
				for (size_t j = 0; j < end; ++j)
				{
					if (keys[j]->interpolation == FUDaeInterpolation::BEZIER)
					{
						FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) keys[j];
						bkey->outTangent.y = bkey->output + outTangents[j].x;
					}
				}
			}
		}
		else if (stride == curveCount * 2)
		{
			// This is the typical, 2D tangent case.
			for (uint32 i = 0; i < curveCount; ++i)
			{
				FMVector2List& outTangents = tempVector2Arrays[i];
				FCDAnimationKey** keys = curves[i]->GetKeys();
				size_t end = min(outTangents.size(), keyCount);
				for (size_t j = 0; j < end; ++j)
				{
					if (keys[j]->interpolation == FUDaeInterpolation::BEZIER)
					{
						FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) keys[j];
						bkey->outTangent = outTangents[j];
					}
				}
			}
		}
	}

	if (tcbSource != NULL)
	{
		//Process TCB parameters
		fm::vector<FMVector3List> tempVector3Arrays;
		tempVector3Arrays.resize(curveCount);
		fm::pvector<FMVector3List> arrays(curveCount);
		for (uint32 i = 0; i < curveCount; ++i) arrays[i] = &tempVector3Arrays[i];

		ReadSourceInterleaved(tcbSource, arrays);

		for (uint32 i = 0; i < curveCount; ++i)
		{
			FMVector3List& tcbs = tempVector3Arrays[i];
			FCDAnimationKey** keys = curves[i]->GetKeys();
			size_t end = min(tcbs.size(), keyCount);
			for (size_t j = 0; j < end; ++j)
			{
				if (keys[j]->interpolation == FUDaeInterpolation::TCB)
				{
					FCDAnimationKeyTCB* tkey = (FCDAnimationKeyTCB*) keys[j];
					tkey->tension = tcbs[j].x;
					tkey->continuity = tcbs[j].y;
					tkey->bias = tcbs[j].z;
				}
			}
		}
	}

	if (easeSource != NULL)
	{
		//Process Ease-in and ease-out data
		fm::vector<FMVector2List> tempVector2Arrays;
		tempVector2Arrays.resize(curveCount);
		fm::pvector<FMVector2List> arrays(curveCount);
		for (uint32 i = 0; i < curveCount; ++i) arrays[i] = &tempVector2Arrays[i];

		ReadSourceInterleaved(easeSource, arrays);

		for (uint32 i = 0; i < curveCount; ++i)
		{
			FMVector2List& eases = tempVector2Arrays[i];
			FCDAnimationKey** keys = curves[i]->GetKeys();
			size_t end = min(eases.size(), keyCount);
			for (size_t j = 0; j < end; ++j)
			{
				if (keys[j]->interpolation == FUDaeInterpolation::TCB)
				{
					FCDAnimationKeyTCB* tkey = (FCDAnimationKeyTCB*) keys[j];
					tkey->easeIn = eases[j].x;
					tkey->easeOut = eases[j].y;
				}
			}
		}
	}

	// Read in the pre/post-infinity type
	xmlNodeList mayaParameterNodes; StringList mayaParameterNames;
	xmlNode* mayaTechnique = FindTechnique(inputSource, DAEMAYA_MAYA_PROFILE);
	FindParameters(mayaTechnique, mayaParameterNames, mayaParameterNodes);
	size_t parameterCount = mayaParameterNodes.size();
	for (size_t i = 0; i < parameterCount; ++i)
	{
		xmlNode* parameterNode = mayaParameterNodes[i];
		const fm::string& paramName = mayaParameterNames[i];
		const char* content = ReadNodeContentDirect(parameterNode);

		if (paramName == DAEMAYA_PREINFINITY_PARAMETER)
		{
			for (FCDAnimationCurveContainer::iterator itC = curves.begin(); itC != curves.end(); ++itC)
			{
				(*itC)->SetPreInfinity(FUDaeInfinity::FromString(content));
			}
		}
		else if (paramName == DAEMAYA_POSTINFINITY_PARAMETER)
		{
			for (FCDAnimationCurveContainer::iterator itC = curves.begin(); itC != curves.end(); ++itC)
			{
				(*itC)->SetPostInfinity(FUDaeInfinity::FromString(content));
			}
		}
		else
		{
			// Look for driven-key input target
			if (paramName == DAE_INPUT_ELEMENT)
			{
				fm::string semantic = ReadNodeSemantic(parameterNode);
				if (semantic == DAEMAYA_DRIVER_INPUT)
				{
					inputDriver = ReadNodeSource(parameterNode);
				}
			}
		}
	}

	if (!inputDriver.empty())
	{
		const char* driverTarget = FUDaeParser::SkipPound(inputDriver);
		if (driverTarget != NULL)
		{
			fm::string driverQualifierValue;
			FUDaeParser::SplitTarget(driverTarget, driverPointer, driverQualifierValue);
			driverQualifier = FUDaeParser::ReadTargetMatrixElement(driverQualifierValue);
			if (driverQualifier < 0) driverQualifier = 0;
		}
	}

	SetDirtyFlag();
	return status;
}

// Write out the animation curves for an animation channel to a COLLADA document
xmlNode* FCDAnimationChannel::WriteToXML(xmlNode* parentNode) const
{
	fm::string baseId = CleanId(parent->GetDaeId() + "_" + targetPointer);

	// Check for curve merging
	uint32 realCurveCount = 0;
	const FCDAnimationCurve* masterCurve = NULL;
	FCDAnimationCurveList mergingCurves;
	mergingCurves.resize(defaultValues.size());
	bool mergeCurves = true;
	for (FCDAnimationCurveContainer::const_iterator itC = curves.begin(); itC != curves.end() && mergeCurves; ++itC)
	{
		const FCDAnimationCurve* curve = (*itC);
		if (curve != NULL)
		{
			// Check that we have a default placement for this curve in the default value listing
			size_t dv;
			for (dv = 0; dv < defaultValues.size(); ++dv)
			{
				if (defaultValues[dv].curve == curve)
				{
					mergingCurves[dv] = const_cast<FCDAnimationCurve*>(curve);
					break;
				}
			}
			mergeCurves &= dv != defaultValues.size();

			// Check that the curves can be merged correctly.
			++realCurveCount;
			if (masterCurve == NULL)
			{
				masterCurve = curve;
			}
			else
			{
				// Check the infinity types, the keys and the interpolations.
				size_t curveKeyCount = curve->GetKeyCount();
				size_t masterKeyCount = masterCurve->GetKeyCount();
				mergeCurves &= masterKeyCount == curveKeyCount;
				if (!mergeCurves) break;

				for (size_t j = 0; j < curveKeyCount && mergeCurves; ++j)
				{
					const FCDAnimationKey* curveKey = curve->GetKey(j);
					const FCDAnimationKey* masterKey = masterCurve->GetKey(j);
					mergeCurves &= IsEquivalent(curveKey->input, masterKey->input);
					mergeCurves &= curveKey->interpolation == masterKey->interpolation;

					// Prevent curve having TCB interpolation from merging
					mergeCurves &= curveKey->interpolation != FUDaeInterpolation::TCB;
					mergeCurves &= masterKey->interpolation != FUDaeInterpolation::TCB;
				}
				if (!mergeCurves) break;

				mergeCurves &= curve->GetPostInfinity() == masterCurve->GetPostInfinity();
				mergeCurves &= curve->GetPreInfinity() == masterCurve->GetPreInfinity();
			}

			// Disallow the merging of any curves with a driver.
			mergeCurves &= !curve->HasDriver();
		}
	}

	if (mergeCurves && realCurveCount > 1)
	{
		// Prepare the list of default values
		FloatList values;
		StringList qualifiers;
		values.reserve(defaultValues.size());
		qualifiers.reserve(defaultValues.size());
		for (FCDAnimationChannelDefaultValueList::const_iterator itDV = defaultValues.begin(); itDV != defaultValues.end(); ++itDV)
		{
			values.push_back((*itDV).defaultValue);
			qualifiers.push_back((*itDV).defaultQualifier);
		}

		// Merge and export the curves
		FCDAnimationMultiCurve* multiCurve = FCDAnimationCurveTools::MergeCurves(mergingCurves, values, qualifiers);
		multiCurve->WriteSourceToXML(parentNode, baseId);
		multiCurve->WriteSamplerToXML(parentNode, baseId);
		multiCurve->WriteChannelToXML(parentNode, baseId, targetPointer);
		SAFE_RELEASE(multiCurve);
	}
	else
	{
		// Interlace the curve's sources, samplers and channels
		// Generate new ids for each of the curve's data sources, to avoid collision in special cases
		size_t curveCount = curves.size();
		StringList ids; ids.resize(curves.size());
		FUSStringBuilder curveId;
		for (size_t c = 0; c < curveCount; ++c)
		{
			if (curves[c] != NULL)
			{
				// Generate a valid id for this curve
				curveId.set(baseId);
				if (curves[c]->GetTargetElement() >= 0)
				{
					curveId.append('_'); curveId.append(curves[c]->GetTargetElement()); curveId.append('_');
				}
				curveId.append(curves[c]->GetTargetQualifier());
				ids[c] = CleanId(curveId.ToCharPtr());

				// Write out the curve's sources
				curves[c]->WriteSourceToXML(parentNode, ids[c]);
			}
		}
		for (size_t c = 0; c < curveCount; ++c)
		{
			if (curves[c] != NULL) curves[c]->WriteSamplerToXML(parentNode, ids[c]);
		}
		for (size_t c = 0; c < curveCount; ++c)
		{
			if (curves[c] != NULL) curves[c]->WriteChannelToXML(parentNode, ids[c], targetPointer.c_str());
		}
	}

	const_cast<FCDAnimationChannel*>(this)->defaultValues.clear();
	return parentNode;
}
