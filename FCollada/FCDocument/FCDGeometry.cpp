/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDGeometry);

FCDGeometry::FCDGeometry(FCDocument* document)
:	FCDEntity(document, "Geometry")
{
}

FCDGeometry::~FCDGeometry()
{
}

// Sets the type of this geometry to mesh and creates an empty mesh structure.
FCDGeometryMesh* FCDGeometry::CreateMesh()
{
	spline = NULL;
	mesh = new FCDGeometryMesh(GetDocument(), this);
	SetDirtyFlag();
	return mesh;
}

// Sets the type of this geometry to spline and creates an empty spline structure.
FCDGeometrySpline* FCDGeometry::CreateSpline()
{
	mesh = NULL;
	spline = new FCDGeometrySpline(GetDocument(), this);
	SetDirtyFlag();
	return spline;
}


FCDEntity* FCDGeometry::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDGeometry* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDGeometry(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDGeometry::GetClassType())) clone = (FCDGeometry*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		// Clone the geometric object
		if (IsMesh())
		{
			FCDGeometryMesh* clonedMesh = clone->CreateMesh();
			GetMesh()->Clone(clonedMesh);
		}
		else if (IsSpline())
		{
			FCDGeometrySpline* clonedSpline = clone->CreateSpline();
			GetSpline()->Clone(clonedSpline);
		}
	}
	return clone;
}

// Load from a XML node the given geometry
bool FCDGeometry::LoadFromXML(xmlNode* geometryNode)
{
	mesh = NULL;
	spline = NULL;

	bool status = FCDEntity::LoadFromXML(geometryNode);
	if (!status) return status;
	if (!IsEquivalent(geometryNode->name, DAE_GEOMETRY_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_GL_ELEMENT, geometryNode->line);
		return status;
	}

	// Read in the first valid child element found
	for (xmlNode* child = geometryNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_MESH_ELEMENT))
		{
			// Create a new mesh
			FCDGeometryMesh* m = CreateMesh();
			m->SetConvex(false);
			status &= (m->LoadFromXML(child));
			break;
		}
		else if (IsEquivalent(child->name, DAE_CONVEX_MESH_ELEMENT))
		{
			// Create a new convex mesh
			FCDGeometryMesh* m = CreateMesh();
			m->SetConvex(true);
			status &= (m->LoadFromXML(child));
			break;
		}
		else if (IsEquivalent(child->name, DAE_SPLINE_ELEMENT))
		{
			// Create a new spline
			FCDGeometrySpline* s = CreateSpline();
			status &= (s->LoadFromXML(child));
			break;
		}
	}

	if (mesh == NULL && spline == NULL && !IsPSurface())
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_EMPTY_GEOMETRY, geometryNode->line);
	}

	SetDirtyFlag();
	return status;
}

// Write out the <geometry> node
xmlNode* FCDGeometry::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* geometryNode = WriteToEntityXML(parentNode, DAE_GEOMETRY_ELEMENT);

	if (mesh != NULL) mesh->LetWriteToXML(geometryNode);
	else if (spline != NULL) spline->LetWriteToXML(geometryNode);

	FCDEntity::WriteToExtraXML(geometryNode);
	return geometryNode;
}
