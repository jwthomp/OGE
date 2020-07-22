/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDocument.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUDaeParser.h"

using namespace FUDaeWriter;

//
// FCDControllerInstance
//

ImplementObjectType(FCDControllerInstance);

FCDControllerInstance::FCDControllerInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type entityType)
:	FCDGeometryInstance(document, parent, entityType)
{
}

FCDControllerInstance::~FCDControllerInstance()
{
}

FCDEntityInstance* FCDControllerInstance::Clone(FCDEntityInstance* _clone) const
{
	FCDControllerInstance* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDControllerInstance(const_cast<FCDocument*>(GetDocument()), NULL, GetEntityType());
	else if (_clone->HasType(FCDControllerInstance::GetClassType())) clone = (FCDControllerInstance*) _clone;

	Parent::Clone(_clone);
	
	if (clone != NULL)
	{
		// Clone the URI list.
		clone->skeletonRoots = skeletonRoots;

		// Clone the joint list.
		clone->joints = joints;
	}
	return _clone;
}

bool FCDControllerInstance::LoadFromXML(xmlNode* instanceNode)
{
	bool status = FCDGeometryInstance::LoadFromXML(instanceNode);

	xmlNodeList skeletonList;
	FUDaeParser::FindChildrenByType(instanceNode, DAE_SKELETON_ELEMENT, skeletonList);
	size_t numRoots = skeletonList.size();
	skeletonRoots.resize(numRoots);

	for (size_t i = 0; i < numRoots; ++i)
	{
		skeletonRoots[i] = FUUri(FUDaeParser::ReadNodeContentDirect(skeletonList[i]));
	}

	return status;
}


// Retrieves a list of all the root joints for the controller.
void FCDControllerInstance::CalculateRootIds()
{
	skeletonRoots.clear();

	for (FCDSceneNodeTrackList::iterator itJ = joints.begin(); itJ != joints.end(); ++itJ)
	{
		const FCDSceneNode* joint = (*itJ);
		if (joint == NULL) continue;

		bool addToList = true;
		size_t parentCount = joint->GetParentCount();
		for (size_t p = 0; p < parentCount; ++p)
		{
			const FCDSceneNode* parentJoint = joint->GetParent(p);
			if (FindJoint(parentJoint))
			{
				addToList = false;
				break;
			}
		}

		if (addToList)
		{
			FUUri newRoot(fm::string("#") + joint->GetDaeId());
			skeletonRoots.push_back(newRoot);
		}
	}
}



// Link for export.  Ensure that our SubIds match the ones
// set on the SkinController we ref
bool FCDControllerInstance::LinkExport()
{
	// On export, ensure that our joints subIds are set on
	// our skin controller
	FCDSkinController* skin = FindSkin(GetEntity());
	if(skin == NULL) return true;

	size_t jointCount = skin->GetJointCount();
	size_t numBones = GetJointCount();

	FUAssert (numBones == jointCount, numBones = min(numBones, jointCount))
	
	skin->SetJointCount(numBones);
	FCDSkinControllerJoint* controllerJoints = skin->GetJoints();
	for (size_t i = 0; i < numBones; ++i)
	{
		// By default, use the daeId as subid
		FCDSceneNode* sceneNode = GetJoint(i);
		FUAssert(sceneNode != NULL, continue);
		
		FCDSkinControllerJoint& skinJoint = controllerJoints[i];
		fm::string newSubId;
		if (skinJoint.GetId().empty())
		{
			fm::string subId = sceneNode->GetSubId();
			if (subId.empty()) subId = sceneNode->GetDaeId();

			// Enforce uniqueness among sub-ids
			newSubId = subId;
			for (int counter = 1; counter < 200; ++counter)
			{
				bool isUnique = true;
				for (size_t j = 0; j < i; ++j)
				{
					if (IsEquivalent(controllerJoints[j].GetId(), newSubId))
					{
						isUnique = false;
						break;
					}
				}
				if (isUnique) break;
				newSubId = subId + counter++;
			}
			skinJoint.SetId(newSubId);
		}
		else newSubId = skinJoint.GetId();
		sceneNode->SetSubId(newSubId);
	}

	// Don't forget our own id list
	CalculateRootIds();

	return true;
}

// Link to whatever nodes we affect.
bool FCDControllerInstance::LinkImport()
{
	const FCDSkinController* skin = FindSkin(GetEntity());
	if(skin == NULL) return true;

	// Look for each joint, by COLLADA id, within the scene graph
	size_t jointCount = skin->GetJointCount();
	
	FCDSceneNodeList rootNodes = FindSkeletonNodes();
	size_t numRoots = rootNodes.size();
	
	joints.clear();
	for (size_t i = 0; i < jointCount; ++i)
	{
		const fm::string& jid = skin->GetJoint(i)->GetId();
		FCDSceneNode* boneNode = NULL;

		for (size_t i = 0; i < numRoots; i++)
		{
			// Find by subId
			boneNode = (FCDSceneNode*)rootNodes[i]->FindSubId(jid);
			if (boneNode != NULL) break;
		}
		if (boneNode == NULL)
		{
			for (size_t i = 0; i < numRoots; i++)
			{
				// Find by DaeId
				boneNode = (FCDSceneNode*)rootNodes[i]->FindDaeId(jid);
				if (boneNode != NULL) break;
			}
		}

		if (boneNode != NULL)
		{
			AddJoint(boneNode);
		}
		else
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_JOINT, 0);
		}
	}
	return true;
}

// Writes out the controller instance to the given COLLADA XML tree node.
xmlNode* FCDControllerInstance::WriteToXML(xmlNode* parentNode) const
{
	// Export the geometry instantiation information.
	xmlNode* instanceNode = FCDGeometryInstance::WriteToXML(parentNode);
	xmlNode* insertBeforeNode = (instanceNode != NULL) ? instanceNode->children : NULL;

	// Retrieve the parent joints and export the <skeleton> elements.
	for (FUUriList::const_iterator itS = skeletonRoots.begin(); itS != skeletonRoots.end(); ++itS)
	{
		// TODO: External references (again)
		globalSBuilder.set('#'); globalSBuilder.append((*itS).suffix);
		xmlNode* skeletonNode = InsertChild(instanceNode, insertBeforeNode, DAE_SKELETON_ELEMENT);
		AddContent(skeletonNode, globalSBuilder);
	}

	FCDGeometryInstance::WriteToExtraXML(instanceNode);
	return instanceNode;
}

bool FCDControllerInstance::AddJoint(FCDSceneNode* j)
{ 
	if (j != NULL) 
	{ 
		j->SetJointFlag(true);
		AppendJoint(j);
		return true;
	}
	return false;
}

// Look for the information on a given joint
bool FCDControllerInstance::FindJoint(FCDSceneNode* joint)
{
	return joints.contains(joint);
}


// Look for the information on a given joint
bool FCDControllerInstance::FindJoint(const FCDSceneNode* joint) const
{
	return joints.contains(joint);
}


int FCDControllerInstance::FindJointIndex(FCDSceneNode* joint)
{
	int i = 0;
	for(FCDSceneNodeTrackList::iterator itr = joints.begin();  itr != joints.end(); i++, itr++)
	{
		if (*itr == joint)
			return i;
	}
	return -1;
}

void FCDControllerInstance::AppendJoint(FCDSceneNode* j) 
{ 
	joints.push_back(j); 
}


const FCDSkinController* FCDControllerInstance::FindSkin(const FCDEntity* entity) const
{
	if(entity != NULL && entity->GetType() == FCDEntity::CONTROLLER)
	{
		const FCDController* controller = (const FCDController*) entity;
	
		if(controller->IsSkin()) 
		{
			return controller->GetSkinController();
		}
		else return FindSkin(controller->GetBaseTarget());
	}
	return NULL;
}

FCDSceneNodeList FCDControllerInstance::FindSkeletonNodes() 
{
	FCDocument* document = GetDocument();
	size_t numRoots = skeletonRoots.size();
	FCDSceneNodeList rootNodes;
	rootNodes.reserve(numRoots);
	for (size_t i = 0; i < numRoots; ++i)
	{
		FCDSceneNode* aRoot = document->FindSceneNode(skeletonRoots[i].suffix.c_str());
		if (aRoot == NULL)
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_JOINT, 0);
		}
		else rootNodes.push_back(aRoot);
	}
	// If we have no root, add the visual scene root.
	if(rootNodes.size() == 0) 
	{
		rootNodes.push_back((FCDSceneNode*)GetDocument()->GetVisualSceneRoot());
	}

	return rootNodes;
}
