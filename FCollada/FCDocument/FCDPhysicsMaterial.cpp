/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDPhysicsMaterial.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDPhysicsMaterial);

FCDPhysicsMaterial::FCDPhysicsMaterial(FCDocument* document) : FCDEntity(document, "PhysicsMaterial")
{
	staticFriction = 0.f;
	dynamicFriction = 0.f;
	restitution = 0.f;
}

FCDPhysicsMaterial::~FCDPhysicsMaterial()
{
}

// Cloning
FCDEntity* FCDPhysicsMaterial::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPhysicsMaterial* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsMaterial(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPhysicsMaterial::GetClassType())) clone = (FCDPhysicsMaterial*) _clone;
	
	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->SetStaticFriction(staticFriction);
		clone->SetDynamicFriction(dynamicFriction);
		clone->SetRestitution(restitution);
	}
	return _clone;
}

bool FCDPhysicsMaterial::LoadFromXML(xmlNode* physicsMaterialNode)
{
	bool status = FCDEntity::LoadFromXML(physicsMaterialNode);
	if (!status) return status;
	if (!IsEquivalent(physicsMaterialNode->name, DAE_PHYSICS_MATERIAL_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_PHYS_MAT_LIB_ELEMENT, physicsMaterialNode->line);
		return status;
	}

	//read in the <technique_common> element
	xmlNode* commonTechniqueNode = FindChildByType(physicsMaterialNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	if (commonTechniqueNode == NULL)
	{
		//return status.Fail(FS("Unable to find common technique for physics material: ") + TO_FSTRING(GetDaeId()), physicsMaterialNode->line);
		FUError::Error(FUError::ERROR, FUError::ERROR_COMMON_TECHNIQUE_MISSING, physicsMaterialNode->line);
	}

	xmlNode* paramNode = FindChildByType(commonTechniqueNode, DAE_PHYSICS_STATIC_FRICTION);
	if (paramNode != NULL) 
	{
		const char* content = ReadNodeContentDirect(paramNode);
		staticFriction = FUStringConversion::ToFloat(content);
	}

	paramNode = FindChildByType(commonTechniqueNode, DAE_PHYSICS_DYNAMIC_FRICTION);
	if (paramNode != NULL) 
	{
		const char* content = ReadNodeContentDirect(paramNode);
		dynamicFriction = FUStringConversion::ToFloat(content);
	}

	paramNode = FindChildByType(commonTechniqueNode, DAE_PHYSICS_RESTITUTION);
	if (paramNode != NULL)
	{
		const char* content = ReadNodeContentDirect(paramNode);
		restitution = FUStringConversion::ToFloat(content);
	}

	SetDirtyFlag(); 
	return status;
}

xmlNode* FCDPhysicsMaterial::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* physicsMaterialNode = WriteToEntityXML(parentNode, DAE_PHYSICS_MATERIAL_ELEMENT);
	xmlNode* commonTechniqueNode = AddChild(physicsMaterialNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	AddChild(commonTechniqueNode, DAE_PHYSICS_DYNAMIC_FRICTION, dynamicFriction);
	AddChild(commonTechniqueNode, DAE_PHYSICS_RESTITUTION, restitution);
	AddChild(commonTechniqueNode, DAE_PHYSICS_STATIC_FRICTION, staticFriction);

	FCDEntity::WriteToExtraXML(physicsMaterialNode);
	return physicsMaterialNode;
}
