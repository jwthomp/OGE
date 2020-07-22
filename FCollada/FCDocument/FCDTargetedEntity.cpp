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
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTargetedEntity.h"
#include "FUtils/FUDaeParser.h"

ImplementObjectType(FCDTargetedEntity);

FCDTargetedEntity::FCDTargetedEntity(FCDocument* document, const char* className)
:	FCDEntity(document, className)
{
}

FCDTargetedEntity::~FCDTargetedEntity()
{
}

// Sets a new target
void FCDTargetedEntity::SetTargetNode(FCDSceneNode* target)
{
	if (targetNode != NULL)
	{
		targetNode->DecrementTargetCount();
	}

	targetNode = target;

	if (targetNode != NULL)
	{
		targetNode->IncrementTargetCount();
	}

	SetDirtyFlag();
}

FCDEntity* FCDTargetedEntity::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	if (_clone == NULL)
	{
		_clone = new FCDTargetedEntity(const_cast<FCDocument*>(GetDocument()), "TargetedEntity");
	}

	// Clone the base class variables
	FCDEntity::Clone(_clone, cloneChildren);

	if (_clone->HasType(FCDTargetedEntity::GetClassType()))
	{
		FCDTargetedEntity* clone = (FCDTargetedEntity*) _clone;
		// Copy the target information over.
		clone->SetTargetNode(const_cast<FCDSceneNode*>((const FCDSceneNode*)targetNode));
	}

	return _clone;
}

// Link this camera with its target, if there is one
bool FCDTargetedEntity::LinkTarget()
{
	bool status = true;
	if (targetId.empty()) return status;

	// Skip externally-referenced targets, for now.
	FUUri targetUri(targetId);
	if (targetUri.prefix.empty() && !targetUri.suffix.empty())
	{
		FCDSceneNode* target = GetDocument()->FindSceneNode(targetUri.suffix);
		if (target == NULL)
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_TARGET_SCENE_NODE_MISSING);
		}
		SetTargetNode(target);
	}
	else
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNSUPPORTED_EXTERN_REF);
	}

	return status;
}

bool FCDTargetedEntity::LoadFromXML(xmlNode* entityNode)
{
	bool status = Parent::LoadFromXML(entityNode);

	// Look for and extract the target information from the extra tree nodes.
	// For backward-compatibility: we want to process the <technique> straight into the extra tree..
	FCDExtra* extra = GetExtra();
	extra->LoadFromXML(entityNode);

	// Extract out the target information. 
	FCDENode* targetNode = extra->GetDefaultType()->FindRootNode(DAEFC_TARGET_PARAMETER);
	if (targetNode != NULL)
	{
		targetId = TO_STRING(targetNode->GetContent());
		SAFE_RELEASE(targetNode);
	}

	return status;
}

void FCDTargetedEntity::WriteToExtraXML(xmlNode* entityNode) const
{
	FCDETechnique* technique = NULL;
	FCDENode* parameter = NULL;

	if (targetNode != NULL)
	{
		// Just for the export-time, add to the extra tree, the target information.
        FCDExtra* extra = const_cast<FCDExtra*>(GetExtra());
		technique = extra->GetDefaultType()->AddTechnique(DAE_FCOLLADA_PROFILE);
		parameter = technique->AddParameter(DAEFC_TARGET_PARAMETER, FS("#") + TO_FSTRING(GetTargetNode()->GetDaeId()));
	}

	// Export the extra tree to XML
	Parent::WriteToExtraXML(entityNode);

	if (targetNode != NULL)
	{
		// Delete the created extra tree nodes.
		SAFE_RELEASE(parameter);
		if (technique->GetChildNodeCount() == 0) SAFE_RELEASE(technique);
	}
}
