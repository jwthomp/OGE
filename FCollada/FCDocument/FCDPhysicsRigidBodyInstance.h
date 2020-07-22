/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDPhysicsRigidBodyInstance.h
	This file contains the FCDPhysicsRigidBodyInstance class.
*/

#ifndef _FCD_PHYSICS_RIGID_BODY_ENTITY_H_
#define _FCD_PHYSICS_RIGID_BODY_ENTITY_H_

#ifndef _FCD_ENTITY_INSTANCE_H_
#include "FCDocument/FCDEntityInstance.h"
#endif // _FCD_ENTITY_INSTANCE_H_

class FCDocument;
class FCDSceneNode;
class FCDPhysicsRigidBody;
class FCDPhysicsRigidBodyParameters;
class FCDPhysicsMaterial;
class FCDPhysicsModelInstance;
class FCDExternalReference;


/**
	An instance of a physics rigid body.
	Allows you to overwrite the material of a rigid body
	and attach the instance to a visual scene node.

	@ingroup FCDPhysics
*/
class FCOLLADA_EXPORT FCDPhysicsRigidBodyInstance : public FCDEntityInstance
{
private:
	DeclareObjectType(FCDEntityInstance);
	FCDPhysicsModelInstance* parent;

	FMVector3 angularVelocity;
	FMVector3 velocity;

	FCDPhysicsRigidBodyParameters* parameters;

	FUObjectPtr<FCDSceneNode> targetNode;

public:
	/** Constructor: do not use directly. Create new rigid bodies using the 
		FCDPhysicsModelInstance::AddRigidBodyInstance function.
		@param document The COLLADA document that contains this rigid body 
			instance. 
		@param _parent The physics model instance holding this rigid body 
			instance.
		@param body The rigid body to instance. Default values are taken from
			here. This should not be NULL unless it is expected to be filled in
			by LoadFromXML. */
	FCDPhysicsRigidBodyInstance(FCDocument* document, FCDPhysicsModelInstance* _parent, FCDPhysicsRigidBody* body);

	/** Destructor. */
	virtual ~FCDPhysicsRigidBodyInstance();

	/** Retrieves the entity type for this class. This function is part of the 
		FCDEntity interface.
		@return The entity type: PHYSICS_RIGID_BODY. */
	virtual Type GetType() const { return PHYSICS_RIGID_BODY; }


	/** Retrieves the parameters of tihs rigid body.
		@return The parameters. */
	FCDPhysicsRigidBodyParameters& GetParameters() { return *parameters; }
	const FCDPhysicsRigidBodyParameters& GetParameters() const { return *parameters; } /**< See above. */

	/** Retrieves the rigid body for this instance.
		@return The rigid body. */
	inline FCDPhysicsRigidBody* GetRigidBody() { return (FCDPhysicsRigidBody*) GetEntity(); }

	/** Retrieves the angular velocity of this rigid body instance.
		@return The angular velocity. */
	FMVector3& GetAngularVelocity() { return angularVelocity; }
	const FMVector3& GetAngularVelocity() const { return angularVelocity; } /**< See above. */

	/** Sets the angular velocity of this rigid body instance.
		@param _angularVelocity The angular velocity. */
	inline void SetAngularVelocity(const FMVector3& _angularVelocity) 
			{ angularVelocity = _angularVelocity; SetDirtyFlag(); }

	/** Retrives the linear velocity of this rigid body instance.
		@return The linear velocity. */
	FMVector3& GetVelocity() { return velocity; }
	const FMVector3& GetVelocity() const { return velocity; } /**< See above. */

	/** Sets the linear velocity of this rigid body instance.
		@param _velocity The linear velocity. */
	inline void SetVelocity(const FMVector3& _velocity) 
			{ velocity = _velocity; SetDirtyFlag(); }

	/** Retrieves the target node influenced by this rigid body instance.
		@return The target node. */
	FCDSceneNode* GetTargetNode() { return targetNode; }
	const FCDSceneNode* GetTargetNode() const { return targetNode; } /**< See above. */

	/** Sets the target node influenced by this rigid body instance.
		@param target The target node. */
	void SetTargetNode(FCDSceneNode* target) { targetNode = target;	SetDirtyFlag(); }

	/** Clones the rigid body instance.
		@param clone The rigid body instance to become the clone.
			If this pointer is NULL, a new rigid body instance will be created
			and you will need to release it.
		@return The clone. */
	virtual FCDEntityInstance* Clone(FCDEntityInstance* clone = NULL) const;

	/** [INTERNAL] Reads in the rigid body instance components from a XML tree 
		node.
		@param instanceNode The XML tree node.
		@return The status of the import. If the status is 'false', it may be 
			dangerous to use the rigid body instance. */
	virtual bool LoadFromXML(xmlNode* instanceNode);

	/** [INTERNAL] Writes out the \<instance_rigid_body\> element to the given 
		COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the 
			rigid body instance.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;

private:
	/** Sets the rigid body for this rigid body instance. Default values are
		taken from the rigid body.
		@param body The rigid body. */
	void SetRigidBody(FCDPhysicsRigidBody* body);
};

#endif // _FCD_PHYSICS_RIGID_BODY_ENTITY_H_
