/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDExternalReference.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDPlaceHolder.h"

#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDEmitterInstance.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDPhysicsForceFieldInstance.h"

#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUFileManager.h"
#include "FUtils/FUUniqueStringMap.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDEntityInstance);

FCDEntityInstance::FCDEntityInstance(FCDocument* document, FCDSceneNode* _parent, FCDEntity::Type type)
:	FCDObject(document), parent(_parent)
,	entity(NULL), entityType(type), noUrl(false)
,	externalReference(NULL), useExternalReferenceID(false)
{
}

FCDEntityInstance::~FCDEntityInstance()
{
	if (externalReference != NULL)
	{
		UntrackObject(externalReference);
		SAFE_RELEASE(externalReference);
	}

	if (entity != NULL)
	{
		UntrackObject(entity);
		entity = NULL;
	}
}

void FCDEntityInstance::SetName(const fstring& _name) 
{
	name = CleanName(_name);
	SetDirtyFlag();
}

bool FCDEntityInstance::LoadFromXML(xmlNode* instanceNode)
{
	bool status = true;

	FUUri uri = ReadNodeUrl(instanceNode);
	if (!uri.prefix.empty())
	{
		FCDExternalReference* reference = GetExternalReference();
		reference->SetUri(uri);
	}
	else if (!uri.suffix.empty())
	{
		LoadExternalEntity(GetDocument(), uri.suffix);
		if (entity == NULL)
		{
			status = false;
			FUError::Error(FUError::WARNING, FUError::WARNING_INST_ENTITY_MISSING, instanceNode->line);
		}
	}

	wantedSubId = TO_STRING(ReadNodeSid(instanceNode));
	name = TO_FSTRING(ReadNodeName(instanceNode));

	// Read in the extra nodes
	xmlNodeList extraNodes;
	FindChildrenByType(instanceNode, DAE_EXTRA_ELEMENT, extraNodes);
	for (xmlNodeList::iterator it = extraNodes.begin(); it != extraNodes.end(); ++it)
	{
		xmlNode* extraNode = (*it);
		extra->LoadFromXML(extraNode);
	}

	SetDirtyFlag();
	return status;
}

void FCDEntityInstance::LoadExternalEntity(FCDocument* externalDocument, const fm::string& daeId)
{
	if (externalDocument == NULL || entity != NULL) return;

	FCDEntity* instancedEntity = NULL;
	switch (entityType)
	{
	case FCDEntity::ANIMATION: instancedEntity = (FCDEntity*) externalDocument->FindAnimation(daeId); break;
	case FCDEntity::CAMERA: instancedEntity = (FCDEntity*) externalDocument->FindCamera(daeId); break;
	case FCDEntity::EMITTER: instancedEntity = (FCDEntity*) externalDocument->FindEmitter(daeId); break;
	case FCDEntity::LIGHT: instancedEntity = (FCDEntity*) externalDocument->FindLight(daeId); break;
	case FCDEntity::GEOMETRY: instancedEntity = (FCDEntity*) externalDocument->FindGeometry(daeId); break;
	case FCDEntity::CONTROLLER: instancedEntity = (FCDEntity*) externalDocument->FindController(daeId); break;
	case FCDEntity::MATERIAL: instancedEntity = (FCDEntity*) externalDocument->FindMaterial(daeId); break;
	case FCDEntity::EFFECT: instancedEntity = (FCDEntity*) externalDocument->FindEffect(daeId); break;
	case FCDEntity::SCENE_NODE: instancedEntity = (FCDEntity*) externalDocument->FindSceneNode(daeId); break;
	case FCDEntity::FORCE_FIELD: instancedEntity = (FCDEntity*) externalDocument->FindForceField(daeId); break;
	case FCDEntity::PHYSICS_MATERIAL: instancedEntity = (FCDEntity*) externalDocument->FindPhysicsMaterial(daeId); break;
	case FCDEntity::PHYSICS_MODEL: instancedEntity = (FCDEntity*) externalDocument->FindPhysicsModel(daeId); break;
	default: break;
	}

	if (instancedEntity != NULL)
	{
		SetEntity(instancedEntity);
	}
}

// Write out the instantiation information to the XML node tree
xmlNode* FCDEntityInstance::WriteToXML(xmlNode* parentNode) const
{
	const char* instanceEntityName = GetInstanceClassType(entityType);
	xmlNode* instanceNode = AddChild(parentNode, instanceEntityName);

	if (!wantedSubId.empty())
	{
		AddAttribute(instanceNode, DAE_SID_ATTRIBUTE, wantedSubId);
	}
	if (!name.empty())
	{
		AddAttribute(instanceNode, DAE_NAME_ATTRIBUTE, name);
	}

	// Only write out our entity if we have one.
	if (!noUrl && (externalReference || entity))
	{
		fm::string urlString = "#";
		if (externalReference != NULL)
		{
			FUUri uri = externalReference->GetUri();
			urlString = TO_STRING(uri.prefix) + fm::string("#");
			if (entity == NULL || useExternalReferenceID) urlString += uri.suffix;
		}

		if (entity != NULL && !useExternalReferenceID)
		{
			urlString += entity->GetDaeId();
		}

		AddAttribute(instanceNode, DAE_URL_ATTRIBUTE, urlString);
	}
	return instanceNode;
}

void FCDEntityInstance::CleanSubId(FUSUniqueStringMap* parentStringMap)
{
	if (!wantedSubId.empty() && (parentStringMap != NULL))
	{
		parentStringMap->insert(wantedSubId);
	}
}

void FCDEntityInstance::WriteToExtraXML(xmlNode* instanceNode) const
{
	if (extra != NULL)
	{
		extra->LetWriteToXML(instanceNode);
	}
}

FCDExternalReference* FCDEntityInstance::GetExternalReference()
{
	if (externalReference == NULL)
	{
		externalReference = GetDocument()->GetExternalReferenceManager()->AddExternalReference(this);
		TrackObject(externalReference);
	}
	return externalReference;
}

// Retrieves the COLLADA name for the instantiation of a given entity type.
const char* FCDEntityInstance::GetInstanceClassType(FCDEntity::Type type) const
{
	const char* instanceEntityName;
	switch (type)
	{
	case FCDEntity::ANIMATION: instanceEntityName = DAE_INSTANCE_ANIMATION_ELEMENT; break;
	case FCDEntity::CAMERA: instanceEntityName = DAE_INSTANCE_CAMERA_ELEMENT; break;
	case FCDEntity::CONTROLLER: instanceEntityName = DAE_INSTANCE_CONTROLLER_ELEMENT; break;
	case FCDEntity::EMITTER: instanceEntityName = DAE_INSTANCE_EMITTER_ELEMENT; break;
	case FCDEntity::EFFECT: instanceEntityName = DAE_INSTANCE_EFFECT_ELEMENT; break;
	case FCDEntity::FORCE_FIELD: instanceEntityName = DAE_INSTANCE_FORCE_FIELD_ELEMENT; break;
	case FCDEntity::GEOMETRY: instanceEntityName = DAE_INSTANCE_GEOMETRY_ELEMENT; break;
	case FCDEntity::LIGHT: instanceEntityName = DAE_INSTANCE_LIGHT_ELEMENT; break;
	case FCDEntity::MATERIAL: instanceEntityName = DAE_INSTANCE_MATERIAL_ELEMENT; break;
	case FCDEntity::PHYSICS_MODEL: instanceEntityName = DAE_INSTANCE_PHYSICS_MODEL_ELEMENT; break;
	case FCDEntity::PHYSICS_RIGID_BODY: instanceEntityName = DAE_INSTANCE_RIGID_BODY_ELEMENT; break;
	case FCDEntity::PHYSICS_RIGID_CONSTRAINT: instanceEntityName = DAE_INSTANCE_RIGID_CONSTRAINT_ELEMENT; break;
	case FCDEntity::SCENE_NODE: instanceEntityName = DAE_INSTANCE_NODE_ELEMENT; break;

	case FCDEntity::ANIMATION_CLIP:
	case FCDEntity::ENTITY:
	case FCDEntity::IMAGE:
	default: FUFail(instanceEntityName = DAEERR_UNKNOWN_ELEMENT);
	}
	return instanceEntityName;
}

const FCDEntity* FCDEntityInstance::GetEntity() const
{
	if (entity != NULL) return entity;
	if (externalReference != NULL)
	{
		FCDPlaceHolder* placeHolder = externalReference->GetPlaceHolder();
		if (placeHolder != NULL)
		{
			const_cast<FCDEntityInstance*>(this)->LoadExternalEntity(placeHolder->GetTarget(FCollada::GetDereferenceFlag()), externalReference->GetEntityId());
		}
	}
	return entity;
}

void FCDEntityInstance::SetEntity(FCDEntity* _entity)
{
	// Stop tracking the old entity
	if (entity != NULL)
	{
		UntrackObject(entity);
		entity = NULL;
	}

	if (_entity != NULL)
	{
		// Todo: Some hierarchy-aware type checking of entity...

		if (_entity->GetDocument() == GetDocument() && externalReference != NULL)
		{
			UntrackObject(externalReference);
			SAFE_RELEASE(externalReference);
		}

		// Track the new entity
		entity = _entity;
		TrackObject(entity);

		// Update the external reference
		if (entity->GetDocument() != GetDocument())
		{
			if (externalReference == NULL)
			{
				externalReference = GetDocument()->GetExternalReferenceManager()->AddExternalReference(this);
				TrackObject(externalReference);
			}
			externalReference->SetEntityDocument(entity->GetDocument());
			externalReference->SetEntityId(entity->GetDaeId());
		}
	}
	SetDirtyFlag();
}

FCDEntityInstance* FCDEntityInstance::Clone(FCDEntityInstance* clone) const
{
	if (clone == NULL)
	{
		clone = new FCDEntityInstance(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(parent), entityType);
	}

	clone->SetEntity(entity);
	return clone;
}

// Callback when an object tracked by this tracker is being released.
void FCDEntityInstance::OnObjectReleased(FUObject* object)
{
	if (object == entity)
	{
		entity = NULL;
		if (externalReference == NULL || externalReference->GetPlaceHolder()->IsTargetLoaded())
		{
			Release();
		}
	}
	else if (object == externalReference)
	{
		externalReference = NULL;
		Release();
	}
}


/******************* FCDEntityInstanceFactory implementation ***********************/


FCDEntityInstance* FCDEntityInstanceFactory::CreateInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type type)
{
	switch (type)
	{
	case FCDEntity::CONTROLLER: return new FCDControllerInstance(document, parent, type); break;
	case FCDEntity::EMITTER: return new FCDEmitterInstance(document, parent, type); break;
	case FCDEntity::GEOMETRY: return new FCDGeometryInstance(document, parent, type); break;
	case FCDEntity::FORCE_FIELD: return new FCDPhysicsForceFieldInstance(document, parent, type); break;
	case FCDEntity::PHYSICS_MATERIAL:
	case FCDEntity::CAMERA:
	case FCDEntity::LIGHT:
	case FCDEntity::SCENE_NODE: return new FCDEntityInstance(document, parent, type); break;

	default: 
		FUFail(;);
		// Default to always return something.
		return new FCDEntityInstance(document, parent, type);
		break;
	}
}

FCDEntityInstance* FCDEntityInstanceFactory::CreateInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity* entity)
{
	FUAssert(entity != NULL, return NULL);

	FCDEntityInstance* instance = CreateInstance(document, parent, entity->GetType());
	instance->SetEntity(entity);
	return instance;
}

FCDEntityInstance* FCDEntityInstanceFactory::CreateInstance(FCDocument* document, FCDSceneNode* parent, xmlNode* node)
{
	bool status = false;
			// Look for instantiation elements
#define INSTANTIATE(instanceType, entityType) { \
		instanceType* instance = new instanceType(document, parent, entityType); \
		status |= (instance->LoadFromXML(node)); \
		if(status) return instance; \
		else { SAFE_DELETE(instance); return NULL; } }

	if		(IsEquivalent(node->name, DAE_INSTANCE_CAMERA_ELEMENT))		{ INSTANTIATE(FCDEntityInstance, FCDEntity::CAMERA); }
	else if (IsEquivalent(node->name, DAE_INSTANCE_CONTROLLER_ELEMENT)) { INSTANTIATE(FCDControllerInstance, FCDEntity::CONTROLLER); }
	else if (IsEquivalent(node->name, DAE_INSTANCE_EMITTER_ELEMENT))	{ INSTANTIATE(FCDEmitterInstance, FCDEntity::EMITTER); }
	else if (IsEquivalent(node->name, DAE_INSTANCE_FORCE_FIELD_ELEMENT)) { INSTANTIATE(FCDEntityInstance, FCDEntity::FORCE_FIELD); }
	else if (IsEquivalent(node->name, DAE_INSTANCE_GEOMETRY_ELEMENT))	{ INSTANTIATE(FCDGeometryInstance, FCDEntity::GEOMETRY); }
	else if (IsEquivalent(node->name, DAE_INSTANCE_LIGHT_ELEMENT))		{ INSTANTIATE(FCDEntityInstance, FCDEntity::LIGHT); }
	else if (IsEquivalent(node->name, DAE_INSTANCE_NODE_ELEMENT))		{ INSTANTIATE(FCDEntityInstance, FCDEntity::SCENE_NODE); }
#undef INSTANTIATE
	return NULL;
}

