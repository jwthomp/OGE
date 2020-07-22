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

#include "FUtils/FUDaeParser.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationClip.h"
#include "FCDocument/FCDCamera.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDImage.h"
#include "FCDocument/FCDLight.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsScene.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDEmitter.h"

template <class T>
FCDLibrary<T>::FCDLibrary(FCDocument* document) : FCDObject(document)
{
}

template <class T>
FCDLibrary<T>::~FCDLibrary()
{
}

// Read in a list of entities for a library of a COLLADA document
template <class T>
bool FCDLibrary<T>::LoadFromXML(xmlNode* node)
{
	bool status = true;
	for (xmlNode* entityNode = node->children; entityNode != NULL; entityNode = entityNode->next)
	{
		if (entityNode->type == XML_ELEMENT_NODE)
		{
			T* entity = AddEntity();
			status &= (entity->LoadFromXML(entityNode));
		}
	}

	SetDirtyFlag();
	return status;
}

// Write out the library to the COLLADA XML document
template <class T>
xmlNode* FCDLibrary<T>::WriteToXML(xmlNode* node) const
{
	for (typename FCDEntityContainer::const_iterator itEntity = entities.begin(); itEntity != entities.end(); ++itEntity)
	{
		const T* entity = (const T*) (*itEntity);
		entity->LetWriteToXML(node);
	}
	return NULL;
}

// Search for the entity in this library with a given COLLADA id.
template <class T>
const T* FCDLibrary<T>::FindDaeId(const fm::string& _daeId) const
{
	const char* daeId = FUDaeParser::SkipPound(_daeId);
	for (typename FCDEntityContainer::const_iterator itEntity = entities.begin(); itEntity != entities.end(); ++itEntity)
	{
		const FCDEntity* found = (*itEntity)->FindDaeId(daeId);
		if (found != NULL && found->GetObjectType() == T::GetClassType())
		{
			return (T*) found;
		}
	}
	return NULL;
}

// Search for the entity in this library with a given COLLADA id.
template <class T>
T* FCDLibrary<T>::AddEntity()
{
	T* entity = entities.Add(GetDocument());
	SetDirtyFlag();
	return entity;
}


template <class T>
void FCDLibrary<T>::AddEntity(T* entity) 
{ 
	entities.push_back(entity); SetDirtyFlag(); 
}
