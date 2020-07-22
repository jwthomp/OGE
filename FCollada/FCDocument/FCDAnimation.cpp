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
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAsset.h"
#include "FUtils/FUDaeParser.h"
using namespace FUDaeParser;

ImplementObjectType(FCDAnimation);

FCDAnimation::FCDAnimation(FCDocument* document, FCDAnimation* _parent)
:	FCDEntity(document, "Animation")
,	parent(_parent)
{
}

FCDAnimation::~FCDAnimation()
{
	childNodes.clear();
	parent = NULL;
}

FCDEntity* FCDAnimation::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDAnimation* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDAnimation(const_cast<FCDocument*>(GetDocument()), NULL);
	else if (_clone->HasType(FCDAnimation::GetClassType())) clone = (FCDAnimation*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		// Clone the channels
		for (FCDAnimationChannelContainer::const_iterator it = channels.begin(); it != channels.end(); ++it)
		{
			FCDAnimationChannel* clonedChannel = clone->AddChannel();
			(*it)->Clone(clonedChannel);
		}

		if (cloneChildren)
		{
			// Clone the animation tree children
			for (FCDAnimationContainer::const_iterator it = children.begin(); it != children.end(); ++it)
			{
				FCDAnimation* clonedChild = clone->AddChild();
				(*it)->Clone(clonedChild, cloneChildren);
			}
		}
	}

	return _clone;
}

// Creates a new animation entity sub-tree contained within this animation entity tree.
FCDAnimation* FCDAnimation::AddChild()
{
	FCDAnimation* animation = children.Add(GetDocument(), this);
	SetDirtyFlag();
	return animation;
}

// Adds a new animation channel to this animation entity.
FCDAnimationChannel* FCDAnimation::AddChannel()
{
	FCDAnimationChannel* channel = channels.Add(GetDocument(), this);
	SetDirtyFlag();
	return channel;
}

// Optimization: Look for the XML child node with the given id
xmlNode* FCDAnimation::FindChildById(const fm::string& _id)
{
	FUCrc32::crc32 id = FUCrc32::CRC32(_id.c_str() + ((_id[0] == '#') ? 1 : 0));
	for (FUXmlNodeIdPairList::iterator it = childNodes.begin(); it != childNodes.end(); ++it)
	{
		if ((*it).id == id) return (*it).node;
	}

	return (parent != NULL) ? parent->FindChildById(_id) : NULL;
}

// Look for an animation children with the given COLLADA Id.
const FCDEntity* FCDAnimation::FindDaeId(const fm::string& daeId) const
{
	if (GetDaeId() == daeId) return this;
	
	for (FCDAnimationContainer::const_iterator it = children.begin(); it != children.end(); ++it)
	{
		const FCDEntity* found = (*it)->FindDaeId(daeId);
		if (found != NULL) return found;
	}
	return NULL;
}

void FCDAnimation::GetHierarchicalAssets(FCDAssetConstList& assets) const
{
	for (const FCDAnimation* animation = this; animation != NULL; animation = animation->GetParent())
	{
		// Retrieve the asset information structure for this node.
		const FCDAsset* asset = animation->GetAsset();
		if (asset != NULL) assets.push_back(asset);
	}
	assets.push_back(GetDocument()->GetAsset());
}

// Retrieve all the curves created under this animation element, in the animation tree
void FCDAnimation::GetCurves(FCDAnimationCurveList& curves)
{
	// Retrieve the curves for this animation tree element
	for (FCDAnimationChannelContainer::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		const FCDAnimationCurveContainer& channelCurves = (*it)->GetCurves();
		for (FCDAnimationCurveContainer::const_iterator itC = channelCurves.begin(); itC != channelCurves.end(); ++itC)
		{
			curves.push_back(const_cast<FCDAnimationCurve*>(*itC));
		}
	}

	// Retrieve the curves for the animation nodes under this one in the animation tree
	for (FCDAnimationContainer::iterator it = children.begin(); it != children.end(); ++it)
	{
		(*it)->GetCurves(curves);
	}
}

bool FCDAnimation::Link()
{
	bool status = true;
	// Link the child nodes and check the curves for their drivers
	for (FCDAnimationChannelContainer::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		status &= (*it)->CheckDriver();
	}
	for (FCDAnimationContainer::iterator it = children.begin(); it != children.end(); ++it)
	{
		status &= (*it)->Link();
	}

	return status;
}

// Check for animation curves that need this animated as a driver
bool FCDAnimation::LinkDriver(FCDAnimated* animated)
{
	bool driver = false;

	// Link the child curves and child nodes
	for (FCDAnimationChannelContainer::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		driver |= (*it)->LinkDriver(animated);
	}
	for (FCDAnimationContainer::iterator it = children.begin(); it != children.end(); ++it)
	{
		driver |= (*it)->LinkDriver(animated);
	}

	return driver;
}

// Load a COLLADA animation node from the XML document
bool FCDAnimation::LoadFromXML(xmlNode* node)
{
	bool status = FCDEntity::LoadFromXML(node);
	if (!status) return status;
	if (!IsEquivalent(node->name, DAE_ANIMATION_ELEMENT))
	{
		return FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_ANIM_LIB, node->line);
	}

	// Optimization: Grab all the IDs of the child nodes, in CRC format.
	ReadChildrenIds(node, childNodes);

	// Parse all the inner <channel> elements
	xmlNodeList channelNodes;
	FindChildrenByType(node, DAE_CHANNEL_ELEMENT, channelNodes);
	channels.reserve(channelNodes.size());
	for (xmlNodeList::iterator itC = channelNodes.begin(); itC != channelNodes.end(); ++itC)
	{
		// Parse each <channel> element individually
		// They each handle reading the <sampler> and <source> elements
		FCDAnimationChannel* channel = AddChannel();
		status &= (channel->LoadFromXML(*itC));
		if (!status) SAFE_RELEASE(channel);
	}

	// Parse all the hierarchical <animation> elements
	xmlNodeList animationNodes;
	FindChildrenByType(node, DAE_ANIMATION_ELEMENT, animationNodes);
	for (xmlNodeList::iterator itA = animationNodes.begin(); itA != animationNodes.end(); ++itA)
	{
		FCDAnimation* animation = AddChild();
		animation->LoadFromXML(*itA);
	}
	return status;
}

// Search for an animation channel for the given XML pointer in this animation node
void FCDAnimation::FindAnimationChannels(const fm::string& pointer, FCDAnimationChannelList& targetChannels)
{
	// Look for channels locally
	for (FCDAnimationChannelContainer::iterator itChannel = channels.begin(); itChannel != channels.end(); ++itChannel)
	{
		if ((*itChannel)->GetTargetPointer() == pointer)
		{
			targetChannels.push_back(*itChannel);
		}
	}

	// Look for channel(s) within the child animations
	for (FCDAnimationContainer::iterator it = children.begin(); it != children.end(); ++it)
	{
		(*it)->FindAnimationChannels(pointer, targetChannels);
	}
}

// Write out the COLLADA animations to the document
xmlNode* FCDAnimation::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* animationNode = WriteToEntityXML(parentNode, DAE_ANIMATION_ELEMENT);

	// Write out the local channels
	for (FCDAnimationChannelContainer::const_iterator itChannel = channels.begin(); itChannel != channels.end(); ++itChannel)
	{
		(*itChannel)->LetWriteToXML(animationNode);
	}

	// Write out the child animations
	for (FCDAnimationContainer::const_iterator it = children.begin(); it != children.end(); ++it)
	{
		(*it)->LetWriteToXML(animationNode);
	}

	FCDEntity::WriteToExtraXML(animationNode);
	return animationNode;
}
