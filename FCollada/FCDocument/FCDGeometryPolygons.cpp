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
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FUtils/FUDaeEnumSyntax.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUStringConversion.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDGeometryPolygons
//

ImplementObjectType(FCDGeometryPolygons);

FCDGeometryPolygons::FCDGeometryPolygons(FCDocument* document, FCDGeometryMesh* _parent)
:	FCDObject(document)
,	parent(_parent)
,	faceVertexCount(0), faceVertexOffset(0)
,	primitiveType(POLYGONS)
{
	// Pre-buffer the face-vertex counts so that AddFaceVertexCount won't be extremely costly.
	faceVertexCounts.reserve(32);
}

FCDGeometryPolygons::~FCDGeometryPolygons()
{
	holeFaces.clear();
	parent = NULL;
}


// Creates a new face.
void FCDGeometryPolygons::AddFace(uint32 degree)
{
	bool newPolygonSet = faceVertexCounts.empty();
	faceVertexCounts.push_back(degree);

	// Inserts empty indices
	for (FCDGeometryPolygonsInputContainer::iterator it = inputs.begin(); it != inputs.end(); ++it)
	{
		FCDGeometryPolygonsInput* input = (*it);
		if (!newPolygonSet && input->OwnsIndices()) input->SetIndexCount(input->GetIndexCount() + degree);
		else if (newPolygonSet && input->GetIndexCount() == 0)
		{
			// Declare this input as the owner!
			input->SetIndexCount(degree);
		}
	}

	parent->Recalculate();
	SetDirtyFlag();
}

// Removes a face
void FCDGeometryPolygons::RemoveFace(size_t index)
{
	FUAssert(index < GetFaceCount(), return);

	// Remove the associated indices, if they exist.
	size_t offset = GetFaceVertexOffset(index);
	size_t indexCount = GetFaceVertexCount(index);
	for (FCDGeometryPolygonsInputContainer::iterator it = inputs.begin(); it != inputs.end(); ++it)
	{
		FCDGeometryPolygonsInput* input = (*it);
		if (!input->OwnsIndices()) continue;

		size_t inputIndexCount = input->GetIndexCount();
		if (offset < inputIndexCount)
		{
			// Move the indices backwards.
			uint32* indices = input->GetIndices();
			for (size_t o = offset; o < inputIndexCount - indexCount; ++o)
			{
				indices[o] = indices[o + indexCount];
			}
			input->SetIndexCount(max(offset, inputIndexCount - indexCount));
		}
	}

	// Remove the face and its holes
	size_t holeBefore = GetHoleCountBefore(index);
	UInt32List::iterator itFV = faceVertexCounts.begin() + index + holeBefore;
	size_t holeCount = GetHoleCount(index);
	faceVertexCounts.erase(itFV, itFV + holeCount + 1); // +1 in order to remove the polygon as well as the holes.

	parent->Recalculate();
	SetDirtyFlag();
}

// Calculates the offset of face-vertex pairs before the given face index within the polygon set.
size_t FCDGeometryPolygons::GetFaceVertexOffset(size_t index) const
{
	size_t offset = 0;

	// We'll need to skip over the holes
	size_t holeCount = GetHoleCountBefore(index);
	if (index + holeCount < faceVertexCounts.size())
	{
		// Sum up the wanted offset
		UInt32List::const_iterator end = faceVertexCounts.begin() + index + holeCount;
		for (UInt32List::const_iterator it = faceVertexCounts.begin(); it != end; ++it)
		{
			offset += (*it);
		}
	}
	return offset;
}

// Calculates the number of holes within the polygon set that appear before the given face index.
size_t FCDGeometryPolygons::GetHoleCountBefore(size_t index) const
{
	size_t holeCount = 0;
	for (UInt32List::const_iterator it = holeFaces.begin(); it != holeFaces.end(); ++it)
	{
		if ((*it) <= index) { ++holeCount; ++index; }
	}
	return holeCount;
}

// Retrieves the number of holes within a given face.
size_t FCDGeometryPolygons::GetHoleCount(size_t index) const
{
	size_t holeCount = 0;
	for (size_t i = index + GetHoleCountBefore(index) + 1; i < faceVertexCounts.size(); ++i)
	{
		bool isHoled = holeFaces.find(i) != holeFaces.end();
		if (!isHoled) break;
		else ++holeCount;
	}
	return holeCount;
}

// The number of face-vertex pairs for a given face.
size_t FCDGeometryPolygons::GetFaceVertexCount(size_t index) const
{
	size_t count = 0;
	if (index < GetFaceCount())
	{
		size_t holeCount = GetHoleCount(index);
		UInt32List::const_iterator it = faceVertexCounts.begin() + index + GetHoleCountBefore(index);
		UInt32List::const_iterator end = it + holeCount + 1; // +1 in order to sum the face-vertex pairs of the polygon as its holes.
		for (; it != end; ++it) count += (*it);
	}
	return count;
}

FCDGeometryPolygonsInput* FCDGeometryPolygons::AddInput(FCDGeometrySource* source, uint32 offset)
{
	FCDGeometryPolygonsInput* input = inputs.Add(this, offset);
	input->SetSource(source);
	SetDirtyFlag();
	return input;
}

void FCDGeometryPolygons::SetHoleFaceCount(size_t count)
{
	holeFaces.resize(count);
	SetDirtyFlag();
}

bool FCDGeometryPolygons::IsHoleFaceHole(size_t index)
{
	return holeFaces.find(index) != holeFaces.end();
}

void FCDGeometryPolygons::AddHole(uint32 index)
{
	FUAssert(!IsHoleFaceHole(index), return);
	UInt32List::iterator it = holeFaces.begin();
	for (; it != holeFaces.end(); ++it)
	{
		if (index < (*it)) break;
	}
	holeFaces.insert(it, index);
}

void FCDGeometryPolygons::AddFaceVertexCount(uint32 count)
{
	faceVertexCounts.push_back(count);
}

void FCDGeometryPolygons::SetFaceVertexCountCount(size_t count)
{
	faceVertexCounts.resize(count);
}

const FCDGeometryPolygonsInput* FCDGeometryPolygons::FindInput(FUDaeGeometryInput::Semantic semantic) const
{
	for (FCDGeometryPolygonsInputContainer::const_iterator it = inputs.begin(); it != inputs.end(); ++it)
	{
		if ((*it)->GetSemantic() == semantic) return (*it);
	}
	return NULL;
}

const FCDGeometryPolygonsInput* FCDGeometryPolygons::FindInput(const FCDGeometrySource* source) const
{
	for (FCDGeometryPolygonsInputContainer::const_iterator it = inputs.begin(); it != inputs.end(); ++it)
	{
		if ((*it)->GetSource() == source) return (*it);
	}
	return NULL;
}

FCDGeometryPolygonsInput* FCDGeometryPolygons::FindInput(const fm::string& sourceId)
{
	const char* s = sourceId.c_str();
	if (*s == '#') ++s;
	for (FCDGeometryPolygonsInputContainer::iterator it = inputs.begin(); it != inputs.end(); ++it)
	{
		if ((*it)->GetSource()->GetDaeId() == s) return (*it);
	}
	return NULL;
}

void FCDGeometryPolygons::FindInputs(FUDaeGeometryInput::Semantic semantic, FCDGeometryPolygonsInputConstList& _inputs) const
{
	for (FCDGeometryPolygonsInputContainer::const_iterator it = inputs.begin(); it != inputs.end(); ++it)
	{
		if ((*it)->GetSemantic() == semantic) _inputs.push_back(*it);
	}
}

// Recalculates the face-vertex count within the polygons
void FCDGeometryPolygons::Recalculate()
{
	faceVertexCount = 0;
	for (UInt32List::iterator itC = faceVertexCounts.begin(); itC != faceVertexCounts.end(); ++itC)
	{
		faceVertexCount += (*itC);
	}
	SetDirtyFlag();
}

bool FCDGeometryPolygons::LoadFromXML(xmlNode* baseNode)
{
	bool status = true;

	// Retrieve the expected face count from the base node's 'count' attribute
	size_t expectedFaceCount = ReadNodeCount(baseNode);

	// Check the node's name to know whether to expect a <vcount> element
	size_t expectedVertexCount; bool isPolygons = false, isTriangles = false, isPolylist = false;
	if (primitiveType == POLYGONS)
	{
		if (IsEquivalent(baseNode->name, DAE_POLYGONS_ELEMENT)) { expectedVertexCount = 4; isPolygons = true; }
		else if (IsEquivalent(baseNode->name, DAE_TRIANGLES_ELEMENT)) { expectedVertexCount = 3 * expectedFaceCount; isTriangles = true; }
		else { FUAssert(IsEquivalent(baseNode->name, DAE_POLYLIST_ELEMENT), return false); expectedVertexCount = 0; isPolylist = true; }
	}
	else
	{
		expectedVertexCount = 0;
		isPolygons = true; // read in the <p> elements. there really shouldn't be any <h> or <ph> elements anyway.
	}

	// Retrieve the material symbol used by these polygons
	materialSemantic = TO_FSTRING(ReadNodeProperty(baseNode, DAE_MATERIAL_ATTRIBUTE));
	if (materialSemantic.empty())
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_POLYGON_MAT_SYMBOL, baseNode->line);
	}

	// Read in the per-face, per-vertex inputs
	xmlNode* itNode = NULL;
	bool hasVertexInput = false;
	FCDGeometryPolygonsInputList idxOwners;
	for (itNode = baseNode->children; itNode != NULL; itNode = itNode->next)
	{
		if (itNode->type != XML_ELEMENT_NODE) continue;
		if (IsEquivalent(itNode->name, DAE_INPUT_ELEMENT))
		{
			fm::string sourceId = ReadNodeSource(itNode);
			if (sourceId[0] == '#') sourceId.erase(0, 1);

			// Parse input idx/offset
			fm::string idx = ReadNodeProperty(itNode, DAE_OFFSET_ATTRIBUTE);
			uint32 offset = (!idx.empty()) ? FUStringConversion::ToUInt32(idx) : (idxOwners.size() + 1);
			if (offset >= idxOwners.size()) idxOwners.resize(offset + 1);

			// Parse input set
			fm::string setString = ReadNodeProperty(itNode, DAE_SET_ATTRIBUTE);
			uint32 set = setString.empty() ? -1 : FUStringConversion::ToInt32(setString);

			// Parse input semantic
			FUDaeGeometryInput::Semantic semantic = FUDaeGeometryInput::FromString(ReadNodeSemantic(itNode));
			if (semantic == FUDaeGeometryInput::UNKNOWN) continue; // Unknown input type
			else if (semantic == FUDaeGeometryInput::VERTEX)
			{
				// There should never be more than one 'VERTEX' input.
				if (hasVertexInput)
				{
					FUError::Error(FUError::WARNING, FUError::WARNING_EXTRA_VERTEX_INPUT, itNode->line);
					continue;
				}
				hasVertexInput = true;

				// Add an input for all the vertex sources in the parent.
				size_t vertexSourceCount = parent->GetVertexSourceCount();
				for (uint32 i = 0; i < vertexSourceCount; ++i)
				{
					FCDGeometrySource* vertexSource = parent->GetVertexSource(i);
					FCDGeometryPolygonsInput* vertexInput = AddInput(vertexSource, offset);
					if (idxOwners[offset] == NULL) idxOwners[offset] = vertexInput;
					vertexInput->SetSet(set);
				}
			}
			else
			{
				// Retrieve the source for this input
				FCDGeometrySource* source = parent->FindSourceById(sourceId);
				if (source != NULL)
				{
					// The source may have a dangling type: the input sets contains that information in the COLLADA document.
					// So: enforce the source type of the input to the data source.
					source->SetType(semantic); 
					FCDGeometryPolygonsInput* input = AddInput(source, offset);
					if (idxOwners[offset] == NULL) idxOwners[offset] = input;
					input->SetSet(set);
				}
				else
				{
					FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_POLYGONS_INPUT, itNode->line);
				}
			}
		}
		else if (IsEquivalent(itNode->name, DAE_POLYGON_ELEMENT)
			|| IsEquivalent(itNode->name, DAE_VERTEXCOUNT_ELEMENT)
			|| IsEquivalent(itNode->name, DAE_POLYGONHOLED_ELEMENT))
		{
			break;
		}
		else
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_POLYGON_CHILD, itNode->line);
		}
	}
	if (itNode == NULL)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_NO_POLYGON, baseNode->line);
		return status;
	}
	if (!hasVertexInput)
	{
		// Verify that we did find a VERTEX polygon set input.
		FUError::Error(FUError::ERROR, FUError::ERROR_NO_VERTEX_INPUT, baseNode->line);
		return status;
	}

	// Look for the <vcount> element and parse it in
	xmlNode* vCountNode = FindChildByType(baseNode, DAE_VERTEXCOUNT_ELEMENT);
	const char* vCountDataString = ReadNodeContentDirect(vCountNode);
	if (vCountDataString != NULL) FUStringConversion::ToUInt32List(vCountDataString, faceVertexCounts);
	bool hasVertexCounts = !faceVertexCounts.empty();
	if (isPolylist && !hasVertexCounts)
	{
		//return status.Fail(FS("No or empty <vcount> element found in geometry: ") + TO_FSTRING(parent->GetDaeId()), baseNode->line);
		FUError::Error(FUError::ERROR, FUError::ERROR_NO_VCOUNT, baseNode->line);
		return status;
	}
	else if (!isPolylist && hasVertexCounts)
	{
		//return status.Fail(FS("<vcount> is only expected with the <polylist> element in geometry: ") + TO_FSTRING(parent->GetDaeId()), baseNode->line);
		FUError::Error(FUError::ERROR, FUError::ERROR_MISPLACED_VCOUNT, baseNode->line);
		return status;
	}
	else if (isPolylist)
	{
		// Count the total number of face-vertices expected, to pre-buffer the index lists
		expectedVertexCount = 0;
		for (UInt32List::iterator itC = faceVertexCounts.begin(); itC != faceVertexCounts.end(); ++itC)
		{
			expectedVertexCount += *itC;
		}
	}

	// Pre-allocate the buffers with enough memory
	UInt32List allIndices;
	faceVertexCount = 0;
	allIndices.clear();
	allIndices.reserve(expectedVertexCount * idxOwners.size());
	for (FCDGeometryPolygonsInputContainer::iterator it = idxOwners.begin(); it != idxOwners.end(); ++it)
	{
		if ((*it) != NULL)
		{
			(*it)->ReserveIndexCount(expectedVertexCount);
		}
	}

	// Process the tessellation
	for (; itNode != NULL; itNode = itNode->next)
	{
		uint32 localFaceVertexCount;
		const char* content = NULL;
		xmlNode* holeNode = NULL;
		bool failed = false;
		if (!InitTessellation(itNode, &localFaceVertexCount, allIndices, content, holeNode, (uint32) idxOwners.size(), &failed)) continue;

		if (failed)
		{
			FUError::Error(FUError::ERROR, FUError::ERROR_UNKNOWN_PH_ELEMENT, itNode->line);
		}

		if (isTriangles) for (uint32 i = 0; i < localFaceVertexCount / 3; ++i) faceVertexCounts.push_back(3);
		else if (isPolygons) faceVertexCounts.push_back(localFaceVertexCount);
		faceVertexCount += localFaceVertexCount;

		// Append any hole indices found
		for (; holeNode != NULL; holeNode = holeNode->next)
		{
			if (holeNode->type != XML_ELEMENT_NODE) continue;

			// Read in the hole indices and push them on top of the other indices
			UInt32List holeIndices; holeIndices.reserve(expectedVertexCount * idxOwners.size());
			content = ReadNodeContentDirect(holeNode);
			FUStringConversion::ToUInt32List(content, holeIndices);
			allIndices.insert(allIndices.end(), holeIndices.begin(), holeIndices.end());

			// Create the hole face and record its index
			size_t holeVertexCount = holeIndices.size() / idxOwners.size();
			holeFaces.push_back((uint32) faceVertexCounts.size());
			faceVertexCounts.push_back((uint32) holeVertexCount);
			faceVertexCount += holeVertexCount;
		}

		// Create a new entry for the vertex buffer
		for (size_t offset = 0; offset < allIndices.size(); offset += idxOwners.size())
		{
			for (FCDGeometryPolygonsInputList::iterator it = idxOwners.begin(); it != idxOwners.end(); ++it)
			{
				if ((*it) != NULL)
				{
					(*it)->AddIndex(allIndices[offset + (*it)->GetOffset()]);
				}
			}
		}
	}

	// Check the actual face count
	if (expectedFaceCount != faceVertexCounts.size() - holeFaces.size())
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_INVALID_FACE_COUNT, baseNode->line);
		return status;
	}

	SetDirtyFlag();
	return status;
}

bool FCDGeometryPolygons::InitTessellation(xmlNode* itNode, 
		uint32* localFaceVertexCount, UInt32List& allIndices, 
		const char* content, xmlNode*& holeNode, uint32 idxCount, 
		bool* failed)
{
	if (itNode->type != XML_ELEMENT_NODE) return false;
	if (!IsEquivalent(itNode->name, DAE_POLYGON_ELEMENT) 
		&& !IsEquivalent(itNode->name, DAE_POLYGONHOLED_ELEMENT)) return false;

	// Retrieve the indices
	content = NULL;
	holeNode = NULL;
	if (!IsEquivalent(itNode->name, DAE_POLYGONHOLED_ELEMENT)) 
	{
		content = ReadNodeContentDirect(itNode);
	} 
	else 
	{
		// Holed face found
		for (xmlNode* child = itNode->children; child != NULL; child = child->next)
		{
			if (child->type != XML_ELEMENT_NODE) continue;
			if (IsEquivalent(child->name, DAE_POLYGON_ELEMENT)) 
			{
				content = ReadNodeContentDirect(child);
			}
			else if (IsEquivalent(child->name, DAE_HOLE_ELEMENT)) 
			{ 
				holeNode = child; break; 
			}
			else 
			{
				*failed = true;
				return true;
			}
		}
	}

	// Parse the indices
	allIndices.resize(0);
	FUStringConversion::ToUInt32List(content, allIndices);
	*localFaceVertexCount = (uint32) allIndices.size() / idxCount;
	SetDirtyFlag();
	return true;
}

bool FCDGeometryPolygons::IsTriangles() const
{
	UInt32List::const_iterator itC;
	for (itC = faceVertexCounts.begin(); itC != faceVertexCounts.end() && (*itC) == 3; ++itC) {}
	return (itC == faceVertexCounts.end());
}

// Write out the polygons structure to the COLLADA XML tree
xmlNode* FCDGeometryPolygons::WriteToXML(xmlNode* parentNode) const
{
	// Are there holes? Then, export a <polygons> element.
	// Are there only non-triangles within the list? Then, export a <polylist> element.
	// Otherwise, you only have triangles: export a <triangles> element.
	// That's all nice for polygon lists, otherwise we export the correct primitive type.
	bool hasHoles = false, hasNPolys = true;

	// Create the base node for these polygons
	const char* polygonNodeType;
	switch (primitiveType)
	{
	case POLYGONS:
		// Check for polygon with holes and triangle-only conditions.
		hasHoles = !holeFaces.empty();
		if (!hasHoles) hasNPolys = !IsTriangles();

		if (hasHoles) polygonNodeType = DAE_POLYGONS_ELEMENT;
		else if (hasNPolys) polygonNodeType = DAE_POLYLIST_ELEMENT;
		else polygonNodeType = DAE_TRIANGLES_ELEMENT;
		break;

	case LINES: polygonNodeType = DAE_LINES_ELEMENT; break;
	case LINE_STRIPS: polygonNodeType = DAE_LINESTRIPS_ELEMENT; break;
	case TRIANGLE_FANS: polygonNodeType = DAE_TRIFANS_ELEMENT; break;
	case TRIANGLE_STRIPS: polygonNodeType = DAE_TRISTRIPS_ELEMENT; break;
	case POINTS: polygonNodeType = DAE_POINTS_ELEMENT; break;
	default: polygonNodeType = emptyString.c_str(); FUFail(break); break;
	}
	xmlNode* polygonsNode = AddChild(parentNode, polygonNodeType);

	// Add the inputs
	// Find which input owner belongs to the <vertices> element. Replace the semantic and the source id accordingly.
	// Make sure to add that 'vertex' input only once.
	FUSStringBuilder verticesNodeId(parent->GetDaeId()); verticesNodeId.append("-vertices");
	bool isVertexInputFound = false;
	fm::pvector<const FCDGeometryPolygonsInput> idxOwners; // Record a list of input data owners.
	for (FCDGeometryPolygonsInputContainer::const_iterator itI = inputs.begin(); itI != inputs.end(); ++itI)
	{
		const FCDGeometryPolygonsInput* input = *itI;
		const FCDGeometrySource* source = input->GetSource();
		if (source != NULL)
		{
			if (!parent->IsVertexSource(source))
			{
				const char* semantic = FUDaeGeometryInput::ToString(input->GetSemantic());
				FUDaeWriter::AddInput(polygonsNode, source->GetDaeId(), semantic, input->GetOffset(), input->GetSet());
			}
			else if (!isVertexInputFound)
			{
				FUDaeWriter::AddInput(polygonsNode, verticesNodeId.ToCharPtr(), DAE_VERTEX_INPUT, input->GetOffset());
				isVertexInputFound = true;
			}
		}

		if (input->OwnsIndices())
		{
			if (input->GetOffset() >= idxOwners.size()) idxOwners.resize(input->GetOffset() + 1);
			idxOwners[input->GetOffset()] = input;
		}
	}

	FUSStringBuilder builder;
	builder.reserve(1024);

	// For the poly-list case, export the list of vertex counts
	if (!hasHoles && hasNPolys)
	{
		FUStringConversion::ToString(builder, faceVertexCounts);
		xmlNode* vcountNode = AddChild(polygonsNode, DAE_VERTEXCOUNT_ELEMENT);
		AddContentUnprocessed(vcountNode, builder.ToCharPtr());
		builder.clear();
	}

	// For the non-holes cases, open only one <p> element for all the data indices
	xmlNode* pNode = NULL,* phNode = NULL;
	if (!hasHoles) pNode = AddChild(polygonsNode, DAE_POLYGON_ELEMENT);

	// Export the data indices (tessellation information)
	size_t faceCount = GetFaceCount();
	uint32 faceVertexOffset = 0;
	size_t holeOffset = 0;
	for (size_t faceIndex = 0; faceIndex < faceCount; ++faceIndex)
	{
		// For the holes cases, verify whether this face or the next one(s) are holes. We may need to open a new <ph>/<p> element
		size_t holeCount = 0;
		if (hasHoles)
		{
			holeCount = GetHoleCount(faceIndex);

			if (holeCount == 0)
			{
				// Just open a <p> element: this is the most common case
				pNode = AddChild(polygonsNode, DAE_POLYGON_ELEMENT);
			}
			else
			{
				// Open up a new <ph> element and its <p> element
				phNode = AddChild(polygonsNode, DAE_POLYGONHOLED_ELEMENT);
				pNode = AddChild(phNode, DAE_POLYGON_ELEMENT);
			}
		}

		for (size_t holeIndex = 0; holeIndex < holeCount + 1; ++holeIndex)
		{
			// Write out the tessellation information for all the vertices of this face
			uint32 faceVertexCount = faceVertexCounts[faceIndex + holeOffset + holeIndex];
			for (uint32 faceVertexIndex = faceVertexOffset; faceVertexIndex < faceVertexOffset + faceVertexCount; ++faceVertexIndex)
			{
				for (fm::pvector<const FCDGeometryPolygonsInput>::iterator itI = idxOwners.begin(); itI != idxOwners.end(); ++itI)
				{
					if ((*itI) != NULL)
					{
						builder.append((*itI)->GetIndices()[faceVertexIndex]);
						builder.append(' ');
					}
					else builder.append("0 ");
				}
			}

			// For the holes cases: write out the indices for every polygon element
			if (hasHoles)
			{
				if (!builder.empty()) builder.pop_back(); // take out the last space
				AddContentUnprocessed(pNode, builder.ToCharPtr());
				builder.clear();

				if (holeIndex < holeCount)
				{
					// Open up a <h> element
					pNode = AddChild(phNode, DAE_HOLE_ELEMENT);
				}
			}

			faceVertexOffset += faceVertexCount;
		}
		holeOffset += holeCount;
	}

	// For the non-holes cases: write out the indices at the very end, for the single <p> element
	if (!hasHoles)
	{
		if (!builder.empty()) builder.pop_back(); // take out the last space
		AddContentUnprocessed(pNode, builder.ToCharPtr());
	}

	// Write out the material semantic and the number of polygons
	if (!materialSemantic.empty())
	{
		AddAttribute(polygonsNode, DAE_MATERIAL_ATTRIBUTE, materialSemantic);
	}
	AddAttribute(polygonsNode, DAE_COUNT_ATTRIBUTE, GetFaceCount());

	return polygonsNode;
}

// Clone this list of polygons
FCDGeometryPolygons* FCDGeometryPolygons::Clone(FCDGeometryPolygons* clone, const FCDGeometrySourceCloneMap& cloneMap) const
{
	if (clone == NULL) return NULL;

	// Clone the miscellaneous information.
	clone->materialSemantic = materialSemantic;
	clone->faceVertexCounts = faceVertexCounts;
	clone->faceOffset = faceOffset;
	clone->faceVertexCount = faceVertexCount;
	clone->faceVertexOffset = faceVertexOffset;
	clone->holeOffset = holeOffset;
	clone->holeFaces = holeFaces;
	
	// Clone the geometry inputs
	// Note that the vertex source inputs are usually created by default.
	size_t inputCount = inputs.size();
	clone->inputs.reserve(inputCount);
	for (size_t i = 0; i < inputCount; ++i)
	{
		// Find the cloned source that correspond to the original input.
		FCDGeometrySource* cloneSource = NULL;
		FCDGeometrySourceCloneMap::const_iterator it = cloneMap.find(inputs[i]->GetSource());
		if (it == cloneMap.end())
		{
			// Attempt to match by ID instead.
			const fm::string& id = inputs[i]->GetSource()->GetDaeId();
			cloneSource = clone->GetParent()->FindSourceById(id);
		}
		else
		{
			cloneSource = (*it).second;
		}

		// Retrieve or create the input to clone.
		FCDGeometryPolygonsInput* input = clone->FindInput(cloneSource);
		if (input == NULL)
		{
			input = clone->AddInput(cloneSource, inputs[i]->GetOffset());
		}

		// Clone the input information.
		if (inputs[i]->OwnsIndices())
		{
			input->SetIndices(inputs[i]->GetIndices(), inputs[i]->GetIndexCount());
		}
		input->SetSet(inputs[i]->GetSet());
	}

	return clone;
}

//
// FCDGeometryPolygonsInput
//

ImplementObjectType(FCDGeometryPolygonsInput);

FCDGeometryPolygonsInput::FCDGeometryPolygonsInput(FCDGeometryPolygons* _parent, uint32 offset)
:	parent(_parent), source(NULL)
,	set(-1), idx(offset)
{
}

FCDGeometryPolygonsInput::~FCDGeometryPolygonsInput()
{
	if (source != NULL)
	{
		UntrackObject(source);
		source = NULL;
	}
}

FUDaeGeometryInput::Semantic FCDGeometryPolygonsInput::GetSemantic() const
{
	FUAssert(source != NULL, return FUDaeGeometryInput::UNKNOWN);
	return source->GetType();
}

// Sets the referenced source
void FCDGeometryPolygonsInput::SetSource(FCDGeometrySource* _source)
{
	// Untrack the old source and track the new source
	if (source != NULL) UntrackObject(source);
	source = _source;
	if (source != NULL) TrackObject(source);
}

// Callback when the tracked source is released.
void FCDGeometryPolygonsInput::OnObjectReleased(FUObject* object)
{
	if (source == object)
	{
		source = NULL;
		
		// Verify whether we own/share the index list.
		if (!indices.empty())
		{
			size_t inputCount = parent->GetInputCount();
			for (size_t i = 0; i < inputCount; ++i)
			{
				FCDGeometryPolygonsInput* other = parent->GetInput(i);
				if (other->idx == idx)
				{
					// Move the shared list of indices to the other input.
					other->indices = indices;
					indices.clear();
					break;
				}
			}
		}
	}
}

void FCDGeometryPolygonsInput::SetIndices(const uint32* _indices, size_t count)
{
	UInt32List* indices = FindIndices();
	if (count > 0)
	{
		indices->resize(count);
		memcpy(&indices->front(), _indices, count * sizeof(uint32));
	}
	else indices->clear();
}

void FCDGeometryPolygonsInput::SetIndexCount(size_t count)
{
	UInt32List* indices = FindIndices();
	indices->resize(count);
}

const uint32* FCDGeometryPolygonsInput::GetIndices() const
{
	const UInt32List* indices = FindIndices();
	return !indices->empty() ? &indices->front() : NULL;
}

size_t FCDGeometryPolygonsInput::GetIndexCount() const
{
	const UInt32List* indices = FindIndices();
	return indices->size();
}

void FCDGeometryPolygonsInput::ReserveIndexCount(size_t count)
{
	UInt32List* indices = FindIndices();
	if (count > indices->size()) indices->reserve(count);
}

void FCDGeometryPolygonsInput::AddIndex(uint32 index)
{
	UInt32List* indices = FindIndices();
	indices->push_back(index);
}

const UInt32List* FCDGeometryPolygonsInput::FindIndices() const
{
	if (OwnsIndices()) return &indices; // Early exit for local owner.

	size_t inputCount = parent->GetInputCount();
	for (size_t i = 0; i < inputCount; ++i)
	{
		FCDGeometryPolygonsInput* input = parent->GetInput(i);
		if (input->idx == idx && input->OwnsIndices()) return &input->indices;
	}

	// No indices allocated yet.
	return &indices;
}
