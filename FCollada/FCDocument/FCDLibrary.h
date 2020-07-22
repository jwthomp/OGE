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

/**
	@file FCDLibrary.h
	This file contains the FCDLibrary template class.
	See the FCDLibrary.hpp file for the template implementation.
*/

#ifndef _FCD_LIBRARY_
#define _FCD_LIBRARY_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_

class FCDocument;
class FCDEntity;

/**
	A COLLADA library.

	A COLLADA library holds a list of entities. There are libraries for the following entities:
	animations (FCDAnimation), animation clips (FCDAnimationClip), meshes and splines (FCDGeometry),
	materials (FCDMaterial), effects (FCDEffect), images (FCDImage), skins and morphers (FCDController),
	cameras (FCDCamera), lights (FCDLight), physics models (FCDPhysicsModel), physics materials
	(FCDPhysicsMaterial), physics scenes (FCDPhysicsScene) and visual scenes (FCDSceneNode).

	The COLLADA libraries are contained within the FCDocument object.

	@ingroup FCDocument
*/	
template <class T>
class FCOLLADA_EXPORT FCDLibrary : public FCDObject
{
private:
	/** The containment dynamically-sized array for the entities. */
	typedef FUObjectContainer<T> FCDEntityContainer;

	/** Entities list. This list should contain all the root entities of the correct type.
		Note that the following entity types are tree-based, rather than list-based: FCDAnimation,
		FCDSceneNode and FCDPhysicsScene. */
	FCDEntityContainer entities;

public:
	/** Constructor: do not use directly.
		All the necessary libraries are created by the FCDocument object during its creation.
		@param document The parent document. */
	FCDLibrary(FCDocument* document);

	/** Destructor. */
	virtual ~FCDLibrary();

	/** Creates a new entity within this library.
		@return The newly created entity. */
	T* AddEntity();

	/** Inserts a new entity to this library.
		This function is useful if you are adding cloned entites
		back inside the library.
		@param entity The entity to insert in the library.
		@return The entity to insert. */
	void AddEntity(T* entity);

	/** Retrieves the library entity with the given COLLADA id.
		@param daeId The COLLADA id of the entity.
		@return The library entity which matches the COLLADA id.
			This pointer will be NULL if no matching entity was found. */
	T* FindDaeId(const fm::string& daeId) { return const_cast<T*>(const_cast<const FCDLibrary*>(this)->FindDaeId(daeId)); }
	const T* FindDaeId(const fm::string& daeId) const; /**< See above. */

	/** Returns whether the library contains no entities.
		@return Whether the library is empty. */
	inline bool IsEmpty() const { return entities.empty(); }

	/** Retrieves the number of entities within the library.
		@return the number of entities contained within the library. */
	inline size_t GetEntityCount() const { return entities.size(); }

	/** Retrieves an indexed entity from the library.
		@param index The index of the entity to retrieve.
			Should be within the range [0, GetEntityCount()[.
		@return The indexed entity. */
	inline T* GetEntity(size_t index) { FUAssert(index < GetEntityCount(), return NULL); return entities.at(index); }
	inline const T* GetEntity(size_t index) const { FUAssert(index < GetEntityCount(), return NULL); return entities.at(index); } /**< See above. */

	/** [INTERNAL] Reads in the contents of the library from the COLLADA XML document.
		@param node The COLLADA XML tree node to parse into entities.
		@return The status of the import. If the status is 'false', it may be dangerous to
			extract information from the library. */
	bool LoadFromXML(xmlNode* node);

	/** [INTERNAL] Writes out the library entities to the COLLADA XML document.
		@param node The COLLADA XML tree node in which to write the library entities. */
	xmlNode* WriteToXML(xmlNode* node) const;
};

#endif // _FCD_LIBRARY_
