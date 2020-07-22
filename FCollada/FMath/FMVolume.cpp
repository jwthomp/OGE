/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMath/FMVolume.h"

namespace FMVolume
{
	float CalculateBoxVolume(const FMVector3& halfExtents)
	{
		return (halfExtents.x*2) * (halfExtents.y*2) * (halfExtents.z*2);
	}

	float CalculateSphereVolume(float radius)
	{
		return (float)(4.0f * FMath::Pi * radius * radius * radius) / 3.0f;
	}

	float CalculateCylinderVolume(float radius, float height)
	{
		return  (float)(FMath::Pi * radius * radius * height);
	}

	float CalculateCapsuleVolume(float radius, float height)
	{
		// 1 cylinder + 1 sphere
		return CalculateCylinderVolume(radius, height) + 
				CalculateSphereVolume(radius);
	}

	float CalculateConeVolume(float radius, float height)
	{
		return (float) FMath::Pi * radius * radius * height / 3.0f;
	}

	float CalculateTaperedCylinderVolume(float radius, float radius2, 
			float height)
	{
		FUAssert(radius != radius2, ;);

		// 1 big cone - 1 small cone
		// find out height of big cone:
		float maxRadius = max(radius, radius2);
		float minRadius = min(radius, radius2);
		float bigHeight = (maxRadius * height) / (maxRadius - minRadius);
		float smallHeight = bigHeight - height;

		return CalculateConeVolume(maxRadius, bigHeight) -
			CalculateConeVolume(minRadius, smallHeight);
	}
}

