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
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDMaterialInstance.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUUniqueStringMap.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDGeometryInstance);

FCDGeometryInstance::FCDGeometryInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type entityType)
:	FCDEntityInstance(document, parent, entityType)
{
}

FCDGeometryInstance::~FCDGeometryInstance()
{
}

// Access Bound Materials
FCDMaterialInstance* FCDGeometryInstance::FindMaterialInstance(const fchar* semantic)
{ return const_cast<FCDMaterialInstance*>(const_cast<const FCDGeometryInstance*>(this)->FindMaterialInstance(semantic)); }
const FCDMaterialInstance* FCDGeometryInstance::FindMaterialInstance(const fchar* semantic) const
{
	for (FCDMaterialInstanceContainer::const_iterator itB = materials.begin(); itB != materials.end(); ++itB)
	{
		if ((*itB)->GetSemantic() == semantic) return (*itB);
	}
	return NULL;
}

FCDMaterialInstance* FCDGeometryInstance::AddMaterialInstance()
{
	FCDMaterialInstance* instance = materials.Add(this);
	SetDirtyFlag();
	return instance;
}

FCDMaterialInstance* FCDGeometryInstance::AddMaterialInstance(FCDMaterial* material, FCDGeometryPolygons* polygons)
{
	FCDMaterialInstance* instance = AddMaterialInstance();
	instance->SetMaterial(material);
	if (polygons != NULL)
	{
		const fstring& semantic = polygons->GetMaterialSemantic();
		if (!semantic.empty())
		{
			instance->SetSemantic(polygons->GetMaterialSemantic());
		}
		else
		{
			// Generate a semantic.
			fstring semantic = TO_FSTRING(material->GetDaeId()) + TO_FSTRING(polygons->GetFaceOffset());
			polygons->SetMaterialSemantic(semantic);
			instance->SetSemantic(semantic);
		}
	}
	return instance;
}


FCDMaterialInstance* FCDGeometryInstance::AddMaterialInstance(FCDMaterial* material, const fchar* semantic)
{
	FCDMaterialInstance* instance = AddMaterialInstance();
	instance->SetMaterial(material);
	instance->SetSemantic(semantic);
	return instance;
}

FCDEntityInstance* FCDGeometryInstance::Clone(FCDEntityInstance* _clone) const
{
	FCDGeometryInstance* clone = NULL;
	if (_clone == NULL) clone = new FCDGeometryInstance(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()), GetEntityType());
	else if (!_clone->HasType(FCDGeometryInstance::GetClassType())) return Parent::Clone(_clone);
	else clone = (FCDGeometryInstance*) _clone;

	Parent::Clone(clone);

	// Clone the material instances.
	for (FCDMaterialInstanceContainer::const_iterator it = materials.begin(); it != materials.end(); ++it)
	{
		FCDMaterialInstance* materialInstance = clone->AddMaterialInstance();
		(*it)->Clone(materialInstance);
	}

	return clone;
}

// Load the geometry instance from the COLLADA document
bool FCDGeometryInstance::LoadFromXML(xmlNode* instanceNode)
{
	bool status = Parent::LoadFromXML(instanceNode);
	if (!status) return status;

	// Look for the <bind_material> element. The others are discarded for now.
	xmlNode* bindMaterialNode = FindChildByType(instanceNode, DAE_BINDMATERIAL_ELEMENT);
	if (bindMaterialNode != NULL)
	{
		// Retrieve the list of the <technique_common><instance_material> elements.
		xmlNode* techniqueNode = FindChildByType(bindMaterialNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		xmlNodeList materialNodes;
		FindChildrenByType(techniqueNode, DAE_INSTANCE_MATERIAL_ELEMENT, materialNodes);
		for (xmlNodeList::iterator itM = materialNodes.begin(); itM != materialNodes.end(); ++itM)
		{
			FCDMaterialInstance* material = AddMaterialInstance();
			status &= (material->LoadFromXML(*itM));
		}
	}
	else
	{
		// Blinding attempt to use the material semantic from the polygons as a material id.
		FCDGeometry* geometry = (FCDGeometry*) GetEntity();
		if (geometry != NULL && geometry->HasType(FCDGeometry::GetClassType()) && geometry->IsMesh())
		{
			FCDGeometryMesh* mesh = geometry->GetMesh();
			size_t polyCount = mesh->GetPolygonsCount();
			for (size_t i = 0; i < polyCount; ++i)
			{
				FCDGeometryPolygons* polys = mesh->GetPolygons(i);
				const fstring& semantic = polys->GetMaterialSemantic();
				fm::string semanticUTF8 = TO_STRING(semantic);
				semanticUTF8 = FUDaeWriter::CleanId(semanticUTF8.c_str());
				FCDMaterial* material = GetDocument()->FindMaterial(semanticUTF8);
				if (material != NULL)
				{
					AddMaterialInstance(material, polys);
				}
			}
		}
	}

	SetDirtyFlag();
	return status;
}

// Write out the instantiation information to the XML node tree
xmlNode* FCDGeometryInstance::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* instanceNode = Parent::WriteToXML(parentNode);
	if (!materials.empty())
	{
		xmlNode* bindMaterialNode = AddChild(instanceNode, DAE_BINDMATERIAL_ELEMENT);
		xmlNode* techniqueCommonNode = AddChild(bindMaterialNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		for (FCDMaterialInstanceContainer::const_iterator itM = materials.begin(); itM != materials.end(); ++itM)
		{
			(*itM)->LetWriteToXML(techniqueCommonNode);
		}
	}
	Parent::WriteToExtraXML(instanceNode);
	return instanceNode;
}

void FCDGeometryInstance::CleanSubId(FUSUniqueStringMap* parentStringMap)
{
	Parent::CleanSubId(parentStringMap);
	FUSUniqueStringMap myStringMap;

	for (FCDMaterialInstanceContainer::iterator itM = materials.begin(); itM != materials.end(); ++itM)
	{
		(*itM)->CleanSubId(&myStringMap);
	}
}
