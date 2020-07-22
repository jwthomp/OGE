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
	@file FCDEntityInstance.h
	This file contains the FCDEntityInstance class.
*/

#ifndef _FCD_ENTITY_INSTANCE_H_
#define _FCD_ENTITY_INSTANCE_H_

class FCDocument;
class FCDEntity;
class FCDExternalReference;
class FCDENode;
class FCDSceneNode;
class FCDEntityInstanceFactory;

template <class T> class FUUniqueStringMapT;
typedef FUUniqueStringMapT<char> FUSUniqueStringMap; /**< A set of unique strings. */

#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_

/**
	A COLLADA entity instance.
	COLLADA allows for quite a bit of per-instance settings
	for entities. This information is held by the up-classes of this class.
	This base class is simply meant to hold the entity that is instantiated.

	The entity instance tracks the entity, so that when an entity is released,
	all its instances are released.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDEntityInstance : public FCDObject, FUObjectTracker
{
public:
	/** The class type of the entity instance class.
		Used this information to up-cast an entity instance. */
	enum Type
	{
		SIMPLE, /**< A simple entity instance that has no per-instance information.
					This is used for lights, cameras, physics materials and force fields: there is no up-class. */
		GEOMETRY, /**< A geometry entity(FCDGeometryInstance). */
		CONTROLLER, /**< A controller entity(FCDControllerInstance). */
		MATERIAL, /**< A material entity(FCDMaterialInstance). */
		PHYSICS_MODEL, /**< A physics model(FCDPhysicsModelInstance). */
		PHYSICS_RIGID_BODY, /**< A physics rigid body(FCDPhysicsRigidBodyInstance). */
		PHYSICS_RIGID_CONSTRAINT, /**< A physics rigid constraint(FCDPhysicsRigidConstraintInstance). */
		PHYSICS_FORCE_FIELD, /**< A physics force field (FCDPhysicsForceFieldInstance). */
		NUM_TYPES
	};

private:
	DeclareObjectType(FCDObject);
	FCDSceneNode* parent;

	FCDEntity* entity;
	FCDEntity::Type entityType;

	FCDExternalReference* externalReference;
	bool useExternalReferenceID;

	// common attributes for instances
	fstring name;
	fm::string wantedSubId;
	
	// Extra information for the entity instance.
	FUObjectRef<FCDExtra> extra;

	friend class FCDEntityInstanceFactory;

protected:
	/** Bad code. This is used to avoid the export of the 'url' attribute for instance_rigid_constraint.
		Actually, instead of "bad code", I should say bad schema. */
	bool noUrl;

protected:
	/** Constructor: do not use directly.
		Instead, use the appropriate allocation function.
		For scene node instance: FCDSceneNode::AddInstance.
		@param document The COLLADA document that owns the entity instance.
		@param parent The visual scene node that contains the entity instance. This pointer will be NULL for
			instances that are not directly under a visual scene node.
		@param type The type of entity to instantiate. */
	FCDEntityInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type type);

public:
	/** Destructor. */
	virtual ~FCDEntityInstance();

	/** Retrieves the entity instance class type.
		This is used to determine the up-class for the entity instance object.
		@deprecated Instead use: FCDEntityInstance::HasType().
		@return The class type: SIMPLE for entity instances with no up-class. */
	virtual Type GetType() const { return SIMPLE; }

	/** Retrieves the instantiated entity type.
		The instantiated entity type will never change.
		@return The instantiated entity type. */
	FCDEntity::Type GetEntityType() const { return entityType; }

	/** Retrieves the instantiated entity.
		@return The instantiated entity. */
	inline FCDEntity* GetEntity() { return const_cast<FCDEntity*>(const_cast<const FCDEntityInstance*>(this)->GetEntity()); }
	const FCDEntity* GetEntity() const; /**< See above. */

	/** Sets the instantiated entity.
		The type of the entity will be verified.
		@param entity The instantiated entity. */
	void SetEntity(FCDEntity* entity);

	/** Retrieves the name of the entity instance. This value has no direct use
		in COLLADA but is useful to track the user-friendly name of an entity
		instance.
		@return The name. */
	const fstring& GetName() const { return name; }

	/** Sets the name of the entity instance. This value has no direct use in 
		COLLADA but is useful to track the user-friendly name of an entity
		instance.
		@param name The name. */
	void SetName(const fstring& name);

	/** Retrieves the optional sub id and is not garanteed to exist. 
		This id is the same as that given in SetSubId or from the COLLADA document using LoadFromXML unless it clashes with another id and 
		CleanSubId has been called.
		@return The set sub id of the node. */
	const fm::string& GetWantedSubId() const { return wantedSubId; }

	/** Sets the sub id for this object. 
		This id must be unique within the scope of the parent element. If it is not, it can be corrected by calling CleanSubId.
		@param _wantedSubId The new sub id of the object. */
	void SetWantedSubId(const fm::string& _wantedSubId) { wantedSubId = _wantedSubId; }

	/** Retrieves the extra information tree for this entity instance. The 
		prefered way to save extra information in FCollada is at the entity 
		level. Use this extra information tree to store any information you 
		want exported and imported back.
		@return The extra information tree. */
	FCDExtra* GetExtra() { return extra; }
	const FCDExtra* GetExtra() const { return extra; } /**< See above. */

	/** [INTERNAL] Loads the instantiated entity from an external document.
		This function is called by the document placeholder when the external
		document is loaded.
		@param externalDocument The externally referenced document.
		@param daeId The external referenced entity COLLADA id. */
	void LoadExternalEntity(FCDocument* externalDocument, const fm::string& daeId);

	/** Retrieves whether this entity instance points to an external entity.
		@return Whether this is an external entity instantiation. */
	bool IsExternalReference() const { return externalReference != NULL; }

	/** Retrieves the external reference for this entity instance.
		@return The external reference for this entity instance. This
			pointer will be NULL if the entity instance is local. */
	FCDExternalReference* GetExternalReference();
	const FCDExternalReference* GetExternalReference() const { return externalReference; } /**< See above. */

	/** Retrieves if we should use the external reference ID instead of the entity ID in
		XML output.
		@return The boolean value.*/
	bool IsUsingExternalReferenceID() const { return useExternalReferenceID; }

	/** Sets if we should use the external reference ID instead of the entity ID in
		XML output.
		@param u The boolean value.*/
	void UseExternalReferenceID(bool u) { useExternalReferenceID = u; }

	/** Retrieves the parent of the entity instance.
		@return the parent visual scene node. This pointer will be NULL
			when the instance is not created in the visual scene graph. */
	inline FCDSceneNode* GetParent() { return parent; }
	inline const FCDSceneNode* GetParent() const { return parent; } /**< See above. */

	/** [INTERNAL] Links controllers with their created nodes */
	virtual bool LinkImport(){ return true; };

	/** [INTERNAL] Links controllers with their created nodes */
	virtual bool LinkExport(){ return true; };

	/** [INTERNAL] Reads in the entity instance from a given COLLADA XML tree node.
		@param instanceNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the instance.*/
	virtual bool LoadFromXML(xmlNode* instanceNode);

	/** [INTERNAL] Writes out the entity instance to the given COLLADA XML tree node.
		This method should be overridden and called to ensure that the instance
		attributes are written. 
		@param parentNode The COLLADA XML parent node in which to insert the instance.
		@return The created XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;

	/** [INTERNAL] Writes out the extra information for the entity instance.
		This function should be used by all up-classes within the
		WriteToXML overwriting function, at the very end, to write
		the extra tree to the COLLADA document.
		@param instanceNode The created element XML tree node returned
			by the WriteToXML function. */
	virtual void WriteToExtraXML(xmlNode* instanceNode) const;

	/** [INTERNAL] Cleans up the sub identifiers.
		The sub identifiers must be unique with respect to its parent. This method corrects the sub ids if there are conflicts.
		@param parentStringMap The string map from the parent of this instance in which the sub ids must be unique. */
	virtual void CleanSubId(FUSUniqueStringMap* parentStringMap = NULL);

	/** Clones the entity instance.
		@param clone The entity instance to become the clone.
		@return The cloned entity instance. */
	virtual FCDEntityInstance* Clone(FCDEntityInstance* clone = NULL) const;

protected:
	/** [INTERNAL] Retrieves the COLLADA name for the instantiation of a given entity type.
		Children can override this method to easily add more class types.
		@param type The entity class type.
		@return The COLLADA name to instantiate an entity of the given class type. */
	virtual const char* GetInstanceClassType(FCDEntity::Type type) const;

	/** Callback when the instantiated entity is being released.
		@param object A tracked object. */
	virtual void OnObjectReleased(FUObject* object);
};


/**
	[INTERNAL] A factory for COLLADA Entity instances.
	Creates the correct instance object for a given entity type/XML tree node.
	To create new instances, use the FCDSceneNode::AddInstance function.
*/
class FCOLLADA_EXPORT FCDEntityInstanceFactory
{
private:
	FCDEntityInstanceFactory() {} // Static class: do not instantiate.

public:
	/** Creates a new COLLADA instance, given a entity type.
		@param document The COLLADA document that will own the new instance.
		@param parent The visual scene node that will contain the instance.
		@param type The type of instance object to create.
		@return The new COLLADA instance. This pointer will be NULL
			if the given type is invalid. */
	static FCDEntityInstance* CreateInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type type);

	/** Creates a new COLLADA instance of a given entity.
		@param document The COLLADA document that will own the new instance.
		@param parent The visual scene node that will contain the instance.
		@param entity The entity to create an instance of.
		@return The new COLLADA instance. This pointer will be NULL
			if the given type is invalid. */
	static FCDEntityInstance* CreateInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity *entity);

	/** [INTERNAL] Imports a COLLADA instance, given an XML tree node.
		@param document The COLLADA document that will own the new instance.
		@param parent The visual scene node that will contain the instance.
		@param node The XML tree node.
		@return The imported COLLADA instance. This pointer will be NULL
			if the XML tree node does not describe a COLLADA instance. */
	static FCDEntityInstance* CreateInstance(FCDocument* document, FCDSceneNode* parent, xmlNode* node);
};


#endif // _FCD_ENTITY_INSTANCE_H_
