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
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDCamera.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDLight.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTransform.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUFileManager.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUUniqueStringMap.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDSceneNode);

FCDSceneNode::FCDSceneNode(FCDocument* document)
:	FCDEntity(document, "VisualSceneNode")
,	visibility(1.0f), targetCount(0), isJoint(false)
{
	SetTransformsDirtyFlag();
}

FCDSceneNode::~FCDSceneNode()
{
	parents.clear();

	// Delete the children, be watchful for the instantiated nodes
	while (!children.empty())
	{
		FCDSceneNode* child = children.front();
		child->parents.erase(this);
		
		if (child->parents.empty()) { SAFE_RELEASE(child); }
		else
		{
			// Check for external references in the parents
			bool hasLocalReferences = false;
			for (FCDSceneNodeTrackList::iterator itP = parents.begin(); itP != parents.end();)
			{
				if ((*itP) == this) children.erase(itP);
				else
				{
					hasLocalReferences |= (*itP)->GetDocument() == GetDocument();
					++itP;
				}
			}

			if (!hasLocalReferences) SAFE_RELEASE(child);
		}
	}
}

// Add this scene node to the list of children scene node
bool FCDSceneNode::AddChildNode(FCDSceneNode* sceneNode)
{
	if (this == sceneNode || sceneNode == NULL)
	{
		return false;
	}

	// Verify that we don't already contain this child node.
	if (children.contains(sceneNode)) return false;

	// Verify that this node is not one of the parents in the full hierarchically.
	FCDSceneNodeList queue;
	size_t parentCount = parents.size();
	for (size_t i = 0; i < parentCount; ++i) queue.push_back(parents.at(i));
	while (!queue.empty())
	{
		FCDSceneNode* parent = queue.back();
		queue.pop_back();
		if (parent == sceneNode) return false;
		queue.insert(queue.end(), parent->parents.begin(), parent->parents.end());
	}

	children.push_back(sceneNode);
	sceneNode->parents.push_back(this);
	SetDirtyFlag();
	return true;
}

FCDSceneNode* FCDSceneNode::AddChildNode()
{
	FCDSceneNode* node = new FCDSceneNode(GetDocument());
	AddChildNode(node);
	return node;
}

void FCDSceneNode::RemoveChildNode(FCDSceneNode* sceneNode)
{
	sceneNode->parents.erase(this);
	children.erase(sceneNode);
}

// Instantiates an entity
FCDEntityInstance* FCDSceneNode::AddInstance(FCDEntity* entity)
{
	if (entity == NULL) return NULL;
	FCDEntityInstance* instance =  AddInstance(entity->GetType());
	instance->SetEntity(entity);
	return instance;
}

FCDEntityInstance* FCDSceneNode::AddInstance(FCDEntity::Type type)
{
	FCDEntityInstance* instance = FCDEntityInstanceFactory::CreateInstance(GetDocument(), this, type);
	
	instances.push_back(instance);
	SetDirtyFlag();
	return instance;
}

// Adds a transform to the stack, at a given position.
FCDTransform* FCDSceneNode::AddTransform(FCDTransform::Type type, size_t index)
{
	FCDTransform* transform = FCDTFactory::CreateTransform(GetDocument(), this, type);
	if (transform != NULL)
	{
		if (index > transforms.size()) transforms.push_back(transform);
		else transforms.insert(transforms.begin() + index, transform);
	}
	SetDirtyFlag();
	SetTransformsDirtyFlag();
	return transform;
}

// Traverse the scene graph, searching for a node with the given COLLADA id
const FCDEntity* FCDSceneNode::FindDaeId(const fm::string& daeId) const
{
	if (GetDaeId() == daeId) return this;
	
	for (FCDSceneNodeTrackList::const_iterator it = children.begin(); it != children.end(); ++it)
	{
		const FCDEntity* found = (*it)->FindDaeId(daeId);
		if (found != NULL) return found;
	}
	return NULL;
}

void FCDSceneNode::SetSubId(const fm::string& subId)
{
	daeSubId = subId; 
}

// Traverse the scene graph, searching for a node with the given COLLADA sub id
FCDEntity* FCDSceneNode::FindSubId(const fm::string& subId)
{
	if (GetSubId() == subId) return this;
	
	for (FCDSceneNodeTrackList::iterator it = children.begin(); it != children.end(); ++it)
	{
		FCDEntity* found = (*it)->FindSubId(subId);
		if (found != NULL) return found;
	}
	return NULL;
}

// Retrieve the list of hierarchical asset information structures that affect this scene node.
void FCDSceneNode::GetHierarchicalAssets(FCDAssetConstList& assets) const
{
	for (const FCDSceneNode* node = this; node != NULL; node = node->GetParent(0))
	{
		// Retrieve the asset information structure for this node.
		const FCDAsset* asset = node->GetAsset();
		if (asset != NULL) assets.push_back(asset);
	}
	assets.push_back(GetDocument()->GetAsset());
}

// Calculate the transform matrix for a given scene node
FMMatrix44 FCDSceneNode::ToMatrix() const
{
	FMMatrix44 localTransform = FMMatrix44::Identity;
	for (FCDTransformContainer::const_iterator it = transforms.begin(); it != transforms.end(); ++it)
	{
		localTransform = localTransform * (*it)->ToMatrix();
	}
	return localTransform;
}

FMMatrix44 FCDSceneNode::CalculateWorldTransform() const
{
	const FCDSceneNode* parent = GetParent();
	if (parent != NULL)
	{
		//FMMatrix44 tm1 = parent->CalculateWorldTransform();
		//FMMatrix44 tm2 = CalculateLocalTransform();
		//return tm1 * tm2;
		return parent->CalculateWorldTransform() * CalculateLocalTransform();
	}
	else
	{
		return CalculateLocalTransform();
	}
}

// Parse a <scene> or a <node> node from a COLLADA document
bool FCDSceneNode::LoadFromXML(xmlNode* sceneNode)
{
	bool status = FCDEntity::LoadFromXML(sceneNode);
	if (!status) return status;
	if (!IsEquivalent(sceneNode->name, DAE_VSCENE_ELEMENT) && !IsEquivalent(sceneNode->name, DAE_NODE_ELEMENT))
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_UNKNOWN_ELEMENT, sceneNode->line);
	}

	// Read a subid if we gots one
	fm::string nodeSubId = ReadNodeProperty(sceneNode, DAE_SID_ATTRIBUTE);
	SetSubId(nodeSubId);

	// Read in the <node> element's type
	fm::string nodeType = ReadNodeProperty(sceneNode, DAE_TYPE_ATTRIBUTE);
	if (nodeType == DAE_JOINT_NODE_TYPE) SetJointFlag(true);
	else if (nodeType.length() == 0 || nodeType == DAE_NODE_NODE_TYPE) {} // No special consideration
	else
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOW_NODE_ELEMENT_TYPE, sceneNode->line);
	}

	// The scene node has ordered elements, so process them directly and in order.
	for (xmlNode* child = sceneNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_NODE_ELEMENT))
		{
			// Load the child scene node
			FCDSceneNode* node = AddChildNode();
			status = node->LoadFromXML(child);
			if (!status) break;
		}
		// Although this case can be handled by FCDEntityInstanceFactory,
		// we can do some special case handling here.
		else if (IsEquivalent(child->name, DAE_INSTANCE_NODE_ELEMENT))
		{
			FUUri url = ReadNodeUrl(child);
			if (url.prefix.empty())
			{
				// cannot find the node
				FCDSceneNode* node = GetDocument()->FindSceneNode(url.suffix);
				if (node != NULL)
				{
					
					if (!AddChildNode(node))
					{
						FUError::Error(FUError::WARNING, FUError::WARNING_CYCLE_DETECTED, child->line);
					}
				}
				else
				{
					FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_NODE_INST, child->line);
				}
			}
			else
			{
				FCDEntityInstance* reference = FCDEntityInstanceFactory::CreateInstance(GetDocument(), this, FCDEntity::SCENE_NODE);
				reference->LoadFromXML(child);
				instances.push_back(reference);
			}
		}
		else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT)) {} // Handled by FCDEntity.
		else if (IsEquivalent(child->name, DAE_ASSET_ELEMENT)) {} // Handled by FCDEntity.
		else
		{
			FCDTransform* transform = FCDTFactory::CreateTransform(GetDocument(), this, child);
			if (transform != NULL)
			{
				fm::string childSubId = ReadNodeProperty(child, DAE_SID_ATTRIBUTE);
				transform->SetSubId(childSubId);
				transforms.push_back(transform);
				status &= (transform->LoadFromXML(child));
			}
			else
			{
				FCDEntityInstance* instance = FCDEntityInstanceFactory::CreateInstance(GetDocument(), this, child);
				if(instance != NULL)
				{
					instances.push_back(instance);
				}
				else
				{
					FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_TRANSFORM, child->line);
				}
			}
		}
	}

	status &= (LoadFromExtra());
	SetTransformsDirtyFlag();
	SetDirtyFlag();
	return status;
}


bool FCDSceneNode::LinkImport()
{
	bool status;
	size_t i, size = instances.size();
	for (i = 0; i < size; i++)
	{
		status &= instances[i]->LinkImport();
	}

	size = GetChildrenCount();
	for (i = 0; i < size; i++)
	{
		status &= children[i]->LinkImport();
	}

	return status;
}

bool FCDSceneNode::LinkExport()
{
	bool status;
	size_t i, size = instances.size();
	for (i = 0; i < size; i++)
	{
		status &= instances[i]->LinkExport();
	}

	size = GetChildrenCount();
	for (i = 0; i < size; i++)
	{
		status &= children[i]->LinkExport();
	}

	return status;
}


bool FCDSceneNode::LoadFromExtra()
{
	bool status = true;

	FCDENodeList parameterNodes;
	StringList parameterNames;

	// Retrieve the extra information from the base entity class
	FCDExtra* extra = GetExtra();

	// List all the parameters
	size_t techniqueCount = extra->GetDefaultType()->GetTechniqueCount();
	for (size_t i = 0; i < techniqueCount; ++i)
	{
		FCDETechnique* technique = extra->GetDefaultType()->GetTechnique(i);
		technique->FindParameters(parameterNodes, parameterNames);
	}

	// Process the known parameters
	size_t parameterCount = parameterNodes.size();
	for (size_t i = 0; i < parameterCount; ++i)
	{
		FCDENode* parameterNode = parameterNodes[i];
		const fm::string& parameterName = parameterNames[i];
		FCDEAttribute* parameterType = parameterNode->FindAttribute(DAE_TYPE_ATTRIBUTE);
		if (parameterName == DAEMAYA_STARTTIME_PARAMETER)
		{
			GetDocument()->SetStartTime(FUStringConversion::ToFloat(parameterNode->GetContent()));
		}
		else if (parameterName == DAEMAYA_ENDTIME_PARAMETER)
		{
			GetDocument()->SetEndTime(FUStringConversion::ToFloat(parameterNode->GetContent()));
		}
		else if (parameterName == DAEFC_VISIBILITY_PARAMETER)
		{
			visibility = FUStringConversion::ToBoolean(parameterNode->GetContent()) ? 1.0f : 0.0f;
			FCDAnimatedFloat::Clone(GetDocument(), &parameterNode->GetAnimated()->GetDummy(), &visibility);
		}
		else if (parameterName == DAEMAYA_LAYER_PARAMETER || (parameterType != NULL && FUStringConversion::ToString(parameterType->value) == DAEMAYA_LAYER_PARAMETER))
		{
			FCDEAttribute* nameAttribute = parameterNode->FindAttribute(DAE_NAME_ATTRIBUTE);
			if (nameAttribute == NULL) continue;

			// Create a new layer object list
			FCDLayerList& layers = GetDocument()->GetLayers();
			FCDLayer* layer = new FCDLayer(); layers.push_back(layer);

			// Parse in the layer
			layer->name = FUStringConversion::ToString(nameAttribute->value);
			FUStringConversion::ToStringList(parameterNode->GetContent(), layer->objects);
		}
		else continue;

		SAFE_RELEASE(parameterNode);
	}

	// Read in the extra instances from the typed extra.
	FCDEType* instancesExtra = extra->FindType(DAEFC_INSTANCES_TYPE);
	if (instancesExtra != NULL)
	{
		FCDETechnique* fcolladaTechnique = instancesExtra->FindTechnique(DAE_FCOLLADA_PROFILE);
		if (fcolladaTechnique != NULL)
		{
			FCDENodeList nodesToRelease;
			FCDENodeContainer& childNodes = fcolladaTechnique->GetChildNodes();
			for (FCDENodeContainer::iterator it = childNodes.begin(); it != childNodes.end(); ++it)
			{
				xmlNode* baseNode = FUXmlWriter::CreateNode("_temp_");
				xmlNode* instanceNode = (*it)->LetWriteToXML(baseNode);

				FCDEntityInstance* instance = FCDEntityInstanceFactory::CreateInstance(GetDocument(), this, instanceNode);
				if (instance == NULL)
				{
					status = false;
					continue;
				}
				instances.push_back(instance);
				nodesToRelease.push_back(*it);

				xmlFreeNodeList(baseNode);
			}
			CLEAR_POINTER_VECTOR(nodesToRelease);
		}
	}
 
	SetDirtyFlag();
	return status;
}

// Write out a <visual_scene> element to a COLLADA XML document
xmlNode* FCDSceneNode::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* node = NULL;
	bool isVisualScene = false;

	FCDENodeList extraParameters;
	FCDETechnique* extraTechnique = NULL;

	if (GetParentCount() == 0)
	{
		node = WriteToEntityXML(parentNode, DAE_VSCENE_ELEMENT);
		isVisualScene = true;
	}
	else
	{
		node = WriteToEntityXML(parentNode, DAE_NODE_ELEMENT);
		if (GetSubId().length() > 0) AddAttribute(node, DAE_SID_ATTRIBUTE, GetSubId());

		if (isJoint)
		{
			AddAttribute(node, DAE_TYPE_ATTRIBUTE, DAE_JOINT_NODE_TYPE);
		}
		else
		{
			AddAttribute(node, DAE_TYPE_ATTRIBUTE, DAE_NODE_NODE_TYPE);
		}

		// Write out the visibility of this node, if it is not visible or if it is animated.
		if (GetDocument()->IsValueAnimated(&visibility) || !visibility)
		{
			extraTechnique = const_cast<FCDExtra*>(GetExtra())->GetDefaultType()->AddTechnique(DAE_FCOLLADA_PROFILE);
			FCDENode* visibilityNode = extraTechnique->AddParameter(DAEFC_VISIBILITY_PARAMETER, visibility >= 0.5f);
			visibilityNode->GetAnimated()->Copy(GetDocument()->FindAnimatedValue(&visibility));
			extraParameters.push_back(visibilityNode);
		}
	}

	// Write out the transforms
	for (FCDTransformContainer::const_iterator itT = transforms.begin(); itT != transforms.end(); ++itT)
	{
		const FCDTransform* transform = (*itT);
		transform->LetWriteToXML(node);
	}

	// Write out the instances
	// Some of the FCollada instance types are not a part of COLLADA, so buffer them to export in the <extra>.
	FCDENodeList extraInstanceNodes;
	FCDETechnique* extraInstanceTechnique = NULL;
	for (FCDEntityInstanceContainer::const_iterator itI = instances.begin(); itI != instances.end(); ++itI)
	{
		const FCDEntityInstance* instance = (*itI);
		if (instance->GetEntityType() == FCDEntity::FORCE_FIELD || instance->GetEntityType() == FCDEntity::EMITTER)
		{
			if (extraInstanceTechnique == NULL)
			{
				FCDExtra* extra = const_cast<FCDExtra*>(GetExtra());
				FCDEType* extraType = extra->AddType(DAEFC_INSTANCES_TYPE);
				extraInstanceTechnique = extraType->AddTechnique(DAE_FCOLLADA_PROFILE);
			}

			xmlNode* base = FUXmlWriter::CreateNode("_temp_");
			xmlNode* instanceNode = instance->LetWriteToXML(base);
			FCDENode* instanceAsExtra = extraInstanceTechnique->AddChildNode();
			
			bool loadSuccess = instanceAsExtra->LoadFromXML(instanceNode);
			xmlFreeNodeList(base);

			if(loadSuccess)
			{
				extraInstanceNodes.push_back(instanceAsExtra);
				continue;
			}
			// If we fail to create the extra tree, by default we fallback to writing to XML
		}
		instance->LetWriteToXML(node);
	}


	// First, write out the child nodes that we consider instances: there is more than one
	// parent node and we aren't the first one.
	for (FCDSceneNodeTrackList::const_iterator itC = children.begin(); itC != children.end(); ++itC)
	{
		const FCDSceneNode* child = (*itC);
		if (child->GetParent() != this)
		{
			bool alreadyInstantiated = false;
			for (FCDEntityInstanceContainer::const_iterator itI = instances.begin(); itI != instances.end(); ++itI)
			{
				if (!(*itI)->IsExternalReference())
				{
					alreadyInstantiated |= (*itI)->GetEntity() == child;
				}
			}
			if (!alreadyInstantiated)
			{
				FCDEntityInstance* instance = FCDEntityInstanceFactory::CreateInstance(const_cast<FCDocument*>(GetDocument()), NULL, FCDEntity::SCENE_NODE);
				instance->SetEntity(const_cast<FCDSceneNode*>(child));
				instance->LetWriteToXML(node);
			}
		}
	}

	// Then, hierarchically write out the child nodes.
	for (FCDSceneNodeTrackList::const_iterator itC = children.begin(); itC != children.end(); ++itC)
	{
		const FCDSceneNode* child = (*itC);
		if (child->GetParent() == this)
		{
			child->LetWriteToXML(node);
		}
	}

	// This only writes extra-related stuff, so execute last.
	if (isVisualScene) WriteVisualSceneToXML(node);

	// Write out the extra information and release the temporarily added extra parameters
	Parent::WriteToExtraXML(node);

	if (extraTechnique != NULL)
	{
		CLEAR_POINTER_VECTOR(extraParameters);
		if (extraTechnique->GetChildNodeCount() == 0) SAFE_RELEASE(extraTechnique);
	}
	CLEAR_POINTER_VECTOR(extraInstanceNodes);

	return parentNode;
}


void FCDSceneNode::WriteVisualSceneToXML(xmlNode* parentNode) const
{
	// Only one of the visual scenes should write this.
	if (GetDocument()->GetVisualSceneRoot() == this)
	{
		// For the main visual scene: export the layer information
		const FCDLayerList& layers = GetDocument()->GetLayers();
		if (!layers.empty())
		{
			xmlNode* techniqueNode = AddExtraTechniqueChild(parentNode, DAEMAYA_MAYA_PROFILE);
			for (FCDLayerList::const_iterator itL = layers.begin(); itL != layers.end(); ++itL)
			{
				xmlNode* layerNode = AddChild(techniqueNode, DAEMAYA_LAYER_PARAMETER);
				if (!(*itL)->name.empty()) AddAttribute(layerNode, DAE_NAME_ATTRIBUTE, (*itL)->name);
				FUSStringBuilder layerObjects;
				for (StringList::const_iterator itO = (*itL)->objects.begin(); itO != (*itL)->objects.end(); ++itO)
				{
					layerObjects.append(*itO);
					layerObjects.append(' ');
				}
				layerObjects.pop_back();
				AddContent(layerNode, layerObjects);
			}
		}

		// Export the start/end time.
		if (GetDocument()->HasStartTime() || GetDocument()->HasEndTime())
		{
			xmlNode* techniqueNode = AddExtraTechniqueChild(parentNode, DAE_FCOLLADA_PROFILE);
			if (GetDocument()->HasStartTime()) AddChild(techniqueNode, DAEMAYA_STARTTIME_PARAMETER, GetDocument()->GetStartTime());
			if (GetDocument()->HasEndTime()) AddChild(techniqueNode, DAEMAYA_ENDTIME_PARAMETER, GetDocument()->GetEndTime());
		}
	}
}

void FCDSceneNode::CleanSubId()
{
	FUSUniqueStringMap myStringMap;

	for (FCDEntityInstanceContainer::iterator itI = instances.begin(); itI != instances.end(); ++itI)
	{
		(*itI)->CleanSubId(&myStringMap);
	}

	for (FCDSceneNodeTrackList::iterator itC = children.begin(); itC != children.end(); ++itC)
	{
		(*itC)->CleanSubId();
	}
}

FCDEntity* FCDSceneNode::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDSceneNode* clone = NULL;
	if (_clone == NULL) clone = new FCDSceneNode(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDSceneNode::GetClassType())) clone = (FCDSceneNode*) _clone;
	
	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		// Copy over the simple information.
		clone->isJoint = isJoint;
		clone->visibility = visibility;

		// Don't copy the parents list but do clone all the children, transforms and instances
		for (FCDTransformContainer::const_iterator it = transforms.begin(); it != transforms.end(); ++it)
		{
			FCDTransform* transform = clone->AddTransform((*it)->GetType());
			(*it)->Clone(transform);
		}

		if (cloneChildren)
		{
			for (FCDSceneNodeTrackList::const_iterator it = children.begin(); it != children.end(); ++it)
			{
				FCDSceneNode* child = clone->AddChildNode();
				(*it)->Clone(child);
			}
		}

		for (FCDEntityInstanceContainer::const_iterator it = instances.begin(); it != instances.end(); ++it)
		{
			FCDEntityInstance* instance = clone->AddInstance((*it)->GetEntityType());
			(*it)->Clone(instance);
		}
	}

	return _clone;
}
