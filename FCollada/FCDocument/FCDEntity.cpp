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
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDSceneNode.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUUniqueStringMap.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDEntity);

FCDEntity::FCDEntity(FCDocument* document, const char* baseId) : FCDObjectWithId(document, baseId)
{
	extra = new FCDExtra(document, this);
}

FCDEntity::~FCDEntity()
{
}

// Structure cloning
FCDEntity* FCDEntity::Clone(FCDEntity* clone, bool UNUSED(cloneChildren)) const
{
	if (clone == NULL)
	{
		clone = new FCDEntity(const_cast<FCDocument*>(GetDocument()));
	}

	FCDObjectWithId::Clone(clone);
	clone->name = name;
	clone->note = note;
	if (extra != NULL)
	{
		extra->Clone(clone->extra);
	}
	return clone;
}

void FCDEntity::SetName(const fstring& _name) 
{
	name = CleanName(_name);
	SetDirtyFlag();
}

FCDAsset* FCDEntity::GetAsset()
{
	return (asset != NULL) ? asset : (asset = new FCDAsset(GetDocument()));
}

void FCDEntity::GetHierarchicalAssets(FCDAssetConstList& assets) const
{
	if (asset != NULL) assets.push_back(asset);
	else assets.push_back(GetDocument()->GetAsset());
}

// Parse this entity information from the COLLADA XML document
bool FCDEntity::LoadFromXML(xmlNode* entityNode)
{
	bool status = true;

	fm::string fileId = FUDaeParser::ReadNodeId(entityNode);
	if (!fileId.empty()) SetDaeId(fileId);
	else RemoveDaeId();

	name = TO_FSTRING(FUDaeParser::ReadNodeName(entityNode));
	if (name.empty()) name = TO_FSTRING(fileId);

	// Read in the asset information.
	xmlNode* assetNode = FindChildByType(entityNode, DAE_ASSET_ELEMENT);
	if (assetNode != NULL) GetAsset()->LoadFromXML(assetNode);

	// Read in the extra nodes
	xmlNodeList extraNodes;
	FindChildrenByType(entityNode, DAE_EXTRA_ELEMENT, extraNodes);
	for (xmlNodeList::iterator it = extraNodes.begin(); it != extraNodes.end(); ++it)
	{
		xmlNode* extraNode = (*it);
		extra->LoadFromXML(extraNode);

		// Look for an extra node at this level and a valid technique
		FCDETechnique* mayaTechnique = extra->GetDefaultType()->FindTechnique(DAEMAYA_MAYA_PROFILE);
		FCDETechnique* maxTechnique = extra->GetDefaultType()->FindTechnique(DAEMAX_MAX_PROFILE);
		FCDETechnique* fcTechnique = extra->GetDefaultType()->FindTechnique(DAE_FCOLLADA_PROFILE);

		// Read in all the extra parameters
		StringList parameterNames;
		FCDENodeList parameterNodes;
		if (mayaTechnique != NULL) mayaTechnique->FindParameters(parameterNodes, parameterNames);
		if (maxTechnique != NULL) maxTechnique->FindParameters(parameterNodes, parameterNames);
		if (fcTechnique != NULL) fcTechnique->FindParameters(parameterNodes, parameterNames);

		// Look for the note and user-properties, which is the only parameter currently supported at this level
		size_t parameterCount = parameterNodes.size();
		for (size_t i = 0; i < parameterCount; ++i)
		{
			FCDENode* parameterNode = parameterNodes[i];
			const fm::string& parameterName = parameterNames[i];

			if (parameterName == DAEMAX_USERPROPERTIES_NODE_PARAMETER || parameterName == DAEMAYA_NOTE_PARAMETER)
			{
				note = parameterNode->GetContent();
				SAFE_RELEASE(parameterNode);
			}
		}
	}

	SetDirtyFlag();
	return status;
}

// Look for a children with the given COLLADA Id.
const FCDEntity* FCDEntity::FindDaeId(const fm::string& _daeId) const
{
	if (GetDaeId() == _daeId) return this;
	return NULL;
}

xmlNode* FCDEntity::WriteToXML(xmlNode* parentNode) const
{
	return WriteToEntityXML(parentNode, DAEERR_UNKNOWN_ELEMENT);
}

xmlNode* FCDEntity::WriteToEntityXML(xmlNode* parentNode, const char* nodeName, bool writeId) const
{
	// Create the entity node and write out the id and name attributes
	xmlNode* entityNode = AddChild(parentNode, nodeName);
	if (writeId)
	{
		AddAttribute(entityNode, DAE_ID_ATTRIBUTE, GetDaeId());
	}
	if (!name.empty())
	{
		AddAttribute(entityNode, DAE_NAME_ATTRIBUTE, name);
	}

	// Write out the asset information.
	if (asset != NULL) asset->LetWriteToXML(entityNode);

	return entityNode;
}

void FCDEntity::WriteToExtraXML(xmlNode* entityNode) const
{
	if (extra != NULL)
	{
		FCDENodeList extraParameters;
		FCDETechnique* extraTechnique = NULL;

		// Add the note to the extra information
		if (HasNote())
		{
			extraTechnique = const_cast<FCDExtra&>(*extra).GetDefaultType()->AddTechnique(DAE_FCOLLADA_PROFILE);
			FCDENode* noteNode = extraTechnique->AddParameter(DAEMAX_USERPROPERTIES_NODE_PARAMETER, note);
			extraParameters.push_back(noteNode);
		}

		// Write out all the typed and untyped extra information and release the temporarily-added extra parameters.
		extra->LetWriteToXML(entityNode);
		if (extraTechnique != NULL)
		{
			CLEAR_POINTER_VECTOR(extraParameters);
			if (extraTechnique->GetChildNodeCount() == 0) SAFE_RELEASE(extraTechnique);
		}
	}
}
