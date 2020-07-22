/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDPhysicsRigidBodyParameters.h
	This file contains the FCDPhysicsRigidBodyParameters class.
*/

#ifndef _FCD_PHYSICS_RIGID_BODY_PARAMETERS_H_
#define _FCD_PHYSICS_RIGID_BODY_PARAMETERS_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_

class FCDPhysicsMaterial;
class FCDPhysicsShape;
class FCDPhysicsRigidBody;
class FCDPhysicsRigidBodyInstance;
class FCDEntityInstance;

typedef FUObjectContainer<FCDPhysicsShape> FCDPhysicsShapeContainer; /**< A dynamically-sized containment array for physics shapes. */

/**
	A structure to hold the parameters for rigid body and rigid body instance.

	Because many of the parameters found in the rigid body can be overwritten
	by the rigid body instance, it is useful to keep it in one single place.
	This class is responsible for loading, storing, and writing these 
	parameters.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDPhysicsRigidBodyParameters
{
private:
	bool ownsPhysicsMaterial;
	FUObjectPtr<FCDPhysicsMaterial> physicsMaterial;
	FUObjectPtr<FCDEntityInstance> instanceMaterialRef;

	FCDPhysicsShapeContainer physicsShape;

	float dynamic; // animatable
	float mass;
	float density;
	FMVector3 inertia;
	FMVector3 massFrameTranslate;
	FMVector3 massFrameRotateAxis;
	float massFrameRotateAngle;

	FUObjectPtr<FCDObject> owner;
	FUObjectPtr<FCDPhysicsRigidBody> entityOwner;
	FUObjectPtr<FCDPhysicsRigidBodyInstance> instanceOwner;

	bool isDensityMoreAccurate;

public:
	/** Constructor. 
		@param owner The owner of this parameters holder. Its SetDirty will be
			called whenever this class is modified. It cannot be NULL. */
	FCDPhysicsRigidBodyParameters(FCDPhysicsRigidBody* owner);
	FCDPhysicsRigidBodyParameters(FCDPhysicsRigidBodyInstance* owner); /**< See above. */

	/** Destructor. */
	virtual ~FCDPhysicsRigidBodyParameters();

	/** Retrieves whether the owner is dynamic. If it is dynamic, forces like 
		gravity affect it.
		@return True if dynamic. */
	float& GetDynamic() { return dynamic;}
	const float& GetDynamic() const { return dynamic;} /**< See above. */
	bool IsDynamic() { return dynamic >= 0.5f; } /**< See above. */

	/** Sets whether the owner is dynamic. If it is dynamic, forces like 
		gravity affect it.
		@param _dynamic True if dynamic. */
	inline void SetDynamic(bool _dynamic) { dynamic = _dynamic; owner->SetDirtyFlag(); }

	/** Retrieves whether density is more accurate. Because we are using an approximating algorithm for volume, density will be more accurate when
		dealing with non-analytical shapes. Density is calculated as the average density of the shapes. Shapes defining a mass will have density
		of 0.0f. A rigid body containing both shapes with only density and shapes with only mass will have both GetMass and GetDensity 
		approximated.
		@return True if density is more accurate. */
	bool IsDensityMoreAccurate() { return isDensityMoreAccurate; }

	/** Retrieves the density of the owner. The client should call IsDensityMoreAccurate to make sure this is what we want instead of mass.
		@return The density. */
	float& GetDensity() { return density; }
	const float& GetDensity() const { return density; } /**< See above. */

	/** Retrieves the mass of the owner.
		@return The mass. */
	float& GetMass() { return mass;}
	const float& GetMass() const { return mass;} /**< See above. */

	/** Sets the mass of the owner. 
		@param _mass The mass. */
	inline void SetMass(float _mass) { mass = _mass; owner->SetDirtyFlag(); }

	/** Retrieves the inertia of the owner.
		@return The inertia. */
	FMVector3& GetInertia() { return inertia;}
	const FMVector3& GetInertia() const { return inertia;} /**< See above. */

	/** Sets the inertia of the owner.
		@param _inertia The inertia. */
	inline void SetInertia(const FMVector3& _inertia) 
			{ inertia = _inertia; owner->SetDirtyFlag(); }

	/** Retrieves the center of mass of the owner.
		@return The center of mass. */
	FMVector3& GetMassFrameTranslate() { return massFrameTranslate;}
	const FMVector3& GetMassFrameTranslate() const 
			{ return massFrameTranslate;} /**< See above. */

	/** Sets the center of mass of the owner.
		@param _massFrameTranslate The center of mass. */
	inline void SetMassFrameTranslate(const FMVector3& _massFrameTranslate) {
			massFrameTranslate = _massFrameTranslate; owner->SetDirtyFlag(); }
	
	/** Retrieves the axis of orientation of mass of the owner.
		@return The axis of orientation of mass. */
	FMVector3& GetMassFrameRotateAxis() { return massFrameRotateAxis;}
	const FMVector3& GetMassFrameRotateAxis() const 
			{ return massFrameRotateAxis;} /**< See above. */

	/** Sets the axis of orientation of mass of the owner.
		@param _massFrameRotateAxis The axis of orientation of mass. */
	inline void SetMassFrameRotateAxis(const FMVector3& _massFrameRotateAxis) {
			massFrameRotateAxis = _massFrameRotateAxis; owner->SetDirtyFlag();
	}

	/** Retrieves the angle of orientation of mass of the owner along the axis 
		retrieved from GetMassFrameRotateAxis.
		@return The angle of orientation of mass. */
	float GetMassFrameRotateAngle() { return massFrameRotateAngle;}
	const float GetMassFrameRotateAngle() const 
			{ return massFrameRotateAngle;} /**< See above. */

	/** Sets the angle of orientation of mass of the owner along the axis
		retrieved from GetMassFrameRotateAxis.
		@param _massFrameRotateAngle The angle of orientation of mass. */
	inline void SetMassFrameRotateAngle(float _massFrameRotateAngle) {
			massFrameRotateAngle = _massFrameRotateAngle; 
			owner->SetDirtyFlag(); }

	/** Retrives the physics material of the owner.
		@return The physics material. */
	FCDPhysicsMaterial* GetPhysicsMaterial() { return physicsMaterial; }
	const FCDPhysicsMaterial* GetPhysicsMaterial() const 
			{ return physicsMaterial; } /**< See above. */

	/** Sets the physics material of the owner.
		@param physicsMaterial The physics material. */
	void SetPhysicsMaterial(FCDPhysicsMaterial* physicsMaterial);

	/** Adds a physics material for the owner. This parameter structuer is 
		responsible for releasing the physics material. 
		@return The new physics material. */
	FCDPhysicsMaterial* AddOwnPhysicsMaterial();

	/** Retrieves the physics shapes of the owner.
		@return The physics shapes. */
	FCDPhysicsShapeContainer& GetPhysicsShapeList() { return physicsShape; }
	const FCDPhysicsShapeContainer& GetPhysicsShapeList() const { return physicsShape; } /**< See above. */

	/** Retrieves the number of physics shapes of the owner.
		@return The number of physics shapes. */
	size_t GetPhysicsShapeCount() const { return physicsShape.size(); }

	/** Retrieves a speficied physics shape of the owner by index.
		@param index The index of the physics shape.
		@return The physics shape. */
	FCDPhysicsShape* GetPhysicsShape(size_t index) { FUAssert(index < physicsShape.size(), return NULL) return physicsShape.at(index); }
	const FCDPhysicsShape* GetPhysicsShape(size_t index) const { FUAssert(index < physicsShape.size(), return NULL) return physicsShape.at(index); } /**< See above. */

	/** Adds a physics shape to the owner.
		@return The new physics shape. */
	FCDPhysicsShape* AddPhysicsShape();

	/** Copies the rigid body parameters into this parameters structure.
		@param original The original rigid body parameters to get values from.
	*/
	virtual void CopyFrom(const FCDPhysicsRigidBodyParameters& original);

	/** [INTERNAL] Reads in the rigid body components from a XML tree node.
		@param techniqueNode The XML technique tree node.
		@param defaultParameters The default parameters. If NULL, then the
			defaults are taken according to the specification for rigid body.
		@return The status of the import. If the status is 'false', it may be 
			dangerous to use the rigid body. */
	virtual bool LoadFromXML(xmlNode* techniqueNode, 
			FCDPhysicsRigidBodyParameters* defaultParameters = NULL);

	/** [INTERNAL] Writes out the \<rigid_body\> element to the given COLLADA 
		XML tree node.
		@param techniqueNode The COLLADA XML technique node in which to insert 
			the rigid constraint.
		@return The created element XML tree node. */
	virtual void WriteToXML(xmlNode* techniqueNode) const;

	/** [INTERNAL] Writes out the \<[name]\> element to the given COLLADA 
		XML tree node, where name is specified. This is used to write physics
		parameters which may also be animated.
		@param parentNode The COLLADA XML parent node in which to insert the 
			element.
		@param name The name of the parameter.
		@param value The value of the parameter.
		@return The created element XML tree node. */
	template <typename T>
	xmlNode* AddPhysicsParameter(xmlNode* parentNode, const char* name, 
			const T& value) const;
};

#endif // _FCD_PHYSICS_RIGID_BODY_PARAMETERS_H_
