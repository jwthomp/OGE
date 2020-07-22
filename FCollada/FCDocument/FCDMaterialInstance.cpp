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
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDEffectParameterList.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDExternalReference.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDMaterialInstance.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDMaterialInstance);

FCDMaterialInstance::FCDMaterialInstance(FCDEntityInstance* _parent)
:	FCDEntityInstance(_parent->GetDocument(), _parent->GetParent(), FCDEntity::MATERIAL), parent(_parent)
{
}

FCDMaterialInstance::~FCDMaterialInstance()
{
	bindings.clear();
	vertexBindings.clear();
	parent = NULL;
}

FCDObject* FCDMaterialInstance::GetGeometryTarget()
{
	if (parent != NULL && parent->GetEntity() != NULL)
	{
		FCDEntity* e = parent->GetEntity();
		if (e->HasType(FCDController::GetClassType()))
		{
			e = ((FCDController*) e)->GetBaseGeometry();
		}
		if (e->HasType(FCDGeometry::GetClassType()))
		{
			FCDGeometry* geometry = (FCDGeometry*) e;
			if (geometry->IsMesh())
			{
				FCDGeometryMesh* mesh = geometry->GetMesh();
				size_t polygonsCount = mesh->GetPolygonsCount();
				for (size_t i = 0; i < polygonsCount; ++i)
				{
					FCDGeometryPolygons* polygons = mesh->GetPolygons(i);
					if (IsEquivalent(polygons->GetMaterialSemantic(), semantic))
					{
						return polygons;
					}
				}
			}
		}
	}
	return NULL;
}


const FCDMaterialInstanceBind* FCDMaterialInstance::FindBinding(const char* semantic)
{
	for(FCDMaterialInstanceBindList::const_iterator it = bindings.begin(); it != bindings.end(); ++it)
	{
		if(IsEquivalent((*it).semantic, semantic))
			return &(*it);
	}
	return NULL;
}

FCDMaterialInstanceBindVertexInput* FCDMaterialInstance::AddVertexInputBinding()
{
	vertexBindings.push_back(FCDMaterialInstanceBindVertexInput());
	SetDirtyFlag();
	return &vertexBindings.back();
}

FCDMaterialInstanceBindVertexInput* FCDMaterialInstance::AddVertexInputBinding(const char* semantic, FUDaeGeometryInput::Semantic inputSemantic, int32 inputSet)
{
	FCDMaterialInstanceBindVertexInput* vbinding = AddVertexInputBinding();
	vbinding->semantic = semantic;
	vbinding->inputSemantic = inputSemantic;
	vbinding->inputSet = inputSet;
	return vbinding;
}

const FCDMaterialInstanceBindVertexInput* FCDMaterialInstance::FindVertexInputBinding(const char* semantic) const
{
	for (FCDMaterialInstanceBindVertexInputList::const_iterator it = vertexBindings.begin(); it != vertexBindings.end(); ++it)
	{
		if (IsEquivalent((*it).semantic, semantic)) return &(*it);
	}
	return NULL;
}

FCDMaterialInstanceBind* FCDMaterialInstance::AddBinding()
{
	bindings.push_back(FCDMaterialInstanceBind());
	SetDirtyFlag();
	return &bindings.back();
}

FCDMaterialInstanceBind* FCDMaterialInstance::AddBinding(const char* semantic, const char* target)
{
	FCDMaterialInstanceBind* binding = AddBinding();
	binding->semantic = semantic;
	binding->target = target;
	return binding;
}

void FCDMaterialInstance::RemoveBinding(size_t index)
{
	FUAssert(index < bindings.size(), return);
	bindings.erase(index);
}

// Create a flattened version of the instantiated material: this is the
// prefered way to generate DCC/viewer materials from a COLLADA document
FCDMaterial* FCDMaterialInstance::FlattenMaterial()
{
	// Retrieve the necessary geometry and material information
	FCDGeometry* geometry = NULL;
	if (parent->GetEntity() == NULL) return NULL;
	if (parent->GetEntity()->GetType() == FCDEntity::CONTROLLER)
	{
		FCDController* controller = (FCDController*) parent->GetEntity();
		geometry = controller->GetBaseGeometry();
	}
	else if (parent->GetEntity()->HasType(FCDGeometry::GetClassType()))
	{
		geometry = (FCDGeometry*) parent->GetEntity();
	}

	// Retrieve the original material and verify that we have all the information necessary.
	FCDMaterial* material = GetMaterial();
	if (material == NULL || geometry == NULL || (!geometry->IsMesh() && !geometry->IsPSurface()))
	{
		return NULL;
	}

	if (geometry->IsMesh())
	{
		// Retrieve the correct polygons for this material semantic
		FCDGeometryMesh* mesh = geometry->GetMesh();
		size_t polygonsCount = mesh->GetPolygonsCount();
		FCDGeometryPolygons* polygons = NULL;
		for (size_t i = 0; i < polygonsCount; ++i)
		{
			FCDGeometryPolygons* p = mesh->GetPolygons(i);
			if (semantic == p->GetMaterialSemantic()) { polygons = p; break; }
		}
		if (polygons == NULL)
			return NULL;

		FCDMaterial* clone = (FCDMaterial*) material->Clone();
		clone->Flatten();
		return clone;
	}

	return NULL;
}

FCDEntityInstance* FCDMaterialInstance::Clone(FCDEntityInstance* _clone) const
{
	FCDMaterialInstance* clone = NULL;
	if (_clone == NULL) clone = new FCDMaterialInstance(NULL);
	else if (!_clone->HasType(FCDMaterialInstance::GetClassType())) return Parent::Clone(_clone);
	else clone = (FCDMaterialInstance*) _clone;

	Parent::Clone(clone);

	// Clone the bindings and the semantic information.
	clone->semantic = semantic;
	clone->bindings = bindings; // since these two are not vectors of pointers, the copy constructors will work things out.
	clone->vertexBindings = vertexBindings;
	return clone;
}

// Read in the <instance_material> element from the COLLADA document
bool FCDMaterialInstance::LoadFromXML(xmlNode* instanceNode)
{
	// [glaforte] Do not execute the FCDEntityInstance::LoadFromXML parent function
	// as the <instance_material> element does not have the 'url' attribute.
	bool status = true;
	bindings.clear();

	SetWantedSubId(TO_STRING(ReadNodeSid(instanceNode)));
	semantic = TO_FSTRING(ReadNodeProperty(instanceNode, DAE_SYMBOL_ATTRIBUTE));
	fm::string materialId = ReadNodeProperty(instanceNode, DAE_TARGET_ATTRIBUTE);

	FUUri uri(materialId);
	if (!uri.prefix.empty())
	{
		// this is an XRef material
		SetMaterial(NULL);
		FCDExternalReference *xref = GetExternalReference();
		xref->SetUri(uri);
	}
	else
	{
		FCDMaterial* material = GetDocument()->FindMaterial(materialId);
		if (material == NULL)
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_MATERIAL_BINDING, instanceNode->line);
		}
		SetMaterial(material);
	}

	// Read in the ColladaFX bindings
	xmlNodeList bindNodes;
	FindChildrenByType(instanceNode, DAE_BIND_ELEMENT, bindNodes);
	for (xmlNodeList::iterator itB = bindNodes.begin(); itB != bindNodes.end(); ++itB)
	{
		fm::string semantic = ReadNodeSemantic(*itB);
		fm::string target = ReadNodeProperty(*itB, DAE_TARGET_ATTRIBUTE);
		AddBinding(semantic, target);
	}

	// Read in the ColladaFX vertex inputs
	xmlNodeList bindVertexNodes;
	FindChildrenByType(instanceNode, DAE_BIND_VERTEX_INPUT_ELEMENT, bindVertexNodes);
	for (xmlNodeList::iterator itB = bindVertexNodes.begin(); itB != bindVertexNodes.end(); ++itB)
	{
		fm::string inputSet = ReadNodeProperty(*itB, DAE_INPUT_SET_ATTRIBUTE);
		fm::string inputSemantic = ReadNodeProperty(*itB, DAE_INPUT_SEMANTIC_ATTRIBUTE);
		AddVertexInputBinding(ReadNodeSemantic(*itB).c_str(), FUDaeGeometryInput::FromString(inputSemantic.c_str()), FUStringConversion::ToInt32(inputSet));
	}


	SetDirtyFlag();
	return status;
}

// Write out the instantiation information to the XML node tree
xmlNode* FCDMaterialInstance::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* instanceNode = Parent::WriteToXML(parentNode);

	// no url in instance_material
	RemoveAttribute(instanceNode, DAE_URL_ATTRIBUTE);

	AddAttribute(instanceNode, DAE_SYMBOL_ATTRIBUTE, semantic);
	if (IsExternalReference())
	{
		FUUri xrefUri = GetExternalReference()->GetUri();
		AddAttribute(instanceNode, DAE_TARGET_ATTRIBUTE, TO_STRING(xrefUri.prefix) + "#" + xrefUri.suffix);
	}
	else
	{
		const FCDMaterial* material = GetMaterial();
		AddAttribute(instanceNode, DAE_TARGET_ATTRIBUTE, (material != NULL) ? fm::string("#") + material->GetDaeId() : emptyString);
	}

	// Write out the bindings.
	for (FCDMaterialInstanceBindList::const_iterator itB = bindings.begin(); itB != bindings.end(); ++itB)
	{
		const FCDMaterialInstanceBind& bind = *itB;
		xmlNode* bindNode = AddChild(instanceNode, DAE_BIND_ELEMENT);
		AddAttribute(bindNode, DAE_SEMANTIC_ATTRIBUTE, bind.semantic);
		AddAttribute(bindNode, DAE_TARGET_ATTRIBUTE, bind.target);
	}
	
	// Write out the vertex input bindings.
	for (FCDMaterialInstanceBindVertexInputList::const_iterator itV = vertexBindings.begin(); itV != vertexBindings.end(); ++itV)
	{
		const FCDMaterialInstanceBindVertexInput& bind = *itV;
		xmlNode* bindNode = AddChild(instanceNode, DAE_BIND_VERTEX_INPUT_ELEMENT);
		AddAttribute(bindNode, DAE_SEMANTIC_ATTRIBUTE, bind.semantic);
		AddAttribute(bindNode, DAE_INPUT_SEMANTIC_ATTRIBUTE, FUDaeGeometryInput::ToString(bind.inputSemantic));
		AddAttribute(bindNode, DAE_INPUT_SET_ATTRIBUTE, bind.inputSet);
	}


	Parent::WriteToExtraXML(instanceNode);
	return instanceNode;
}
