/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationClip.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimated.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDAnimationClip);

FCDAnimationClip::FCDAnimationClip(FCDocument* document) : FCDEntity(document, "AnimationClip")
{
	start = end = 0.0f;
}

FCDAnimationClip::~FCDAnimationClip()
{
	curves.clear();
}

void FCDAnimationClip::AddClipCurve(FCDAnimationCurve* curve)
{
	curve->RegisterAnimationClip(this);
	curves.push_back(curve);
	SetDirtyFlag();
}

FCDEntity* FCDAnimationClip::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDAnimationClip* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDAnimationClip(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDAnimationClip::GetClassType())) clone = (FCDAnimationClip*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		// Copy the generic animation clip parameters
		clone->start = start;
		clone->end = end;

		// If requested, clone the animation curves as well.
		for (FCDAnimationCurveTrackList::const_iterator it = curves.begin(); it != curves.end(); ++it)
		{
			if (cloneChildren)
			{
				FCDAnimationCurve* clonedCurve = (*it)->Clone(NULL, false);
				clonedCurve->AddClip(clone);
				clone->AddClipCurve(clonedCurve);
			}
		}
	}

	return _clone;
}

bool FCDAnimationClip::LoadFromXML(xmlNode* clipNode)
{
	bool status = FCDEntity::LoadFromXML(clipNode);
	if (!status) return status;
	if (!IsEquivalent(clipNode->name, DAE_ANIMCLIP_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_ANIM_LIB_ELEMENT, clipNode->line);
		return status;
	}

	// Read in and verify the clip's time/input bounds
	start = FUStringConversion::ToFloat(ReadNodeProperty(clipNode, DAE_START_ATTRIBUTE));
	end = FUStringConversion::ToFloat(ReadNodeProperty(clipNode, DAE_END_ATTRIBUTE));
	if (end - start < FLT_TOLERANCE)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_SE_PAIR, clipNode->line);
	}

	// Read in the <input> elements and segment the corresponding animation curves
	xmlNodeList inputNodes;
	FindChildrenByType(clipNode, DAE_INSTANCE_ANIMATION_ELEMENT, inputNodes);
	for (xmlNodeList::iterator itI = inputNodes.begin(); itI != inputNodes.end(); ++itI)
	{
		xmlNode* inputNode = (*itI);

		// Retrieve the animation for this input
		FUUri animationId = ReadNodeUrl(inputNode);
		if (animationId.suffix.empty() || !animationId.prefix.empty())
		{
			FUError::Error(FUError::ERROR, FUError::WARNING_MISSING_URI_TARGET, inputNode->line);
		}
		FCDAnimation* animation = GetDocument()->FindAnimation(animationId.suffix);
		if (animation == NULL) continue;

		// Retrieve all the curves created under this animation node
		FCDAnimationCurveList animationCurves;
		animation->GetCurves(animationCurves);
		if (animationCurves.empty())
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_CURVES_MISSING, inputNode->line);
		}
        
		for (FCDAnimationCurveList::iterator itC = animationCurves.begin(); itC != animationCurves.end(); ++itC)
		{
			// Keep only newly listed curves
			FCDAnimationCurve* curve = *itC;
			FCDAnimationCurveTrackList::iterator itF = curves.find(curve);
			if (itF == curves.end())
			{
				AddClipCurve(curve);
			}
		}
	}

	// Check for an empty clip
	if (curves.empty())
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_EMPTY_ANIM_CLIP, clipNode->line);
	}

	SetDirtyFlag();
	return status;
}

// Write out the COLLADA animations to the document
xmlNode* FCDAnimationClip::WriteToXML(xmlNode* parentNode) const
{
	// Create the <clip> element and write out its start/end information.
	xmlNode* clipNode = FCDEntity::WriteToEntityXML(parentNode, DAE_ANIMCLIP_ELEMENT);
	AddAttribute(clipNode, DAE_START_ATTRIBUTE, start);
	AddAttribute(clipNode, DAE_END_ATTRIBUTE, end);

	// Build a list of the animations to instantiate
	// from the list of curves for this clip
	typedef fm::pvector<const FCDAnimation> FCDAnimationConstList;
	FCDAnimationConstList animations;
	for (FCDAnimationCurveTrackList::const_iterator itC = curves.begin(); itC != curves.end(); ++itC)
	{
		const FCDAnimationChannel* channel = (*itC)->GetParent();
		if (channel == NULL) continue;
		const FCDAnimation* animation = channel->GetParent();
		if (animations.find(animation) == animations.end())
		{
			animations.push_back(animation);
		}
	}

	// Instantiate all the animations
	for (FCDAnimationConstList::iterator itA = animations.begin(); itA != animations.end(); ++itA)
	{
		xmlNode* instanceNode = AddChild(clipNode, DAE_INSTANCE_ANIMATION_ELEMENT);
		AddAttribute(instanceNode, DAE_URL_ATTRIBUTE, fm::string("#") + (*itA)->GetDaeId());
	}

	FCDEntity::WriteToExtraXML(clipNode);
	return clipNode;
}

