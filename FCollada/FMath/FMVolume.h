/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FMVolume.h
	This file contains the functions for working with volumes.
	This includes finding volumes of various shapes.
*/

#ifndef _FM_VOLUME_H_
#define _FM_VOLUME_H_

namespace FMVolume
{
	/** Calculates the volume of an axis-aligned box.
		@param halfExtents The three half-lengths the compose the box. */
	FCOLLADA_EXPORT float CalculateBoxVolume(const FMVector3& halfExtents);
	
	/** Calculates the volume of a sphere.
		@param radius The radius of the sphere. */
	FCOLLADA_EXPORT float CalculateSphereVolume(float radius);
	
	/** Calculates the volume of a cylinder.
		@param radius The radius of the cylinder caps.
		@param height The height of the cylinder shaft. */
	FCOLLADA_EXPORT float CalculateCylinderVolume(float radius, float height);
	
	/** Calculates the volume of a capsule.
		@param radius The radius of the capsule caps.
		@param height The radius of the capsule shaft. */
	FCOLLADA_EXPORT float CalculateCapsuleVolume(float radius, float height);
	
	/** Calculates the volume of a cone.
		@param radius The radius of the cone cap or bottom.
		@param height The height of the cone. */
	FCOLLADA_EXPORT float CalculateConeVolume(float radius, float height);
	
	/** Calculates the volume of a tapered cylinder.
		@param radius The radius of the first cap of the cylinder.
		@param radius2 The radius of the second cap of the cylinder.
		@param height The height of the cylinder shaft. */
	FCOLLADA_EXPORT float CalculateTaperedCylinderVolume(float radius, float radius2, float height);
};

#endif // _FM_VOLUME_H_
