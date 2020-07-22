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
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDPhysicsForceFieldInstance.h"
#include "FCDocument/FCDPhysicsScene.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUFileManager.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUUniqueStringMap.h"
#include "FCDocument/FCDExtra.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDPhysicsScene);

FCDPhysicsScene::FCDPhysicsScene(FCDocument* document)
:	FCDEntity(document, "PhysicsSceneNode")
,	gravity(0.0f, -9.8f, 0.0f), timestep(1.0f)
{
}

FCDPhysicsScene::~FCDPhysicsScene()
{
}

FCDEntity* FCDPhysicsScene::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPhysicsScene* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsScene(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPhysicsScene::GetClassType())) clone = (FCDPhysicsScene*) _clone;

	Parent::Clone(_clone, cloneChildren);
	
	if (clone == NULL)
	{
		// Clone the miscellaneous parameters
		clone->gravity = gravity;
		clone->timestep = timestep;

		// Clone the physics model instances
		for (FCDPhysicsModelInstanceContainer::const_iterator it = physicsModelInstances.begin(); it != physicsModelInstances.end(); ++it)
		{
			FCDPhysicsModelInstance* clonedInstance = clone->AddPhysicsModelInstance();
			(*it)->Clone(clonedInstance);
		}

		// Clone the force field instances
		for (FCDForceFieldInstanceContainer::const_iterator it = forceFieldInstances.begin(); it != forceFieldInstances.end(); ++it)
		{
			FCDPhysicsForceFieldInstance* clonedInstance = clone->AddForceFieldInstance();
			(*it)->Clone(clonedInstance);
		}
	}
	return _clone;
}

FCDPhysicsModelInstance* FCDPhysicsScene::AddPhysicsModelInstance(FCDPhysicsModel* model)
{
	FCDPhysicsModelInstance* instance = physicsModelInstances.Add(GetDocument());
	instance->SetEntity(model);
	SetDirtyFlag();
	return instance;
}

FCDPhysicsForceFieldInstance* FCDPhysicsScene::AddForceFieldInstance(FCDForceField* forceField)
{
	FCDPhysicsForceFieldInstance* instance = (FCDPhysicsForceFieldInstance*)
			FCDEntityInstanceFactory::CreateInstance(
					GetDocument(), (FCDSceneNode*) NULL, forceField);
	forceFieldInstances.push_back(instance);
	SetDirtyFlag();
	return instance;
}

bool FCDPhysicsScene::LoadFromXML(xmlNode* sceneNode)
{
	bool status = FCDEntity::LoadFromXML(sceneNode);
	if (!status) return status;

	if (IsEquivalent(sceneNode->name, DAE_PHYSICS_SCENE_ELEMENT))
	{
		for (xmlNode* child = sceneNode->children; child != NULL; child = child->next)
		{
			if (child->type != XML_ELEMENT_NODE) continue;

			// Look for instantiation elements
			if (IsEquivalent(child->name, DAE_INSTANCE_PHYSICS_MODEL_ELEMENT)) 
			{
				FCDPhysicsModelInstance* instance = AddPhysicsModelInstance(NULL);
				status &= (instance->LoadFromXML(child));
				continue; 
			}
			else if (IsEquivalent(child->name, DAE_TECHNIQUE_COMMON_ELEMENT))
			{
				xmlNode* gravityNode = FindChildByType(child, DAE_GRAVITY_ATTRIBUTE);
				if (gravityNode)
				{
					const char* gravityVal = ReadNodeContentDirect(gravityNode);
					gravity.x = FUStringConversion::ToFloat(&gravityVal);
					gravity.y = FUStringConversion::ToFloat(&gravityVal);
					gravity.z = FUStringConversion::ToFloat(&gravityVal);
				}
				xmlNode* timestepNode = FindChildByType(child, DAE_TIME_STEP_ATTRIBUTE);
				if (timestepNode)
				{
					timestep = FUStringConversion::ToFloat(ReadNodeContentDirect(timestepNode));
				}
			}
			else if (IsEquivalent(child->name, 
					DAE_INSTANCE_FORCE_FIELD_ELEMENT))
			{
				FCDPhysicsForceFieldInstance* instance = 
						AddForceFieldInstance(NULL);
				status &= (instance->LoadFromXML(child));
			}
			else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT))
			{
				// The extra information is loaded by the FCDEntity class.
			}
		}
	}

	SetDirtyFlag();
	return status;
}

xmlNode* FCDPhysicsScene::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* physicsSceneNode = WriteToEntityXML(parentNode, DAE_PHYSICS_SCENE_ELEMENT);
	if (physicsSceneNode == NULL) return physicsSceneNode;
	
	// Write out the instantiation: force fields, then physics models
	for (FCDForceFieldInstanceContainer::const_iterator itI = forceFieldInstances.begin(); itI != forceFieldInstances.end(); ++itI)
	{
		const FCDEntityInstance* instance = (*itI);
		instance->LetWriteToXML(physicsSceneNode);
	}

	for (FCDPhysicsModelInstanceContainer::const_iterator itI = physicsModelInstances.begin(); itI != physicsModelInstances.end(); ++itI)
	{
		const FCDEntityInstance* instance = (*itI);
		instance->LetWriteToXML(physicsSceneNode);
	}

	// Add COMMON technique.
	xmlNode* techniqueNode = AddChild(physicsSceneNode, 
			DAE_TECHNIQUE_COMMON_ELEMENT);
	AddChild(techniqueNode, DAE_GRAVITY_ATTRIBUTE, TO_STRING(gravity));
	AddChild(techniqueNode, DAE_TIME_STEP_ATTRIBUTE, timestep);

	// Write out the extra information
	FCDEntity::WriteToExtraXML(physicsSceneNode);

	return physicsSceneNode;
}

void FCDPhysicsScene::CleanSubId()
{
	FUSUniqueStringMap myStringMap;

	for (FCDForceFieldInstanceContainer::iterator itI = forceFieldInstances.begin(); itI != forceFieldInstances.end(); ++itI)
	{
		(*itI)->CleanSubId(&myStringMap);
	}

	for (FCDPhysicsModelInstanceContainer::iterator itI = physicsModelInstances.begin(); itI != physicsModelInstances.end(); ++itI)
	{
		(*itI)->CleanSubId(&myStringMap);
	}
}
