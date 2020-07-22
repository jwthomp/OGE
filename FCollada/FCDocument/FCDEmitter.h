/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCD_EMITTER_H_
#define _FCD_EMITTER_H_

/**
	@file FCDEmitter.h
	This file defines the entity class for emitters in FCollada.
	
	@ingroup FCDEmitters
*/

class FCDocument;
class FCDExtra;
class FCDEmitterParticle;


#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_

/**
	A COLLADA generic emitter.

	This class does not belong to the COLLADA COMMON profile.
	It is used to define both particle and sound emitters.

	@ingroup FCDEmitters
*/

class FCOLLADA_EXPORT FCDEmitter : public FCDEntity
{
private:
	DeclareObjectType(FCDEntity);



public:

	/** Constructor.  Do not use directly, emitters should be created using
		the AddEntity function on the FCDEmitterLibrary
		@param document The COLLADA document that owns the emitter. */
	FCDEmitter(FCDocument* document);

	/** Destructor. */
	~FCDEmitter();

	/** Retrieves the entity class type.
		@return The entity class type: EMITTER */
	virtual Type GetType() const { return EMITTER; }

	/** Clones the emitter information.
		@param clone The cloned emitter.
			If this pointer is NULL, a new emitter is created and
			you will need to release it manually.
		@param cloneChildren Whether to recursively clone this entity's children.
		@return The clone. */
	virtual FCDEntity* Clone(FCDEntity* clone = NULL, bool cloneChildren = false) const;


	/** [INTERNAL] Link particle systems with any particle/emitter node instances once the document is loaded.
		@return currently always true. */
	bool Link();

	/** [INTERNAL] Reads in the emitter from a given COLLADA XML tree node.
		@param node The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the emitter.*/
	virtual bool LoadFromXML(xmlNode* node);

	/** [INTERNAL] Writes out the \<emitter\> element to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the geometry information.
		@return The created \<emitter\> element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};


#endif //_FCD_EMITTER_H
