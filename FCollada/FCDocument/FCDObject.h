/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDObject.h
	This file contains the FCDObject and the FCDObjectWithId classes.
*/

#ifndef __FCD_OBJECT_H_
#define __FCD_OBJECT_H_

#ifndef _FU_OBJECT_H_
#include "FUtils/FUObject.h"
#endif // _FU_OBJECT_H_

/**
	A basic COLLADA document object.
	
	All the objects owned by the COLLADA document derive from this class.
	The FCDocument object is accessible through this interface to all the object which it owns.

	Space for an handle which has no meaning to FCollada is available in this base class, for our users.
	You can therefore attach your own objects to most FCollada objects. If you assign memory buffers
	to the user-specified handle, be aware that FCollada will make no attempt to release it.

	A dirty flag is also available within this object. All FCollada objects should set
	the dirty flag when modifications are made to the objects, but FCollada will never reset it.
	This flag should be used by multi-tier applications. This flag defaults to 'true'.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDObject : public FUObject
{
private:
	DeclareObjectType(FUObject);

	// The COLLADA document that owns this object
	FCDocument* document;

	// An handle which has no meaning to FCollada but is available for users to
	// attach objects to most FCollada objects.
	void* userHandle;

protected:
	/** A generic flag container. Contains notifications of object state. */
	uint32 dirty;

	/** Use the following macros to declare flags. This will create functions
		to get, set and clear the flag, and a public static variable: 
		FLAG_(flag_name) to represent the flag mask.
		@param flag_name The unique name of the flag.
		@param local_position The local offset for this flag. */
#define DeclareFlag(flag_name, local_position) \
public: \
	static const uint32 FLAG_##flag_name = (1 << (Parent::nextAvailableBit + local_position)); \
	inline void Set##flag_name##Flag() { dirty |= FLAG_##flag_name; } \
	inline void Reset##flag_name##Flag() { dirty &= ~FLAG_##flag_name; } \
	inline bool Get##flag_name##Flag() const { return (dirty & FLAG_##flag_name) != 0; } 

	/** Use the following macro to set the number of
		local flags declared by this class.
		@param count The number of local flags declared by this class. */
#define DeclareFlagCount(count) \
protected: \
	static const uint32 nextAvailableBit = Parent::nextAvailableBit + count; \
public:

public:
	/** Declare the flags to set various states available on an FCDObject 
		Each Declare flag requires its local index.  It is required to 
		DeclareFlagCount, declaring the amount of flags specified locally */
	DeclareFlag(Dirty, 0);			/**< [EXPERIMENTAL] Generic Dirty Flag. */
	DeclareFlag(ValueChanged, 1);	/**< [EXPERIMENTAL] On Member Value Changed. */
	DeclareFlag(Transient, 2);		/**< [EXPERIMENTAL] Transient means don't save. */
	DeclareFlag(NewChild, 3);		/**< [EXPERIMENTAL] New Child has been assigned. */
	DeclareFlagCount(4);			/**< 4 flags are locally declared. */


	/** Constructor: sets the COLLADA document object.
		@param document The COLLADA document which owns this object. */
	FCDObject(FCDocument* document);

	/** Destructor. */
	virtual ~FCDObject() {}

	/** Retrieves the COLLADA document which owns this object.
		@return The COLLADA document. */
	inline FCDocument* GetDocument() { return document; }
	inline const FCDocument* GetDocument() const { return document; } /**< See above. */

	/** Retrieves whether a given object is a local reference from this object.
		@param object A data object.
		@return Whether a reference from this object to the given object is local. */
	inline bool IsLocal(const FCDObject* object) const { return document == object->document; }

	/** Retrieves whether a given object is an external reference from this object.
		@param object A data object.
		@return Whether a reference from this object to the given object is external. */
	inline bool IsExternal(const FCDObject* object) const { return document != object->document; }

	/** Retrieves the object's user-specified handle.
		This handle is available for users and has no
		meaning to FCollada.
		@return The object user-specified handle. */
	inline void* GetUserHandle() const { return userHandle; }
	
	/** Sets the object's user-specified handle.
		This handle is available for users and has no
		meaning to FCollada.
		@param handle The user-specified handle. */
	inline void SetUserHandle(void* handle) { userHandle = handle; SetDirtyFlag(); }

	/** These generic SetFlag/TestFlag routines can be used if the 
		user wants to test/set multiple flag types in one operation */
	// TODO: Once flag changes are complete and migrated, get rid of this lazy GetDirtyFlag
//	inline void SetDirtyFlag()		{ dirty = 0xFFFFFFFF; }
//	inline bool GetDirtyFlag()		{ return dirty != 0; }
//	inline void ResetDirtyFlag()	{ dirty = 0; }

	/** Sets the value of one or more flags.
		When a flag is set, its value is 1.
		@param f The mask of the flag(s) to set. */
	inline void SetFlag(uint32 f) { dirty |= f; }

	/** Retrieves the value of one or more flags.
		@param f The mask of the flag(s) to retrieve.
		@return The value of the requested flags. */
	inline uint32 TestFlag(uint32 f) const { return dirty & f; }
	
	/** Resets the value of one or more flags.
		When a flag is reset, its value is 0.
		@param f The mask of the flag(s) to reset. */
	inline void ClearFlag(uint32 f) { dirty &= ~f; }

	/** [INTERNAL] Entry function for writing to XML.  This function
		is here to automatically handle the case of transient objects,
		and can return NULL if this object doesn't want to be saved.
		@param entityNode The XML node of the parent. 
		@param unusedFlag Reserved.
		@return The XML node created for this object or NULL
			if this object is transient or the write failed. */
	inline xmlNode* LetWriteToXML(void* entityNode, bool unusedFlag=true) const { unusedFlag; if (!GetTransientFlag()) return WriteToXML((xmlNode*) entityNode); return NULL; }

	/** [INTERNAL] Reads in the entity from a given COLLADA XML tree node.
		This function should be overwritten by all up-classes.
		@param entityNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the object.*/
	virtual bool LoadFromXML(xmlNode* entityNode) { entityNode; return false; }

protected:
	/** [INTERNAL] Writes out the entity to the given COLLADA XML tree node.
		This function should be overwritten by all up-classes.
		@param parentNode The COLLADA XML parent node in which to insert the entity.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const { parentNode; return NULL; }

	// TODO: Track down each use of this flag and specialize it.
	/** Sets the dirty flag. */
//	inline void SetDirtyFlag() { dirty = 0x1; }

	/** Resets the dirty flag. */
//	inline void ResetDirtyFlag() { dirty &= ~0x1; }

	/** Retrieves the status of the dirty flag. */
//	inline bool GetDirtyFlag() const { return dirty & 0x1; }*/
};

/**
	A basic COLLADA object which has a unique COLLADA id.
	
	Many COLLADA structures such as entities and sources need a unique COLLADA id.
	The COLLADA document contains a map of all the COLLADA ids known in its scope.
	The interface of the FCDObjectWithId class allows for the retrieval and the modification
	of the unique COLLADA id attached to these objects.

	A unique COLLADA id is built, if none are provided, using the 'baseId' field of the constructor.
	A unique COLLADA id is generated only on demand.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDObjectWithId : public FCDObject
{
private:
	DeclareObjectType(FCDObject);

	fm::string daeId;
	bool hasUniqueId;

public:

	DeclareFlag(DaeIdChanged, 0);
	DeclareFlagCount(1);

	/** Constructor: sets the prefix COLLADA id to be used if no COLLADA id is provided.
		@param document The COLLADA document which owns this object.
		@param baseId The prefix COLLADA id to be used if no COLLADA id is provided. */
	FCDObjectWithId(FCDocument* document, const char* baseId = "ObjectWithID");

	/** Destructor. */
	virtual ~FCDObjectWithId();

	/** Retrieves the unique COLLADA id for this object.
		If no unique COLLADA id has been previously generated or provided, this function
		has the side-effect of generating a unique COLLADA id.
		@return The unique COLLADA id. */
	const fm::string& GetDaeId() const;

	/** Sets the COLLADA id for this object.
		There is no guarantee that the given COLLADA id will be used, as it may not be unique.
		You can call the GetDaeId function after this call to retrieve the final, unique COLLADA id.
		@param id The wanted COLLADA id for this object. This COLLADA id does not need to be unique.
			If the COLLADA id is not unique, a new unique COLLADA id will be generated. */
	void SetDaeId(const fm::string& id);

	/** Sets the COLLADA id for this object.
		There is no guarantee that the given COLLADA id will be used, as it may not be unique.
		@param id The wanted COLLADA id for this object. This COLLADA id does not need to be unique.
			If the COLLADA id is not unique, a new unique COLLADA id will be generated and
			this formal variable will be modified to contain the new COLLADA id. */
	void SetDaeId(fm::string& id);

	/** [INTERNAL] Release the unique COLLADA id of an object.
		Use this function wisely, as it leaves the object id-less and without a way to automatically
		generate a COLLADA id. */
	void RemoveDaeId();

	/** [INTERNAL] Clones the object. The unique COLLADA id will be copied over to the clone object.
		Use carefully: when a cloned object with an id is released, it
		does remove the unique COLLADA id from the unique name map.
		@param clone The object clone. */
	void Clone(FCDObjectWithId* clone) const;
};

#endif // __FCD_OBJECT_H_
