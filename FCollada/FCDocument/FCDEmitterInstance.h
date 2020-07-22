/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDEmitterInstance.h
	This file contains the FCDEmitterInstance class.

	@ingroup FCDEmitters
*/

#ifndef _FCD_EMITTER_INSTANCE_H_
#define _FCD_EMITTER_INSTANCE_H_

#ifndef _FCD_ENTITY_INSTANCE_H_
#include "FCDocument/FCDEntityInstance.h"
#endif // _FCD_ENTITY_INSTANCE_H_

class FCDGeometryPolygons;
class FCDMaterial;
class FCDMaterialInstance;
class FCDForceField;

typedef FUObjectContainer<FCDEntityInstance> FCDEntityInstanceContainer; /**< A dynamically-sized container of FCDEntityInstance objects. */
typedef FUObjectList<FCDEntityInstance> FCDEntityInstanceTrackList; /**< A dynamically-sized list of tracked FCDEntityInstance objects. */

/**
	A COLLADA emitter instance.

	The types of particles emitted are instance dependent.
	Instances also contain the FCDForceInstances pointers to forces applied to the particles.
*/
class FCOLLADA_EXPORT FCDEmitterInstance : public FCDEntityInstance
{
	friend class FCDEntityInstanceFactory;

private:
    DeclareObjectType(FCDEntityInstance);

	/** The forces that apply to our particles */
	FCDEntityInstanceTrackList forceInstances;

	/** The instances we will emit */
	FCDEntityInstanceContainer emittedInstances;

	/** [INTERNAL] A list of ids of forces that affect this emitter
		This list is valid during import -only- */
	StringList forceInstUris;

protected:
	/** Constructor: do not use directly.
		Instead, use the FCDSceneNode::AddInstance function.
		@param document The COLLADA document that owns the emitter instance.
		@param parent The parent visual scene node.
		@param entityType The type of the entity to instantiate. Unless this class
			is overwritten, FCDEntity::EMITTER should be given. */
	FCDEmitterInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type entityType = FCDEntity::EMITTER);

public:
	/** Destructor. */
	virtual ~FCDEmitterInstance();

	/** Clones the emitter instance.
		@param clone The emitter instance to become the clone.
			If this pointer is NULL, a new emitter instance will be created
			and you will need to release the returned pointer.
		@return The clone. */
	virtual FCDEntityInstance* Clone(FCDEntityInstance* clone = NULL) const;

	/** Add an COLLADA instance to be emitted from this system.
		@param type The type of FCDEntity to create an instance for.
		@return A new instance of the appropriate type */
	FCDEntityInstance* AddEmittedInstance(FCDEntity::Type type);

	/** Add an COLLADA instance to be emitted from this system.
		@param entity The FCDEntity to create an instance of.
		@return A new instance of the appropriate type */
	FCDEntityInstance* AddEmittedInstance(FCDEntity* entity);

	/** Get the number of instances we can emit.
		@return Our instance count */
	inline size_t GetEmittedInstanceCount() const							{ return emittedInstances.size(); }
	
	/** Get one of our emitted instances
		@param idx The index of the emitted instance
		@return The emitted instance.  This can be NULL if idx is out of bounds */
	inline FCDEntityInstance* GetEmittedInstance(size_t idx)				{ return emittedInstances.at(idx); }
	inline const FCDEntityInstance* GetEmittedInstance(size_t idx) const	{ return emittedInstances.at(idx); } /**< See above. */
	
	/** Retrieves the list of emitted instances.
		@return The list of emitted instances. */
	inline const FCDEntityInstanceContainer& GetEmittedInstanceContainer()	{ return emittedInstances; }

	/** Retrieves the number of force fields affecting this emitter instance.
		@return The number of force fields. */
	inline size_t GetForceFieldInstanceCount() const { return forceInstances.size(); }

	/** Retrieves a specific force field instance from the list for this emitter instance.
		@param index An index to a force field instance.
		@return The indexed force field instance. This pointer will be NULL
			if the given index is out-of-bounds. */
	inline FCDEntityInstance* GetForceFieldInstance(size_t index) { FUAssert(index < forceInstances.size(), return NULL); return forceInstances.at(index); }
	inline const FCDEntityInstance* GetForceFieldInstance(size_t index) const { FUAssert(index < forceInstances.size(), return NULL); return forceInstances.at(index); } /**< See above. */

	/** Set an instance of a force field to apply to this emitter.  This pointer must
		reference an existing and valid FCDEntityInstance in the scene node hierarchy that
		references an FCDForceField
		@param forceInstance the instance that will affect our particles
		@return false if for any reason the given instance is not valid */
	bool AddForceFieldInstance(FCDEntityInstance* forceInstance);

	/** [INTERNAL] Reads in the emitter instance properties from a XML tree node.
		@param instanceNode The XML tree node.
		@return The status of the import. If the status is 'false', it may be dangerous to
			use the emitter instance. */
	virtual bool LoadFromXML(xmlNode* instanceNode);

	/** [INTERNAL] Writes out the emitter instance to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the instance.
		@return The created XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;

	/** [INTERNAL] Link to our force instances on import.
		Will call LinkImport on all emitted instances also.
		@return TRUE if all the forces were successfully found. */
	virtual bool LinkImport();

	/** [INTERNAL] Not Used but
		Will call LinkImport on all emitted instances also.
		@return TRUE if all instances link correctly */
	virtual bool LinkExport();

private:
	/** [INTERNAL]
		Export the the list of emitted instances
		@param parent The xmlNode to export onto */
	void ExportEmittedInstanceList(xmlNode* parent) const;
	/** [INTERNAL]
		Import the the list of emitted instances
		@param node The xmlNode to containing our instances */
	bool ImportEmittedInstanceList(xmlNode* node);
};

#endif // _FCD_EMITTER_INSTANCE_H_
