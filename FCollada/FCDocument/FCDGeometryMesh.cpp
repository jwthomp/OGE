/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDLibrary.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

// 
// FCDGeometryMesh
//

ImplementObjectType(FCDGeometryMesh);

FCDGeometryMesh::FCDGeometryMesh(FCDocument* document, FCDGeometry* _parent)
:	FCDObject(document), parent(_parent)
,	faceCount(0), holeCount(0), faceVertexCount(0)
,	isConvex(true), convexify(false)
{
}

FCDGeometryMesh::~FCDGeometryMesh()
{
	polygons.clear();
	sources.clear();
	faceVertexCount = faceCount = holeCount = 0;
	parent = NULL;
}

// Retrieve the parent's id
const fm::string& FCDGeometryMesh::GetDaeId() const
{
	return parent->GetDaeId();
}

void FCDGeometryMesh::SetConvexHullOf(FCDGeometry* _geom)
{
	convexHullOf = _geom->GetDaeId();
	SetDirtyFlag();
}

const FCDGeometryMesh* FCDGeometryMesh::FindConvexHullOfMesh() const
{
	const FCDGeometryMesh* mesh = this;
	while ((mesh != NULL) && !mesh->GetConvexHullOf().empty())
	{
		const FCDocument* document = mesh->GetDocument();
		const FCDGeometry* geometry = document->GetGeometryLibrary()->
				FindDaeId(mesh->GetConvexHullOf());
		if (geometry == NULL) return NULL;
		mesh = geometry->GetMesh();
	}
	return mesh;
}

// Search for a data source in the geometry node
const FCDGeometrySource* FCDGeometryMesh::FindSourceById(const fm::string& id) const
{
	const char* localId = id.c_str();
	if (localId[0] == '#') ++localId;
	for (const FCDGeometrySource** it = sources.begin(); it != sources.end(); ++it)
	{
		if ((*it)->GetDaeId() == localId) return (*it);
	}
	return NULL;
}

// Retrieve the source for the given type
const FCDGeometrySource* FCDGeometryMesh::FindSourceByType(FUDaeGeometryInput::Semantic type) const 
{
	for (const FCDGeometrySource** itS = sources.begin(); itS != sources.end(); ++itS)
	{
		if ((*itS)->GetType() == type) return (*itS);
	}
	return NULL;
}

void FCDGeometryMesh::FindSourcesByType(FUDaeGeometryInput::Semantic type, FCDGeometrySourceConstList& _sources) const
{
	for (const FCDGeometrySource** itS = sources.begin(); itS != sources.end(); ++itS)
	{
		if ((*itS)->GetType() == type) _sources.push_back(*itS);
	}
}

const FCDGeometrySource* FCDGeometryMesh::FindSourceByName(const fstring& name) const 
{
	for (const FCDGeometrySource** itS = sources.begin(); itS != sources.end(); ++itS)
	{
		if ((*itS)->GetName() == name) return (*itS);
	}
	return NULL;
}

// Creates a new polygon group.
FCDGeometryPolygons* FCDGeometryMesh::AddPolygons()
{
	FCDGeometryPolygons* polys = new FCDGeometryPolygons(GetDocument(), this);
	polygons.push_back(polys);

	// Add to this new polygons all the per-vertex sources.
	for (FCDGeometrySource** itS = vertexSources.begin(); itS != vertexSources.end(); ++itS)
	{
		polys->AddInput(*itS, 0);
	}

	SetDirtyFlag();
	if (parent != NULL) parent->SetDirtyFlag();
	return polys;
}

bool FCDGeometryMesh::IsTriangles() const
{
	bool isTriangles = true;
	for (size_t i = 0; i < polygons.size() && isTriangles; i++)
	{
		isTriangles &= polygons[i]->IsTriangles();
	}
	return isTriangles;
}

// Retrieves the polygon sets that use a given material semantic
void FCDGeometryMesh::FindPolygonsByMaterial(const fstring& semantic, FCDGeometryPolygonsList& sets)
{
	for (FCDGeometryPolygons** itP = polygons.begin(); itP != polygons.end(); ++itP)
	{
		if ((*itP)->GetMaterialSemantic() == semantic) sets.push_back(*itP);
	}
}

// Creates a new per-vertex data source
FCDGeometrySource* FCDGeometryMesh::AddVertexSource(FUDaeGeometryInput::Semantic type)
{
	FCDGeometrySource* vertexSource = AddSource(type);
	vertexSources.push_back(vertexSource);

	// Add this new per-vertex data source to all the existing polygon groups, at offset 0.
	for (FCDGeometryPolygons** itP = polygons.begin(); itP != polygons.end(); ++itP)
	{
		(*itP)->AddInput(vertexSource, 0);
	}

	SetDirtyFlag();
	return vertexSource;
}

// Sets a source as per-vertex data.
void FCDGeometryMesh::AddVertexSource(FCDGeometrySource* source)
{
	FUAssert(source != NULL, return);
	FUAssert(!vertexSources.contains(source), return);

	// Add the source to the list of per-vertex sources.
	vertexSources.push_back(source);

	// Remove any polygon set input that uses the source.
	for (FCDGeometryPolygons** itP = polygons.begin(); itP != polygons.end(); ++itP)
	{
		FCDGeometryPolygonsInput* input = (*itP)->FindInput(source);
		int32 set = (input != NULL) ? input->GetSet() : -1;
		SAFE_RELEASE(input);
		input = (*itP)->AddInput(source, 0);
		if (set > -1) input->SetSet(set);
	}

	SetDirtyFlag();
}

// Creates a new data source
FCDGeometrySource* FCDGeometryMesh::AddSource(FUDaeGeometryInput::Semantic type)
{
	FCDGeometrySource* source = new FCDGeometrySource(GetDocument(), type);
	sources.push_back(source);
	SetDirtyFlag();
	return source;
}

// Recalculates all the hole/vertex/face-vertex counts and offsets within the mesh and its polygons
void FCDGeometryMesh::Recalculate()
{
	faceCount = holeCount = faceVertexCount = 0;
	for (FCDGeometryPolygons** itP = polygons.begin(); itP != polygons.end(); ++itP)
	{
		FCDGeometryPolygons* polygons = *itP;
		polygons->Recalculate();

		polygons->SetFaceOffset(faceCount);
		polygons->SetHoleOffset(holeCount);
		polygons->SetFaceVertexOffset(faceVertexCount);
		faceCount += polygons->GetFaceCount();
		holeCount += polygons->GetHoleCount();
		faceVertexCount += polygons->GetFaceVertexCount();
	}
	SetDirtyFlag();
}

FCDGeometryMesh* FCDGeometryMesh::Clone(FCDGeometryMesh* clone) const
{
	if (clone == NULL) clone = new FCDGeometryMesh(const_cast<FCDocument*>(GetDocument()), NULL);

	// Copy the miscellaneous information
	clone->convexHullOf = convexHullOf;
	clone->isConvex = isConvex;
	clone->convexify = convexify;
	clone->faceCount = faceCount;
	clone->holeCount = holeCount;
	clone->faceVertexCount = faceVertexCount;

	// Clone the sources
	FCDGeometrySourceCloneMap cloneMap;
	for (const FCDGeometrySource** itS = sources.begin(); itS != sources.end(); ++itS)
	{
		FCDGeometrySource* clonedSource = (IsVertexSource(*itS)) ? clone->AddVertexSource() : clone->AddSource();
		(*itS)->Clone(clonedSource);
		cloneMap.insert(*itS, clonedSource);
	}

	// Clone the polygon sets.
	for (const FCDGeometryPolygons** itP = polygons.begin(); itP != polygons.end(); ++itP)
	{
		FCDGeometryPolygons* clonedPolys = clone->AddPolygons();
		(*itP)->Clone(clonedPolys, cloneMap);
	}

	return clone;
}

// Link a convex mesh to its source geometry
bool FCDGeometryMesh::Link()
{
	bool status = true;
	if (!isConvex || convexHullOf.empty())
		return status;

	FCDGeometry* concaveGeom = GetDocument()->FindGeometry(convexHullOf);
	if (concaveGeom)
	{
		FCDGeometryMesh* origMesh = concaveGeom->GetMesh();
		if (origMesh != NULL)
		{
			origMesh->Clone(this);
			SetConvexify(true);
			SetConvex(true); // may have been overwritten by clone
		}
		return status;
	}
	else
	{
		//return status.Fail(FS("Unknown geometry for creation of convex hull of: ") + TO_FSTRING(parent->GetDaeId()));
		FUError::Error(FUError::ERROR, FUError::ERROR_UNKNOWN_GEO_CH);
		return status;
	}
}

// Read in the <mesh> node of the COLLADA document
bool FCDGeometryMesh::LoadFromXML(xmlNode* meshNode)
{
	bool status = true;

	if (isConvex) // <convex_mesh> element
		convexHullOf = ReadNodeProperty(meshNode, DAE_CONVEX_HULL_OF_ATTRIBUTE);

	if (isConvex && !convexHullOf.empty())
	{
		return status;
	}

	// Read in the data sources
	xmlNodeList sourceDataNodes;
	FindChildrenByType(meshNode, DAE_SOURCE_ELEMENT, sourceDataNodes);
	for (xmlNodeList::iterator it = sourceDataNodes.begin(); it != sourceDataNodes.end(); ++it)
	{
		FCDGeometrySource* source = AddSource();
		status &= (source->LoadFromXML(*it));
	}

	// Retrieve the <vertices> node
	xmlNode* verticesNode = FindChildByType(meshNode, DAE_VERTICES_ELEMENT);
	if (verticesNode == NULL)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_MESH_VERTICES_MISSING, meshNode->line);
		return status;
	}

	// Read in the per-vertex inputs
	bool hasPositions = false;

	xmlNodeList vertexInputNodes;
	FindChildrenByType(verticesNode, DAE_INPUT_ELEMENT, vertexInputNodes);
	for (xmlNodeList::iterator it = vertexInputNodes.begin(); it < vertexInputNodes.end(); ++it)
	{
		xmlNode* vertexInputNode = *it;
		fm::string inputSemantic = ReadNodeSemantic(vertexInputNode);
		FUDaeGeometryInput::Semantic semantic = FUDaeGeometryInput::FromString(inputSemantic);
		if (semantic != FUDaeGeometryInput::VERTEX)
		{
			fm::string sourceId = ReadNodeSource(vertexInputNode);
			FCDGeometrySource* source = FindSourceById(sourceId);
			if (source == NULL)
			{
				return FUError::Error(FUError::ERROR, FUError::ERROR_UNKNOWN_MESH_ID, vertexInputNode->line);
			}
			source->SetType(semantic);
			if (semantic == FUDaeGeometryInput::POSITION) hasPositions = true;
			vertexSources.push_back(source);
		}
	}
	if (!hasPositions)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_VP_INPUT_NODE_MISSING, verticesNode->line);
	}
	if (vertexSources.empty())
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_GEOMETRY_VERTICES_MISSING, verticesNode->line);
	}

	// Create our rendering object and read in the tessellation
	bool primitivesFound = false;
	xmlNodeList polygonsNodes;
	for (xmlNode* childNode = meshNode->children; childNode != NULL; childNode = childNode->next)
	{
		FCDGeometryPolygons::PrimitiveType primType = (FCDGeometryPolygons::PrimitiveType) -1;

		if (childNode->type != XML_ELEMENT_NODE) continue;
		else if (IsEquivalent(childNode->name, DAE_SOURCE_ELEMENT)) continue;
		else if (IsEquivalent(childNode->name, DAE_VERTICES_ELEMENT)) continue;
		else if (IsEquivalent(childNode->name, DAE_POLYGONS_ELEMENT)
			|| IsEquivalent(childNode->name, DAE_TRIANGLES_ELEMENT)
			|| IsEquivalent(childNode->name, DAE_POLYLIST_ELEMENT)) primType = FCDGeometryPolygons::POLYGONS;
		else if (IsEquivalent(childNode->name, DAE_LINES_ELEMENT)) primType = FCDGeometryPolygons::LINES;
		else if (IsEquivalent(childNode->name, DAE_LINESTRIPS_ELEMENT)) primType = FCDGeometryPolygons::LINE_STRIPS;
		else if (IsEquivalent(childNode->name, DAE_TRIFANS_ELEMENT)) primType = FCDGeometryPolygons::TRIANGLE_FANS;
		else if (IsEquivalent(childNode->name, DAE_TRISTRIPS_ELEMENT)) primType = FCDGeometryPolygons::TRIANGLE_STRIPS;
		else if (IsEquivalent(childNode->name, DAE_POINTS_ELEMENT)) primType = FCDGeometryPolygons::POINTS;
		else continue;

		FUAssert(primType != (FCDGeometryPolygons::PrimitiveType) -1, continue);
		FCDGeometryPolygons* polygon = new FCDGeometryPolygons(GetDocument(), this);
		polygon->SetPrimitiveType(primType);
		status |= polygon->LoadFromXML(childNode);
		polygons.push_back(polygon);
		primitivesFound = true;
	}

	if (!primitivesFound)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_MESH_TESSELLATION_MISSING, meshNode->line);
		return status;
	}

	// Calculate the important statistics/offsets/counts
	Recalculate();
	SetDirtyFlag();
	return status;
}

// Write out the <mesh> node to the COLLADA XML tree
xmlNode* FCDGeometryMesh::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* meshNode = NULL;

	if (isConvex && !convexHullOf.empty())
	{
		meshNode = AddChild(parentNode, DAE_CONVEX_MESH_ELEMENT);
		FUSStringBuilder convexHullOfName(convexHullOf);
		AddAttribute(meshNode, DAE_CONVEX_HULL_OF_ATTRIBUTE, convexHullOfName);
	}
	else
	{
		meshNode = AddChild(parentNode, DAE_MESH_ELEMENT);

		// Write out the sources
		for (const FCDGeometrySource** itS = sources.begin(); itS != sources.end(); ++itS)
		{
			(*itS)->LetWriteToXML(meshNode);
		}

		// Write out the <vertices> element
		xmlNode* verticesNode = AddChild(meshNode, DAE_VERTICES_ELEMENT);
		for (const FCDGeometrySource** itS = vertexSources.begin(); itS != vertexSources.end(); ++itS)
		{
			const char* semantic = FUDaeGeometryInput::ToString((*itS)->GetType());
			AddInput(verticesNode, (*itS)->GetDaeId(), semantic);
		}
		FUSStringBuilder verticesNodeId(GetDaeId()); verticesNodeId.append("-vertices");
		AddAttribute(verticesNode, DAE_ID_ATTRIBUTE, verticesNodeId);

		// Write out the polygons
		for (const FCDGeometryPolygons** itP = polygons.begin(); itP != polygons.end(); ++itP)
		{
			(*itP)->LetWriteToXML(meshNode);
		}
	}
	return meshNode;
}
