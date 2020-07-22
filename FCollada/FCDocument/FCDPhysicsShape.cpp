/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDPhysicsShape.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDTransform.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDLibrary.hpp"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsMaterial.h"
#include "FCDocument/FCDPhysicsAnalyticalGeometry.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUBoundingBox.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDPhysicsShape);

FCDPhysicsShape::FCDPhysicsShape(FCDocument* document) : FCDObject(document)
{
	hollow = true; // COLLADA 1.4.1 no default specified
	physicsMaterial = NULL;
	ownsPhysicsMaterial = false;
	isDensityMoreAccurate = false;
	geometry = NULL;
	analGeom = NULL;
	mass = NULL;
	density = NULL;
	instanceMaterialRef = NULL;
}

FCDPhysicsShape::~FCDPhysicsShape()
{
	SetPhysicsMaterial(NULL);
	SAFE_DELETE(mass);
	SAFE_DELETE(density);
	SAFE_RELEASE(instanceMaterialRef);

	if (ownsPhysicsMaterial) SAFE_RELEASE(physicsMaterial);
	SAFE_RELEASE(geometry);
	geometry = NULL;
}

FCDTransform* FCDPhysicsShape::AddTransform(FCDTransform::Type type, size_t index)
{
	FCDTransform* transform = FCDTFactory::CreateTransform(GetDocument(), NULL, type);
	if (transform != NULL)
	{
		if (index > transforms.size()) transforms.push_back(transform);
		else transforms.insert(transforms.begin() + index, transform);
	}
	SetDirtyFlag();
	return transform;
}

FCDPhysicsMaterial* FCDPhysicsShape::AddOwnPhysicsMaterial()
{
	if (ownsPhysicsMaterial) SAFE_RELEASE(physicsMaterial);

	physicsMaterial = new FCDPhysicsMaterial(GetDocument());
	ownsPhysicsMaterial = true;
	SetDirtyFlag();
	return physicsMaterial;
}

void FCDPhysicsShape::SetPhysicsMaterial(FCDPhysicsMaterial* _physicsMaterial)
{
	if (ownsPhysicsMaterial) SAFE_RELEASE(physicsMaterial);
	ownsPhysicsMaterial = false;
	physicsMaterial = _physicsMaterial;
	SetDirtyFlag();
}

FCDGeometryInstance* FCDPhysicsShape::CreateGeometryInstance(FCDGeometry* geom, bool createConvexMesh)
{
	analGeom = NULL;
	SAFE_RELEASE(geometry);
	
	geometry = (FCDGeometryInstance*)FCDEntityInstanceFactory::CreateInstance(GetDocument(), NULL, FCDEntity::GEOMETRY);

	if (createConvexMesh)
	{
		FCDGeometry* convexHullGeom = GetDocument()->GetGeometryLibrary()->AddEntity();
		fm::string convexId = geom->GetDaeId()+"-convex";
		convexHullGeom->SetDaeId(convexId);
		convexHullGeom->SetName(FUStringConversion::ToFString(convexId));
		FCDGeometryMesh* convexHullGeomMesh = convexHullGeom->CreateMesh();
		convexHullGeomMesh->SetConvexHullOf(geom);
		convexHullGeomMesh->SetConvex(true);
		geometry->SetEntity(convexHullGeom);
	}
	else
	{
		geometry->SetEntity(geom);
	}

	SetDirtyFlag();
	return geometry;
}

FCDPhysicsAnalyticalGeometry* FCDPhysicsShape::CreateAnalyticalGeometry(FCDPhysicsAnalyticalGeometry::GeomType type)
{
	SAFE_RELEASE(geometry);
	analGeom = FCDPASFactory::CreatePAS(GetDocument(), type);
	SetDirtyFlag();
	return analGeom;
}

// Create a copy of this shape
// Note: geometries are just shallow-copied
FCDPhysicsShape* FCDPhysicsShape::Clone(FCDPhysicsShape* clone) const
{
	if (clone == NULL) clone = new FCDPhysicsShape(const_cast<FCDocument*>(GetDocument()));

	if (mass != NULL) clone->SetMass(*mass);
	if (density != NULL) clone->SetDensity(*density);
	clone->SetHollow(hollow);

	// Clone the material instance
	if (instanceMaterialRef != NULL)
	{
		clone->instanceMaterialRef = FCDEntityInstanceFactory::CreateInstance(clone->GetDocument(), NULL, FCDEntity::PHYSICS_MATERIAL);
		instanceMaterialRef->Clone(instanceMaterialRef);
	}
	if (physicsMaterial != NULL)
	{
		FCDPhysicsMaterial* clonedMaterial = clone->AddOwnPhysicsMaterial();
		physicsMaterial->Clone(clonedMaterial);
	}

	// Clone the analytical geometry or the mesh geometry
	if (analGeom != NULL)
	{
		clone->analGeom = FCDPASFactory::CreatePAS(clone->GetDocument(), analGeom->GetGeomType());
		analGeom->Clone(clone->analGeom);
	}
	if (geometry != NULL)
	{
		clone->geometry = (FCDGeometryInstance*)FCDEntityInstanceFactory::CreateInstance(clone->GetDocument(), NULL, geometry->GetEntityType());
		geometry->Clone(clone->geometry);
	}

	// Clone the shape placement transform
	for (size_t i = 0; i < transforms.size(); ++i)
	{
		FCDTransform* clonedTransform = clone->AddTransform(transforms[i]->GetType());
		transforms[i]->Clone(clonedTransform);
	}

	return clone;
}


// Load from a XML node the given physicsShape
bool FCDPhysicsShape::LoadFromXML(xmlNode* physicsShapeNode)
{
	bool status = true;
	if (!IsEquivalent(physicsShapeNode->name, DAE_SHAPE_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOW_PS_LIB_ELEMENT, physicsShapeNode->line);
		return status;
	}

	// Read in the first valid child element found
	for (xmlNode* child = physicsShapeNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_HOLLOW_ELEMENT))
		{
			hollow = FUStringConversion::ToBoolean(ReadNodeContentDirect(child));
		}
		else if (IsEquivalent(child->name, DAE_MASS_ELEMENT))
		{
			SAFE_DELETE(mass);
			mass = new float;
			*mass = FUStringConversion::ToFloat(ReadNodeContentDirect(child));
			isDensityMoreAccurate = false;
		}
		else if (IsEquivalent(child->name, DAE_DENSITY_ELEMENT))
		{
			SAFE_DELETE(density);
			density = new float;
			*density = FUStringConversion::ToFloat(ReadNodeContentDirect(child));
			isDensityMoreAccurate = (mass == NULL); // mass before density in COLLADA 1.4.1
		}
		else if (IsEquivalent(child->name, DAE_PHYSICS_MATERIAL_ELEMENT))
		{
			FCDPhysicsMaterial* material = AddOwnPhysicsMaterial();
			material->LoadFromXML(child);
		}
		else if (IsEquivalent(child->name, 
				DAE_INSTANCE_PHYSICS_MATERIAL_ELEMENT))
		{
			instanceMaterialRef = FCDEntityInstanceFactory::CreateInstance(GetDocument(), NULL, FCDEntity::PHYSICS_MATERIAL);
			instanceMaterialRef->LoadFromXML(child);

			if (!HasNodeProperty(child, DAE_URL_ATTRIBUTE))
			{
				//inline definition of physics_material
				FCDPhysicsMaterial* material = AddOwnPhysicsMaterial();
				material->LoadFromXML(child);
				instanceMaterialRef->SetEntity(material);
			}
		}
		else if (IsEquivalent(child->name, DAE_INSTANCE_GEOMETRY_ELEMENT))
		{
			FUUri url = ReadNodeUrl(child);
			if (url.prefix.empty())
			{
				FCDGeometry* entity = GetDocument()->FindGeometry(url.suffix);
				if (entity != NULL)
				{
					analGeom = NULL;
					geometry = (FCDGeometryInstance*)FCDEntityInstanceFactory::CreateInstance(GetDocument(), NULL, FCDEntity::GEOMETRY);
					geometry->SetEntity((FCDEntity*)entity);
					status &= (geometry->LoadFromXML(child));
					continue; 
				}
			}
			FUError::Error(FUError::WARNING, FUError::WARNING_FCDGEOMETRY_INST_MISSING, child->line);
		}

#define PARSE_ANALYTICAL_SHAPE(type, nodeName) \
		else if (IsEquivalent(child->name, nodeName)) { \
			FCDPhysicsAnalyticalGeometry* analytical = CreateAnalyticalGeometry(FCDPhysicsAnalyticalGeometry::type); \
			status = analytical->LoadFromXML(child); \
			if (!status) { \
				FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_SHAPE_NODE, child->line); break; \
			} }

		PARSE_ANALYTICAL_SHAPE(BOX, DAE_BOX_ELEMENT)
		PARSE_ANALYTICAL_SHAPE(PLANE, DAE_PLANE_ELEMENT)
		PARSE_ANALYTICAL_SHAPE(SPHERE, DAE_SPHERE_ELEMENT)
		PARSE_ANALYTICAL_SHAPE(CYLINDER, DAE_CYLINDER_ELEMENT)
		PARSE_ANALYTICAL_SHAPE(CAPSULE, DAE_CAPSULE_ELEMENT)
		PARSE_ANALYTICAL_SHAPE(TAPERED_CAPSULE, DAE_TAPERED_CAPSULE_ELEMENT)
		PARSE_ANALYTICAL_SHAPE(TAPERED_CYLINDER, DAE_TAPERED_CYLINDER_ELEMENT)
#undef PARSE_ANALYTICAL_SHAPE


		// Parse the physics shape transforms <rotate>, <translate> are supported.
		else if (IsEquivalent(child->name, DAE_ASSET_ELEMENT)) {}
		else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT)) {}
		else
		{
			FCDTransform* transform = FCDTFactory::CreateTransform(GetDocument(), NULL, child);
			if (transform != NULL && (transform->GetType() != FCDTransform::TRANSLATION
				&& transform->GetType() != FCDTransform::ROTATION))
			{
				SAFE_RELEASE(transform);
			}
			else if (transform != NULL)
			{
				transforms.push_back(transform);
				status &= (transform->LoadFromXML(child));
			}
		}
	}

	if ((mass == NULL) && (density == NULL))
	{
		density = new float;
		*density = 1.0f;
		isDensityMoreAccurate = true;
	}

	// default value if only one is defined.
	if ((mass == NULL) && (density != NULL))
	{
		mass = new float;
		*mass = *density * CalculateVolume();
	}
	else if ((mass != NULL) && (density == NULL))
	{
		density = new float;
		*density = *mass / CalculateVolume();
	}

	SetDirtyFlag();
	return status;
}

// Write out the <physicsShape> node
xmlNode* FCDPhysicsShape::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* physicsShapeNode = AddChild(parentNode, DAE_SHAPE_ELEMENT);

	AddChild(physicsShapeNode, DAE_HOLLOW_ELEMENT, hollow?"true":"false");
	if (mass && !isDensityMoreAccurate)
		AddChild(physicsShapeNode, DAE_MASS_ELEMENT, FUStringConversion::ToString(*mass));
	if (density)
		AddChild(physicsShapeNode, DAE_DENSITY_ELEMENT, FUStringConversion::ToString(*density));

	if (ownsPhysicsMaterial && physicsMaterial)
	{
		xmlNode* materialNode = AddChild(physicsShapeNode, DAE_PHYSICS_MATERIAL_ELEMENT);
		physicsMaterial->LetWriteToXML(materialNode);
	}
	else if (instanceMaterialRef)
	{
		instanceMaterialRef->LetWriteToXML(physicsShapeNode);
	}
	
	if (geometry)
		geometry->LetWriteToXML(physicsShapeNode);
	if (analGeom)
		analGeom->LetWriteToXML(physicsShapeNode);

	for (FCDTransformContainer::const_iterator it = transforms.begin(); it != transforms.end(); ++it)
	{
		(*it)->LetWriteToXML(physicsShapeNode);
	}

	return physicsShapeNode;
}


float FCDPhysicsShape::GetMass() const
{
	if (mass) 
		return *mass;
		
	return 0.f;
}

void FCDPhysicsShape::SetMass(float _mass)
{
	SAFE_DELETE(mass);
	mass = new float;
	*mass = _mass;
	SetDirtyFlag();
}

float FCDPhysicsShape::GetDensity()  const
{
	if (density) 
		return *density; 

	return 0.f;
}

void FCDPhysicsShape::SetDensity(float _density) 
{
	SAFE_DELETE(density);
	density = new float;
	*density = _density;
	SetDirtyFlag();
}

float FCDPhysicsShape::CalculateVolume() const
{
	if (IsGeometryInstance())
	{
		FCDGeometry* geom = ((FCDGeometry*)geometry->GetEntity());
		if (geom->IsMesh())
		{
			FUBoundingBox boundary;
			float countingVolume = 0.0f;
			const FCDGeometryMesh* mesh = geom->GetMesh();

			if (!mesh->GetConvexHullOf().empty())
			{
				mesh = mesh->FindConvexHullOfMesh();
			}
			if (mesh == NULL) return 1.0f; // missing convex hull or of spline

			for (size_t i = 0; i < mesh->GetPolygonsCount(); i++)
			{
				const FCDGeometryPolygons* polygons = mesh->GetPolygons(i);
				const FCDGeometryPolygonsInput* positionInput = 
						polygons->FindInput(FUDaeGeometryInput::POSITION);
				const FCDGeometrySource* positionSource = 
						positionInput->GetSource();
				uint32 positionStride = positionSource->GetStride();
				FUAssert(positionStride == 3, continue; );
				const float* positionData = positionSource->GetData();
				size_t positionDataLength = positionSource->GetDataCount();
				for (size_t pos = 0; pos < positionDataLength;)
				{
					boundary.Include(FMVector3(positionData, (uint32)pos));
					pos += positionStride;
				}

				FMVector3 min = boundary.GetMin();
				FMVector3 max = boundary.GetMax();
				countingVolume += 
						(max.x - min.x) * (max.y - min.y) * (max.z - min.z);
				boundary.Reset();
			}
			return countingVolume;
		}
		// splines have no volume!
		return 1.0f;
	}
	else
	{
		FUAssert(IsAnalyticalGeometry(), return 1.0f; );
		return (analGeom->CalculateVolume());
	}
}
