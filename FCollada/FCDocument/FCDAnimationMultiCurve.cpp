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
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationKey.h"
#include "FCDocument/FCDAnimationMultiCurve.h"
#include "FUtils/FUDaeEnum.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeWriter;

// Implemented in FCDAnimationCurve.cpp
extern float FindT(float cp0x, float cp1x, float cp2x, float cp3x, float input, float initialGuess);

// Declaring the type of evaluation for curves once.
bool FCDAnimationMultiCurve::is2DEvaluation = true;

//
// FCDAnimationMultiCurve
//

ImplementObjectType(FCDAnimationMultiCurve);

FCDAnimationMultiCurve::FCDAnimationMultiCurve(FCDocument* document, uint32 _dimension)
:	FCDObject(document), dimension(_dimension)
,	targetElement(-1), targetQualifiers(NULL)
,	preInfinity(FUDaeInfinity::CONSTANT), postInfinity(FUDaeInfinity::CONSTANT)
{
	if (dimension == 0) dimension = 1;
	
	// Prepare the target information
	targetQualifiers = new fm::string[dimension];
}

FCDAnimationMultiCurve::~FCDAnimationMultiCurve()
{
	CLEAR_POINTER_VECTOR(keys);
	SAFE_DELETE_ARRAY(targetQualifiers);
}

void FCDAnimationMultiCurve::SetKeyCount(size_t count, FUDaeInterpolation::Interpolation interpolation)
{
	size_t oldCount = GetKeyCount();
	if (oldCount < count)
	{
		keys.reserve(count);
		for (; oldCount < count; ++oldCount) AddKey(interpolation);
	}
	else if (count < oldCount)
	{
		for (FCDAnimationMKeyList::iterator it = keys.begin() + count; it != keys.end(); ++it) delete (*it);
		keys.resize(count);
	}
	SetDirtyFlag();
}

FCDAnimationMKey* FCDAnimationMultiCurve::AddKey(FUDaeInterpolation::Interpolation interpolation)
{
	FCDAnimationMKey* key;
	switch (interpolation)
	{
	case FUDaeInterpolation::STEP: key = new FCDAnimationMKey(dimension); break;
	case FUDaeInterpolation::LINEAR: key = new FCDAnimationMKey(dimension); break;
	case FUDaeInterpolation::BEZIER: key = new FCDAnimationMKeyBezier(dimension); break;
	case FUDaeInterpolation::TCB: key = new FCDAnimationMKeyTCB(dimension); break;
	default: FUFail(return NULL);
	}
	key->interpolation = (uint32) interpolation;
	keys.push_back(key);
	SetDirtyFlag();
	return key;
}

// Samples all the curves for a given input
void FCDAnimationMultiCurve::Evaluate(float input, float* output) const
{
	// Check for empty curves and poses (curves with 1 key).
	if (keys.size() == 0)
	{
		for (uint32 i = 0; i < dimension; ++i) output[i] = 0.0f;
	}
	else if (keys.size() == 1)
	{
		for (uint32 i = 0; i < dimension; ++i) output[i] = keys.front()->output[i];
	}
	else
	{
		// Find the current interval
		FCDAnimationMKeyList::const_iterator it, start = keys.begin(), terminate = keys.end();
		while (terminate - start > 3)
		{ 
			// Binary search.
			it = (const FCDAnimationMKey**) ((((size_t) terminate) / 2 + ((size_t) start) / 2) & ~((sizeof(size_t)-1)));
			if ((*it)->input > input) terminate = it;
			else start = it;
		}
		// Linear search is more efficient on the last interval
		for (it = start; it != terminate; ++it)
		{
			if ((*it)->input > input) break;
		}

		if (it == keys.end())
		{
			// We're sampling after the curve, return the last values
			const FCDAnimationMKey* lastKey = keys.back();
			for (uint32 i = 0; i < dimension; ++i) output[i] = lastKey->output[i];
		}
		else if (it == keys.begin())
		{
			// We're sampling before the curve, return the first values
			const FCDAnimationMKey* firstKey = keys.front();
			for (uint32 i = 0; i < dimension; ++i) output[i] = firstKey->output[i];
		}
		else
		{
			// Get the keys and values for this interval
			const FCDAnimationMKey* startKey = *(it - 1);
			const FCDAnimationMKey* endKey = *it;
			float inputInterval = endKey->input - startKey->input;

			// Interpolate the outputs.
			// Similar code is found in FCDAnimationCurve.cpp. If you update this, update the other one too.
			switch (startKey->interpolation)
			{
			case FUDaeInterpolation::LINEAR:
				for (uint32 i = 0; i < dimension; ++i)
				{
					output[i] = startKey->output[i] + (input - startKey->input) / inputInterval * (endKey->output[i] - startKey->output[i]); 
				}
				break;

			case FUDaeInterpolation::BEZIER: {
				FCDAnimationMKeyBezier* bkey1 = (FCDAnimationMKeyBezier*) startKey;
				for (uint32 i = 0; i < dimension; ++i)
				{
					FMVector2 inTangent;
					if (endKey->interpolation == FUDaeInterpolation::BEZIER) inTangent = ((FCDAnimationMKeyBezier*) endKey)->inTangent[i];
					else inTangent = FMVector2(endKey->input, 0.0f);

					float t = (input - startKey->input) / inputInterval;
					if (is2DEvaluation) t = FindT(startKey->input, bkey1->outTangent[i].x, inTangent.x, endKey->input, input, t);
					float b = bkey1->outTangent[i].v;
					float c = inTangent.v;
					float ti = 1.0f - t;
					float br = inputInterval / (bkey1->outTangent[i].u - startKey->input);
					float cr = inputInterval / (endKey->input - inTangent.u);
					br = FMath::Clamp(br, 0.01f, 100.0f);
					cr = FMath::Clamp(cr, 0.01f, 100.0f);

					output[i] = startKey->output[i] * ti * ti * ti + br* b * ti * ti * t + cr * c * ti * t * t + endKey->output[i] * t * t * t;
				}
				break; }

			case FUDaeInterpolation::TCB: // Not implemented..
			case FUDaeInterpolation::UNKNOWN:
			case FUDaeInterpolation::STEP:
			default:
				for (uint32 i = 0; i < dimension; ++i) output[i] = startKey->output[i];
				break;
			}
		}
	}
}

// Write out the specific animation elements to the COLLADA XML tree node
void FCDAnimationMultiCurve::WriteSourceToXML(xmlNode* parentNode, const fm::string& baseId)
{
	if (keys.empty() || dimension == 0) return;

	// Generate the list of the parameters
	typedef const char* pchar;
	pchar* parameters = new pchar[dimension];
	for (size_t i = 0; i < dimension; ++i)
	{
		parameters[i] = targetQualifiers[i].c_str();
		if (*(parameters[i]) == '.') ++(parameters[i]);
	}

	// Retrieve the source information data
	bool hasTangents = false, hasTCB = false;
	for (FCDAnimationMKeyList::iterator it = keys.begin(); it != keys.end(); ++it)
	{
		hasTangents |= (*it)->interpolation == FUDaeInterpolation::BEZIER;
		hasTCB |= (*it)->interpolation == FUDaeInterpolation::TCB;
	}

	// Prebuffer the source information lists
	size_t keyCount = keys.size();
	FloatList inputs; inputs.reserve(keyCount);
	FloatList outputs; outputs.reserve(keyCount * dimension);
	UInt32List interpolations; interpolations.reserve(keyCount);
	FloatList inTangents; if (hasTangents) inTangents.reserve(keyCount * dimension * 2);
	FloatList outTangents; if (hasTangents) outTangents.reserve(keyCount * dimension * 2);
	FloatList tcbs; if (hasTCB) tcbs.reserve(keyCount * dimension * 3);
	FloatList eases; if (hasTCB) eases.reserve(keyCount * dimension * 2);

	// Retrieve the source information data
	for (FCDAnimationMKeyList::iterator it = keys.begin(); it != keys.end(); ++it)
	{
		inputs.push_back((*it)->input);
		for (uint32 i = 0; i < dimension; ++i)
		{
			outputs.push_back((*it)->output[i]);
			if (hasTangents)
			{
				if ((*it)->interpolation == FUDaeInterpolation::BEZIER)
				{
					FCDAnimationMKeyBezier* bkey = (FCDAnimationMKeyBezier*) (*it);
					if (inTangents.size() * dimension * 2 < interpolations.size())
					{
						// Grow to the correct index
						inTangents.resize(interpolations.size() * dimension * 2, 0.0f);
						outTangents.resize(interpolations.size() * dimension * 2, 0.0f);
					}
					inTangents.push_back(bkey->inTangent[i].u);
					inTangents.push_back(bkey->inTangent[i].v);
					outTangents.push_back(bkey->outTangent[i].u);
					outTangents.push_back(bkey->outTangent[i].v);
				}
				else if ((*it)->interpolation == FUDaeInterpolation::LINEAR)
				{
					// Export interpolative tangents
					const FCDAnimationMKey* previousKey = it == keys.begin() ? NULL : *(it - 1);
					const FCDAnimationMKey* nextKey = (it + 1) == keys.end() ? NULL : *(it + 1);
					float inIntervalX = previousKey == NULL ? 1.0f : (*it)->input - previousKey->input;
					float outIntervalX = nextKey == NULL ? 1.0f : nextKey->input - (*it)->input;
					float inIntervalY = previousKey == NULL ? 0.0f : (*it)->output[i] - previousKey->output[i];
					float outIntervalY = nextKey == NULL ? 0.0f : nextKey->output[i] - (*it)->output[i];
					inTangents.push_back((*it)->input - inIntervalX / 3.0f);
					inTangents.push_back((*it)->output[i] - inIntervalY / 3.0f);
					outTangents.push_back((*it)->input + outIntervalX / 3.0f);
					outTangents.push_back((*it)->output[i] + outIntervalY / 3.0f);
				}
				else
				{
					// Export flat tangents
					inTangents.push_back((*it)->input - 0.0001f);
					inTangents.push_back((*it)->output[i]);
					outTangents.push_back((*it)->input + 0.0001f);
					outTangents.push_back((*it)->output[i]);
				}
			}
			if (hasTCB)
			{
				if ((*it)->interpolation == FUDaeInterpolation::TCB)
				{
					FCDAnimationMKeyTCB* tkey = (FCDAnimationMKeyTCB*) (*it);
					if (tcbs.size() * dimension * 3 < interpolations.size())
					{
						// Grow to the correct index
						tcbs.resize(interpolations.size() * dimension * 3, 0.0f);
						eases.resize(interpolations.size() * dimension * 2, 0.0f);
					}
					tcbs.push_back(tkey->tension[i]);
					tcbs.push_back(tkey->continuity[i]);
					tcbs.push_back(tkey->bias[i]);
					eases.push_back(tkey->easeIn[i]);
					eases.push_back(tkey->easeOut[i]);
				}
				else
				{
					// Export flat tangents
					tcbs.push_back(0.5f);
					tcbs.push_back(0.5f);
					tcbs.push_back(0.5f);
					eases.push_back(0.0f);
					eases.push_back(0.0f);
				}
			}
		}

		// Append the interpolation type last, because we use this list's size to correctly
		// index TCB and Bezier information when exporting mixed-interpolation curves.
		interpolations.push_back((*it)->interpolation);
	}

	// Export the data arrays
	xmlNode* inputSourceNode = AddSourceFloat(parentNode, baseId + "-input", inputs, "TIME");
	AddSourceFloat(parentNode, baseId + "-output", outputs, dimension, parameters);
	AddSourceInterpolation(parentNode, baseId + "-interpolations", *(FUDaeInterpolationList*) &interpolations);
	if (!inTangents.empty())
	{
		AddSourceFloat(parentNode, baseId + "-intangents", inTangents, dimension * 2, FUDaeAccessor::XY);
		AddSourceFloat(parentNode, baseId + "-outtangents", outTangents, dimension * 2, FUDaeAccessor::XY);
	}
	if (!tcbs.empty())
	{
		AddSourceFloat(parentNode, baseId + "-tcbs", tcbs, dimension * 3, FUDaeAccessor::XYZW);
		AddSourceFloat(parentNode, baseId + "-eases", eases, dimension * 2, FUDaeAccessor::XY);
	}

	SAFE_DELETE_ARRAY(parameters);

	// Export the infinity parameters
	xmlNode* mayaTechnique = AddTechniqueChild(inputSourceNode, DAEMAYA_MAYA_PROFILE);
	fm::string infinityType = FUDaeInfinity::ToString(preInfinity);
	AddChild(mayaTechnique, DAEMAYA_PREINFINITY_PARAMETER, infinityType);
	infinityType = FUDaeInfinity::ToString(postInfinity);
	AddChild(mayaTechnique, DAEMAYA_POSTINFINITY_PARAMETER, infinityType);
}

xmlNode* FCDAnimationMultiCurve::WriteSamplerToXML(xmlNode* parentNode, const fm::string& baseId)
{
	xmlNode* samplerNode = AddChild(parentNode, DAE_SAMPLER_ELEMENT);
	AddAttribute(samplerNode, DAE_ID_ATTRIBUTE, baseId + "-sampler");

	// Retrieve the source information data
	bool hasTangents = false, hasTCB = false;
	for (FCDAnimationMKeyList::iterator it = keys.begin(); it != keys.end(); ++it)
	{
		hasTangents |= (*it)->interpolation == FUDaeInterpolation::BEZIER;
		hasTCB |= (*it)->interpolation == FUDaeInterpolation::TCB;
	}

	// Add the sampler inputs
	AddInput(samplerNode, baseId + "-input", DAE_INPUT_ANIMATION_INPUT);
	AddInput(samplerNode, baseId + "-output", DAE_OUTPUT_ANIMATION_INPUT);
	AddInput(samplerNode, baseId + "-interpolations", DAE_INTERPOLATION_ANIMATION_INPUT);
	if (hasTangents)
	{
		AddInput(samplerNode, baseId + "-intangents", DAE_INTANGENT_ANIMATION_INPUT);
		AddInput(samplerNode, baseId + "-outtangents", DAE_OUTTANGENT_ANIMATION_INPUT);
	}
	if (hasTCB)
	{
		AddInput(samplerNode, baseId + "-tcbs", DAEFC_TCB_ANIMATION_INPUT);
		AddInput(samplerNode, baseId + "-eases", DAEFC_EASE_INOUT_ANIMATION_INPUT);
	}
	return samplerNode;	
}

xmlNode* FCDAnimationMultiCurve::WriteChannelToXML(xmlNode* parentNode, const fm::string& baseId, const fm::string& pointer)
{
	xmlNode* channelNode = AddChild(parentNode, DAE_CHANNEL_ELEMENT);
	AddAttribute(channelNode, DAE_SOURCE_ATTRIBUTE, fm::string("#") + baseId + "-sampler");

	// Generate and export the full target [no qualifiers]
	globalSBuilder.set(pointer);
	if (targetElement >= 0)
	{
		globalSBuilder.append('('); globalSBuilder.append(targetElement); globalSBuilder.append(')');
	}
	AddAttribute(channelNode, DAE_TARGET_ATTRIBUTE, globalSBuilder);
	return channelNode;
}
