/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDPhysicsAnalyticalGeometry.h"
#include "FCDocument/FCDocument.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FMath/FMVolume.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDPhysicsAnalyticalGeometry
//

ImplementObjectType(FCDPhysicsAnalyticalGeometry);

FCDPhysicsAnalyticalGeometry::FCDPhysicsAnalyticalGeometry(FCDocument* document)
:	FCDEntity(document, "AnalyticalGeometry")
{
}

FCDPhysicsAnalyticalGeometry::~FCDPhysicsAnalyticalGeometry()
{
}

FCDEntity* FCDPhysicsAnalyticalGeometry::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	// FCDPhysicsAnalyticalGeometry has no data values to clone.
	// It is also abstract and cannot be created if (_clone == NULL).
	return Parent::Clone(_clone, cloneChildren);
}

// Load from a XML node the given physicsAnalyticalGeometry
bool FCDPhysicsAnalyticalGeometry::LoadFromXML(xmlNode* node)
{
	bool status = FCDEntity::LoadFromXML(node);
	return status;
}

//
// FCDPASBox
//

ImplementObjectType(FCDPASBox);

FCDPASBox::FCDPASBox(FCDocument* document)
:	FCDPhysicsAnalyticalGeometry(document)
{
	halfExtents.x = halfExtents.y = halfExtents.z = 0.f;
}

FCDEntity* FCDPASBox::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPASBox* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASBox(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASBox::GetClassType())) clone = (FCDPASBox*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->halfExtents = halfExtents;
	}
	return _clone;
}

float FCDPASBox::CalculateVolume() const
{
	return FMVolume::CalculateBoxVolume(halfExtents);
}

bool FCDPASBox::LoadFromXML(xmlNode* node)
{
	bool status = true;

	if (!IsEquivalent(node->name, DAE_BOX_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_BOX_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_HALF_EXTENTS_ELEMENT))
		{
			const char* halfExt = ReadNodeContentDirect(child);
			halfExtents.x = FUStringConversion::ToFloat(&halfExt);
			halfExtents.y = FUStringConversion::ToFloat(&halfExt);
			halfExtents.z = FUStringConversion::ToFloat(&halfExt);
		}
	}

	SetDirtyFlag();
	return status;
}

xmlNode* FCDPASBox::WriteToXML(xmlNode* node) const
{
	xmlNode* geomNode = AddChild(node, DAE_BOX_ELEMENT);
	fm::string s = FUStringConversion::ToString(halfExtents);
	AddChild(geomNode, DAE_HALF_EXTENTS_ELEMENT, s);
	return geomNode;
}

//
// FCDPASPlane
//

ImplementObjectType(FCDPASPlane);

FCDPASPlane::FCDPASPlane(FCDocument* document) : FCDPhysicsAnalyticalGeometry(document)
{
	normal.x = normal.y = normal.z = d = 0.f;
}

FCDEntity* FCDPASPlane::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPASPlane* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASPlane(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASPlane::GetClassType())) clone = (FCDPASPlane*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->normal = normal;
	}
	return _clone;
}

float FCDPASPlane::CalculateVolume() const
{
	return 1.0f;
}

bool FCDPASPlane::LoadFromXML(xmlNode* node)
{
	bool status = true;

	if (!IsEquivalent(node->name, DAE_PLANE_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_PLANE_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_EQUATION_ELEMENT))
		{
			const char* eq = ReadNodeContentDirect(child);
			normal.x = FUStringConversion::ToFloat(&eq);
			normal.y = FUStringConversion::ToFloat(&eq);
			normal.z = FUStringConversion::ToFloat(&eq);
			d = FUStringConversion::ToFloat(&eq);
		}
	}

	SetDirtyFlag();
	return status;
}

xmlNode* FCDPASPlane::WriteToXML(xmlNode* node) const
{
	xmlNode* geomNode = AddChild(node, DAE_PLANE_ELEMENT);
	FMVector4 equation;
	equation.w = normal.x; equation.x = normal.y; equation.y = normal.z; equation.z = d;
	fm::string s = FUStringConversion::ToString(equation);
	AddChild(geomNode, DAE_EQUATION_ELEMENT, s);
	return geomNode;
}

//
// FCDPASSphere
//

ImplementObjectType(FCDPASSphere);

FCDPASSphere::FCDPASSphere(FCDocument* document) : FCDPhysicsAnalyticalGeometry(document)
{
	radius = 0.f;
}

FCDEntity* FCDPASSphere::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPASSphere* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASSphere(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASSphere::GetClassType())) clone = (FCDPASSphere*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->radius = radius;
	}
	return _clone;
}

float FCDPASSphere::CalculateVolume() const
{
	return FMVolume::CalculateSphereVolume(radius);
}

bool FCDPASSphere::LoadFromXML(xmlNode* node)
{
	bool status = true;

	if (!IsEquivalent(node->name, DAE_SPHERE_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_SPHERE_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_RADIUS_ELEMENT))
		{
			radius = FUStringConversion::ToFloat(ReadNodeContentDirect(child));
		}
	}

	SetDirtyFlag();
	return status;
}

xmlNode* FCDPASSphere::WriteToXML(xmlNode* node) const
{
	xmlNode* geomNode = AddChild(node, DAE_SPHERE_ELEMENT);
	AddChild(geomNode, DAE_RADIUS_ELEMENT, radius);
	return geomNode;
}

//
// FCDPASCylinder
//

ImplementObjectType(FCDPASCylinder);

FCDPASCylinder::FCDPASCylinder(FCDocument* document) : FCDPhysicsAnalyticalGeometry(document)
{
	height = 0.f;
	radius = 0.f;
}

FCDEntity* FCDPASCylinder::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPASCylinder* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASCylinder(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASCylinder::GetClassType())) clone = (FCDPASCylinder*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->radius = radius;
		clone->height = height;
	}
	return _clone;
}

float FCDPASCylinder::CalculateVolume() const
{
	return FMVolume::CalculateCylinderVolume(radius, height);
}

bool FCDPASCylinder::LoadFromXML(xmlNode* node)
{
	bool status = true;

	if (!IsEquivalent(node->name, DAE_CYLINDER_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_SPHERE_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_HEIGHT_ELEMENT))
		{
			height = FUStringConversion::ToFloat(ReadNodeContentDirect(child));
		}
		else if (IsEquivalent(child->name, DAE_RADIUS_ELEMENT))
		{
			radius = FUStringConversion::ToFloat(ReadNodeContentDirect(child));
		}
	}

	SetDirtyFlag();
	return status;
}

xmlNode* FCDPASCylinder::WriteToXML(xmlNode* node) const
{
	xmlNode* geomNode = AddChild(node, DAE_CYLINDER_ELEMENT);
	AddChild(geomNode, DAE_HEIGHT_ELEMENT, height);
	AddChild(geomNode, DAE_RADIUS_ELEMENT, radius);
	return geomNode;
}

//
// FCDPASCapsule
//

ImplementObjectType(FCDPASCapsule);

FCDPASCapsule::FCDPASCapsule(FCDocument* document) : FCDPhysicsAnalyticalGeometry(document)
{
	height = 0.f;
	radius = 0.f;
}

FCDEntity* FCDPASCapsule::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPASCapsule* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASCapsule(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASCapsule::GetClassType())) clone = (FCDPASCapsule*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->radius = radius;
		clone->height = height;
	}
	return _clone;
}

float FCDPASCapsule::CalculateVolume() const
{
	return FMVolume::CalculateCapsuleVolume(radius, height);
}

bool FCDPASCapsule::LoadFromXML(xmlNode* node)
{
	bool status = true;

	if (!IsEquivalent(node->name, DAE_CAPSULE_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_CAPSULE_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_HEIGHT_ELEMENT))
		{
			height = FUStringConversion::ToFloat(ReadNodeContentDirect(child));
		}
		else if (IsEquivalent(child->name, DAE_RADIUS_ELEMENT))
		{
			radius = FUStringConversion::ToFloat(ReadNodeContentDirect(child));
		}
	}

	SetDirtyFlag();
	return status;
}

xmlNode* FCDPASCapsule::WriteToXML(xmlNode* node) const
{
	xmlNode* geomNode = AddChild(node, DAE_CAPSULE_ELEMENT);
	AddChild(geomNode, DAE_HEIGHT_ELEMENT, height);
	globalSBuilder.set(radius); globalSBuilder.append(' '); globalSBuilder.append(radius);
	fm::string content = globalSBuilder.ToString();
	AddChild(geomNode, DAE_RADIUS_ELEMENT, content);
	return geomNode;
}

//
// FCDPASTaperedCapsule
//

ImplementObjectType(FCDPASTaperedCapsule);

FCDPASTaperedCapsule::FCDPASTaperedCapsule(FCDocument* document) : FCDPASCapsule(document)
{
	radius2 = 0.f;
}

FCDPhysicsAnalyticalGeometry* FCDPASTaperedCapsule::Clone(FCDPhysicsAnalyticalGeometry* _clone, bool cloneChildren) const
{
	FCDPASTaperedCapsule* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASTaperedCapsule(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASTaperedCapsule::GetClassType())) clone = (FCDPASTaperedCapsule*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->radius2 = radius2;
	}
	return _clone;
}

float FCDPASTaperedCapsule::CalculateVolume() const
{
	if (IsEquivalent(radius, radius2)) // this is a capsule
	{
		return FMVolume::CalculateCapsuleVolume(radius, height);
	}

	// 1 tapered cylinder + 1/2 sphere + 1/2 other sphere
	return FMVolume::CalculateTaperedCylinderVolume(radius, radius2, height) 
			+ FMVolume::CalculateSphereVolume(radius) / 2.0f +
			FMVolume::CalculateSphereVolume(radius2) / 2.0f;
}

bool FCDPASTaperedCapsule::LoadFromXML(xmlNode* node)
{
	bool status = true;

	if (!IsEquivalent(node->name, DAE_TAPERED_CAPSULE_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_TCAPSULE_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_HEIGHT_ELEMENT))
		{
			const char* h = ReadNodeContentDirect(child);
			height = FUStringConversion::ToFloat(&h);
		}
		else if (IsEquivalent(child->name, DAE_RADIUS1_ELEMENT))
		{
			const char* rad = ReadNodeContentDirect(child);
			radius = FUStringConversion::ToFloat(&rad);
		}
		else if (IsEquivalent(child->name, DAE_RADIUS2_ELEMENT))
		{
			const char* rad = ReadNodeContentDirect(child);
			radius2 = FUStringConversion::ToFloat(&rad);
		}
	}

	SetDirtyFlag();
	return status;
}

xmlNode* FCDPASTaperedCapsule::WriteToXML(xmlNode* node) const
{
	xmlNode* geomNode = AddChild(node, DAE_TAPERED_CAPSULE_ELEMENT);
	AddChild(geomNode, DAE_HEIGHT_ELEMENT, height);
	AddChild(geomNode, DAE_RADIUS1_ELEMENT, radius);
	AddChild(geomNode, DAE_RADIUS2_ELEMENT, radius2);
	return geomNode;
}

//
// FCDPASTaperedCylinder
//

ImplementObjectType(FCDPASTaperedCylinder);

FCDPASTaperedCylinder::FCDPASTaperedCylinder(FCDocument* document) : FCDPASCylinder(document)
{
	 radius2 = 0.f;
}

FCDEntity* FCDPASTaperedCylinder::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPASTaperedCylinder* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASTaperedCylinder(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASTaperedCylinder::GetClassType())) clone = (FCDPASTaperedCylinder*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->radius2 = radius2;
	}
	return _clone;
}

float FCDPASTaperedCylinder::CalculateVolume() const
{
	if (IsEquivalent(radius, radius2)) // this is a cylinder
	{
		return FMVolume::CalculateCylinderVolume(radius, height);
	}

	return FMVolume::CalculateTaperedCylinderVolume(radius, radius2, height);
}

bool FCDPASTaperedCylinder::LoadFromXML(xmlNode* node)
{
	bool status = true;

	if (!IsEquivalent(node->name, DAE_TAPERED_CYLINDER_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_TCYLINDER_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;
		
		if (IsEquivalent(child->name, DAE_HEIGHT_ELEMENT))
		{
			const char* h = ReadNodeContentDirect(child);
			height = FUStringConversion::ToFloat(&h);
		}
		else if (IsEquivalent(child->name, DAE_RADIUS1_ELEMENT))
		{
			const char* rad = ReadNodeContentDirect(child);
			radius = FUStringConversion::ToFloat(&rad);
		}
		else if (IsEquivalent(child->name, DAE_RADIUS2_ELEMENT))
		{
			const char* rad = ReadNodeContentDirect(child);
			radius2 = FUStringConversion::ToFloat(&rad);
		}
	}

	SetDirtyFlag();
	return status;
}

xmlNode* FCDPASTaperedCylinder::WriteToXML(xmlNode* node) const
{
	xmlNode* geomNode = AddChild(node, DAE_TAPERED_CYLINDER_ELEMENT);
	AddChild(geomNode, DAE_HEIGHT_ELEMENT, height);
	AddChild(geomNode, DAE_RADIUS1_ELEMENT, radius);
	AddChild(geomNode, DAE_RADIUS2_ELEMENT, radius2);
	return geomNode;
}

//
// FCDPASFactory
//

FCDPhysicsAnalyticalGeometry* FCDPASFactory::CreatePAS(FCDocument* document, FCDPhysicsAnalyticalGeometry::GeomType type)
{
	switch (type)
	{
	case FCDPhysicsAnalyticalGeometry::BOX: return new FCDPASBox(document);
	case FCDPhysicsAnalyticalGeometry::PLANE: return new FCDPASPlane(document);
	case FCDPhysicsAnalyticalGeometry::SPHERE: return new FCDPASSphere(document);
	case FCDPhysicsAnalyticalGeometry::CYLINDER: return new FCDPASCylinder(document);
	case FCDPhysicsAnalyticalGeometry::CAPSULE: return new FCDPASCapsule(document);
	case FCDPhysicsAnalyticalGeometry::TAPERED_CYLINDER: return new FCDPASTaperedCylinder(document);
	case FCDPhysicsAnalyticalGeometry::TAPERED_CAPSULE: return new FCDPASTaperedCapsule(document);
	default: return NULL;
	}
}
