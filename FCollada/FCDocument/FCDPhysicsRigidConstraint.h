/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDPhysicsRigidConstraint.h
	This file contains the FCDPhysicsRigidConstraint class.
*/

#ifndef _FCD_PHYSICS_RIGID_CONSTRAINT_H_
#define _FCD_PHYSICS_RIGID_CONSTRAINT_H_

#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_
#ifndef _FCD_TRANSFORM_H_
#include "FCDocument/FCDTransform.h" /** @todo Remove this include by moving the FCDTransform::Type enum to FUDaeEnum.h. */
#endif // _FCD_TRANSFORM_H_
#ifndef _FU_DAE_ENUM_H_
#include "FUtils/FUDaeEnum.h"
#endif // _FU_DAE_ENUM_H_

class FCDocument;
class FCDTransform;
class FCDPhysicsModel;
class FCDPhysicsRigidBody;
class FCDSceneNode;

typedef FUObjectContainer<FCDTransform> FCDTransformContainer; /**< A dynamically-sized containment array for transforms. */

/**
	A COLLADA rigid constraint.

	A rigid constraint attaches two rigid bodies together using springs, ball
	joints, or other limitations.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDPhysicsRigidConstraint : public FCDEntity
{
private:
	DeclareObjectType(FCDEntity);
	FCDPhysicsModel* parent;

	fm::string sid;

	bool enabled;
	bool interpenetrate;
	FUObjectPtr<FCDPhysicsRigidBody> referenceRigidBody;
	FUObjectPtr<FCDSceneNode> referenceNode;
	FUObjectPtr<FCDPhysicsRigidBody> targetRigidBody;
	FUObjectPtr<FCDSceneNode> targetNode;
	FMVector3 limitsLinearMin;
	FMVector3 limitsLinearMax;
	FMVector3 limitsSCTMin;
	FMVector3 limitsSCTMax;

	float springLinearStiffness;
	float springLinearDamping;
	float springLinearTargetValue;

	float springAngularStiffness;
	float springAngularDamping;
	float springAngularTargetValue;

	FCDTransformContainer transformsRef;
	FCDTransformContainer transformsTar;

public:
	/** Constructor: do not use directly. Create new rigid constraints using 
		the FCDPhysicsModel::AddRigidConstraint function.
		@param document The COLLADA document that contains this rigid 
			constraint. 
		@param _parent The physics model that contains this rigid constraint. 
	*/
	FCDPhysicsRigidConstraint(FCDocument* document, FCDPhysicsModel* _parent);

	/** Destructor. */
	virtual ~FCDPhysicsRigidConstraint();

	/** Retrieves the entity type for this class. This function is part of the 
		FCDEntity interface.
		@return The entity type: PHYSICS_RIGID_CONSTRAINT. */
	virtual Type GetType() const { return PHYSICS_RIGID_CONSTRAINT; }

	/** Retrieves the physics model that contraints this rigid constraint.
		@return The physics model. */
	FCDPhysicsModel* GetParent() { return parent; }
	const FCDPhysicsModel* GetParent() const { return parent; } /**< See above. */

	/** Retrieves the wanted sub-id for this transform.
		A wanted sub-id will always be exported, even if the transform is not animated.
		But the wanted sub-id may be modified if it isn't unique within the scope.
		@return The sub-id. */
	inline const fm::string& GetSubId() const { return sid; }

	/** Sets the wanted sub-id for this transform.
		A wanted sub-id will always be exported, even if the transform is not animated.
		But the wanted sub-id may be modified if it isn't unique within the scope.
		@param subId The wanted sub-id. */
	inline void SetSubId(const fm::string& subId) { sid = subId; SetDirtyFlag(); }
	inline void SetSubId(const char* subId) { sid = subId; SetDirtyFlag(); } /**< See above. */

	/** Retrieves the attached rigid body which is the reference.
		@return The reference rigid body. */
	FCDPhysicsRigidBody* GetReferenceRigidBody() { return referenceRigidBody; }
	const FCDPhysicsRigidBody* GetReferenceRigidBody() const { return referenceRigidBody; } /**< See above. */

	/** Sets the attached rigid body which is the reference.
		@param _referenceRigidBody The reference rigid body. */
	void SetReferenceRigidBody(FCDPhysicsRigidBody* _referenceRigidBody) { referenceRigidBody = _referenceRigidBody; referenceNode = NULL; SetDirtyFlag(); }

	/** Retrieves the attached rigid body which is not the reference.
		@return The non reference rigid body. */
	FCDPhysicsRigidBody* GetTargetRigidBody() { return targetRigidBody; }
	const FCDPhysicsRigidBody* GetTargetRigidBody() const { return targetRigidBody; } /**< See above. */

	/** Sets the attached rigid body which is not the reference.
		@param _targetRigidBody The non reference rigid body. */
	void SetTargetRigidBody(FCDPhysicsRigidBody* _targetRigidBody) { targetRigidBody = _targetRigidBody; targetNode = NULL; SetDirtyFlag(); }

	/** Retrieves the attached node which is the reference. This method should
		be avoided as the specification says the attachment should be to a 
		rigid body. This value is only used if GetReferenceRigidBody is NULL.
		@return The attached reference node. */
	FCDSceneNode* GetReferenceNode() { return referenceNode; }
	const FCDSceneNode* GetReferenceNode() const { return referenceNode; } /**< See above. */

	/** Sets the attached node which is the reference. This method should be 
		avoided as the specification says the attachment should be to a rigid 
		body. This value is only used if GetReferenceRigidBody is NULL.
		@param _referenceNode The attached reference node. */
	void SetReferenceNode(FCDSceneNode* _referenceNode) { referenceNode = _referenceNode; referenceRigidBody = NULL; SetDirtyFlag(); }

	/** Retrieves the attached node which is not the reference. This method 
		should be avoided as the specification says the attachment should be to
		a rigid body. This value is only used if GetTargetRigidBody is NULL.
		@return The attached non reference node. */
	FCDSceneNode* GetTargetNode() { return targetNode; }
	const FCDSceneNode* GetTargetNode() const { return targetNode; } /**< See above. */
	
	/** Sets the attached node which is not the reference. This method should 
		be avoided as the specification says the attachment should be to a 
		rigid body. This value is only used if GetTargetRigidBody is NULL.
		@param _targetNode The attached non reference node. */
	void SetTargetNode(FCDSceneNode* _targetNode) { targetNode = _targetNode; targetRigidBody = NULL; SetDirtyFlag(); }

	/** Retrieves the transforms for the attached rigid body which is the 
		reference. 
		@return The transforms. */
	FCDTransformContainer& GetTransformsRef() { return transformsRef; }
	const FCDTransformContainer& GetTransformsRef() const { return transformsRef; } /**< See above. */

	/** Retrieves the transforms for the attached rigid body which is not the
		reference.
		@return The transforms. */
	FCDTransformContainer& GetTransformsTar() { return transformsTar; }
	const FCDTransformContainer& GetTransformsTar() const { return transformsTar; } /**< See above. */

	/** Adds a transform for the attached rigid body which is the reference. 
		@param type The type of transform.
		@param index The position to add the transform to. */
	FCDTransform* AddTransformRef(FCDTransform::Type type, size_t index = (size_t)-1);

	/** Adds a transform for the attached rigid body which is not the 
		reference. 
		@param type The type of transform.
		@param index The position to add the transform to. */
	FCDTransform* AddTransformTar(FCDTransform::Type type, size_t index = (size_t)-1);

	/** Retrieves whether this rigid constraint is enabled.
		@return True if enabled. */
	const bool& GetEnabled() const { return enabled;}

	/** Sets whether this rigid constraint is enabled. 
		@param _enabled True of enabled. */
	void SetEnabled(bool _enabled) { enabled = _enabled; SetDirtyFlag(); }

	/** Retrieves whether the connected rigid bodies can penetrate eachother.
		@return True of they can penetrate. */
	const bool& GetInterpenetrate() const { return interpenetrate;}

	/** Sets whether the connected rigid bodies can penetrate eachother.
		@param _interpenetrate True of they can penetrate. */
	void SetInterpenetrate(bool _interpenetrate) { interpenetrate = _interpenetrate; SetDirtyFlag(); }

	/** Retrieves the linear min limit of the degrees of freedom.
		@return The linear min limit. */
	const FMVector3& GetLimitsLinearMin() const { return limitsLinearMin;}

	/** Sets the linear min limit of the degrees of freedom.
		@param _limitsLinearMin The linear min limit. */
	void SetLimitsLinearMin(const FMVector3& _limitsLinearMin) { limitsLinearMin = _limitsLinearMin; SetDirtyFlag(); }

	/** Retrieves the linear max limit of the degrees of freedom.
		@return The linear max limit. */
	const FMVector3& GetLimitsLinearMax() const { return limitsLinearMax;}

	/** Sets the linear max limit of the degrees of freedom.
		@param _limitsLinearMax The linear max limit. */
	void SetLimitsLinearMax(const FMVector3& _limitsLinearMax) { limitsLinearMax = _limitsLinearMax; SetDirtyFlag(); }

	/** Retrieves the swing cone and twist min limit of the degrees of freedom.
		@return The swing cone and twist min limit. */
	const FMVector3& GetLimitsSCTMin() const { return limitsSCTMin;}

	/** Sets the swing cone and twist min limit of the degrees of freedom.
		@param _limitsSCTMin The swing cone and twist min limit. */
	void SetLimitsSCTMin(const FMVector3& _limitsSCTMin) { limitsSCTMin = _limitsSCTMin; SetDirtyFlag(); }

	/** Retrieves the swing cone and twist max limit of the degrees of freedom.
		@return The swing cone and twist max limit. */
	const FMVector3& GetLimitsSCTMax() const { return limitsSCTMax;}

	/** Sets the swing cone and twist max limit of the degrees of freedom.
		@param _limitsSCTMax The swing cone and twist max limit. */
	void SetLimitsSCTMax(const FMVector3& _limitsSCTMax) { limitsSCTMax = _limitsSCTMax; SetDirtyFlag(); }

	/** Retrieves the spring linear stiffness of the spring rigid constraint.
		This is set to 1.0 if there is no spring.
		@return The spring linear stiffness. */
	float GetSpringLinearStiffness() const { return springLinearStiffness;}

	/** Sets the spring linear stiffness of the spring rigid constraint. This 
		is set to 1.0 if there is no spring.
		@param _springLinearStiffness The spring linear stiffness. */
	void SetSpringLinearStiffness(float _springLinearStiffness) { springLinearStiffness = _springLinearStiffness; SetDirtyFlag(); }

	/** Retrieves the spring linear damping of the spring rigid constraint.
		This is set to 0.0 if there is no spring.
		@return The spring linear damping. */
	float GetSpringLinearDamping() const { return springLinearDamping;}

	/** Sets the spring linear damping of the spring rigid constraint. This is 
		set to 0.0 if there is no spring.
		@param _springLinearDamping The spring linear damping. */
	void SetSpringLinearDamping(float _springLinearDamping) { springLinearDamping = _springLinearDamping; SetDirtyFlag(); }

	/** Retrieves the sping linear target value of the spring rigid constraint.
		This is set to 0.0 if there is no spring.
		@return The spring linear target value. */
	float GetSpringLinearTargetValue() const { return springLinearTargetValue;}

	/** Sets the sping linear target value of the spring rigid constraint. This
		is set to 0.0 if there is no spring.
		@param _springLinearTargetValue The spring linear target value. */
	void SetSpringLinearTargetValue(float _springLinearTargetValue) { springLinearTargetValue = _springLinearTargetValue; SetDirtyFlag(); }

	/** Retrieves the spring angular stiffness of the spring rigid constraint.
		This is set to 1.0 if there is no spring.
		@return The spring angular stiffness. */
	float GetSpringAngularStiffness() const { return springAngularStiffness;}

	/** Sets the spring angular stiffness of the spring rigid constraint. This 
		is set to 1.0 if there is no spring.
		@param _springAngularStiffness The spring angular stiffness. */
	void SetSpringAngularStiffness(float _springAngularStiffness) { springAngularStiffness = _springAngularStiffness; SetDirtyFlag(); }

	/** Retrieves the spring angular damping of the spring rigid constraint.
		This is set to 0.0 if there is no spring.
		@return The spring angular damping. */
	float GetSpringAngularDamping() const { return springAngularDamping;}

	/** Sets the spring angular damping of the spring rigid constraint. This is
		set to 0.0 if there is no spring.
		@param _springAngularDamping The spring angular damping. */
	void SetSpringAngularDamping(float _springAngularDamping) { springAngularDamping = _springAngularDamping; SetDirtyFlag(); }

	/** Retrieves the sping angular target value of the spring rigid 
		constraint. This is set to 0.0 if there is no spring.
		@return The spring angular target value. */
	float GetSpringAngularTargetValue() const { return springAngularTargetValue;}

	/** Sets the sping angular target value of the spring rigid constraint. 
		This is set to 0.0 if there is no spring.
		@param _springAngularTargetValue The spring angular target value. */
	void SetSpringAngularTargetValue(float _springAngularTargetValue) { springAngularTargetValue = _springAngularTargetValue; SetDirtyFlag(); }

	/** Retrieves the animated value for enabled.
		@returns The animated value, or NULL if enabled is not animated. */
	FCDAnimated* GetAnimatedEnabled();
	const FCDAnimated* GetAnimatedEnabled() const; /**< See above. */

	/** Retrieves the animated value for interpenetrate.
		@returns The animated value, or NULL if interpenetrate is not animated.
	*/
	FCDAnimated* GetAnimatedInterpenetrate();
	const FCDAnimated* GetAnimatedInterpenetrate() const; /**< See above. */

	/** Copies the rigid constraint into a clone.
		@param clone The empty clone. If this pointer is NULL, a new rigid
			constraint will be created and you will need to release the 
			returned pointer manually.
		@param cloneChildren Whether to recursively clone this entity's 
			children.
		@return The clone. */
	virtual FCDEntity* Clone(FCDEntity* clone = NULL, bool cloneChildren = false) const;

	/** [INTERNAL] Reads in the rigid constraint components from a XML tree 
		node.
		@param node The XML tree node.
		@return The status of the import. If the status is 'false', it may be 
			dangerous to use the rigid constraint. */
	virtual bool LoadFromXML(xmlNode* node);

	/** [INTERNAL] Writes out the \<rigid_constraint\> element to the given 
		COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the 
			rigid constraint.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_PHYSICS_RIGID_CONSTRAINT_H_
