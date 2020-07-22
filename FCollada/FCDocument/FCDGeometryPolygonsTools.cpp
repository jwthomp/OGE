/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsTools.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDAnimated.h"

namespace FCDGeometryPolygonsTools
{
	// Triangulates a mesh.
	void Triangulate(FCDGeometryMesh* mesh)
	{
		if (mesh == NULL) return;

		size_t polygonsCount = mesh->GetPolygonsCount();
		for (size_t i = 0; i < polygonsCount; ++i)
		{
			Triangulate(mesh->GetPolygons(i), false);
		}

		// Recalculate the mesh/polygons statistics
		mesh->Recalculate();
	}

	// Triangulates a polygons set.
	void Triangulate(FCDGeometryPolygons* polygons, bool recalculate)
	{
		if (polygons == NULL) return;

		// Pre-allocate and ready the end index/count buffers
		size_t oldFaceCount = polygons->GetFaceVertexCountCount();
		UInt32List oldFaceVertexCounts(polygons->GetFaceVertexCounts(), oldFaceCount);
		polygons->SetFaceVertexCountCount(0);
		fm::pvector<FCDGeometryPolygonsInput> indicesOwners;
		fm::vector<UInt32List> oldDataIndices;
		size_t inputCount = polygons->GetInputCount();
		FCDGeometryPolygonsInput** inputs = polygons->GetInputs();
		for (size_t i = 0; i < inputCount; ++i)
		{
			FCDGeometryPolygonsInput* input = inputs[i];
			if (input->GetIndexCount() == 0) continue;
			uint32* indices = input->GetIndices();
			size_t oldIndexCount = input->GetIndexCount();
			oldDataIndices.push_back(UInt32List(indices, oldIndexCount));
			indicesOwners.push_back(input);
			input->SetIndexCount(0);
			input->ReserveIndexCount(oldIndexCount);
		}
		size_t dataIndicesCount = oldDataIndices.size();

		// Rebuild the index/count buffers through simple fan-triangulation.
		// Drop holes and polygons with less than three vertices. 
		size_t oldOffset = 0;
		for (size_t oldFaceIndex = 0; oldFaceIndex < oldFaceCount; ++oldFaceIndex)
		{
			size_t oldFaceVertexCount = oldFaceVertexCounts[oldFaceIndex];
			bool isHole = polygons->IsHoleFaceHole((uint32) oldFaceIndex);
			if (!isHole && oldFaceVertexCount >= 3)
			{
				// Fan-triangulation: works well on convex polygons.
				size_t triangleCount = oldFaceVertexCount - 2;
				for (size_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
				{
					for (size_t j = 0; j < dataIndicesCount; ++j)
					{
						UInt32List& oldData = oldDataIndices[j];
						FCDGeometryPolygonsInput* input = indicesOwners[j];
						input->AddIndex(oldData[oldOffset]);
						input->AddIndex(oldData[oldOffset + triangleIndex + 1]);
						input->AddIndex(oldData[oldOffset + triangleIndex + 2]);
					}
					polygons->AddFaceVertexCount(3);
				}
			}
			oldOffset += oldFaceVertexCount;
		}

		polygons->SetHoleFaceCount(0);

		if (recalculate) polygons->Recalculate();
	}

	static uint32 CompressSortedVector(FMVector3& toInsert, FloatList& insertedList, UInt32List& compressIndexReferences)
	{
		// Look for this vector within the already inserted list.
		size_t start = 0, end = compressIndexReferences.size(), mid;
		for (mid = (start + end) / 2; start < end; mid = (start + end) / 2)
		{
			uint32 index = compressIndexReferences[mid];
			if (toInsert.x == insertedList[3 * index]) break;
			else if (toInsert.x < insertedList[3 * index]) end = mid;
			else start = mid + 1;
		}

		// Look for the tolerable range within the binary-sorted dimension.
		size_t rangeStart, rangeEnd;
		for (rangeStart = mid; rangeStart > 0; --rangeStart)
		{
			uint32 index = compressIndexReferences[rangeStart - 1];
			if (!IsEquivalent(insertedList[3 * index], toInsert.x)) break;
		}
		for (rangeEnd = min(mid + 1, compressIndexReferences.size()); rangeEnd < compressIndexReferences.size(); ++rangeEnd)
		{
			uint32 index = compressIndexReferences[rangeEnd];
			if (!IsEquivalent(insertedList[3 * index], toInsert.x)) break;
		}
		FUAssert(rangeStart >= 0 && (rangeStart < rangeEnd || (rangeStart == rangeEnd && rangeEnd == compressIndexReferences.size())), return 0);

		// Look for an equivalent vector within the tolerable range
		for (size_t g = rangeStart; g < rangeEnd; ++g)
		{
			uint32 index = compressIndexReferences[g];
			if (IsEquivalent(toInsert, *(const FMVector3*) &insertedList[3 * index])) return index;
		}

		// Insert this new vector in the list and add the index reference at the correct position.
		uint32 compressIndex = (uint32) (insertedList.size() / 3);
		compressIndexReferences.insert(compressIndexReferences.begin() + mid, compressIndex);
		insertedList.push_back(toInsert.x);
		insertedList.push_back(toInsert.y);
		insertedList.push_back(toInsert.z);
		return compressIndex;
	}

	struct TangentialVertex
	{
		FMVector2 texCoord;
		FMVector3 normal;
		FMVector3 tangent;
		uint32 count;
		uint32 tangentId;
		uint32 binormalId;
	};
	typedef fm::vector<TangentialVertex> TangentialVertexList;

	// Generates the texture tangents and binormals for a given source of texture coordinates.
	void GenerateTextureTangentBasis(FCDGeometryMesh* mesh, FCDGeometrySource* texcoordSource, bool generateBinormals)
	{
		if (texcoordSource == NULL || mesh == NULL) return;

		// First count the positions.
		FCDGeometrySource* positionSource = mesh->FindSourceByType(FUDaeGeometryInput::POSITION);
		if (positionSource == NULL) return;
		size_t globalVertexCount = positionSource->GetValueCount();

		// Allocate the tangential vertices.
		// This temporary buffer is necessary to ensure we have smooth tangents/binormals.
		TangentialVertexList* globalVertices = new TangentialVertexList[globalVertexCount];
		memset(globalVertices, 0, sizeof(TangentialVertexList) * globalVertexCount);

		// This operation is done on the tessellation: fill in the list of tangential vertices.
		size_t polygonsCount = mesh->GetPolygonsCount();
		for (size_t i = 0; i < polygonsCount; ++i)
		{
			FCDGeometryPolygons* polygons = mesh->GetPolygons(i);

			// Verify that this polygons set uses the given texture coordinate source.
			FCDGeometryPolygonsInput* texcoordInput = polygons->FindInput(texcoordSource);
			if (texcoordInput == NULL) continue;

			// Retrieve the data and index buffer of positions/normals/texcoords for this polygons set.
			FCDGeometryPolygonsInput* positionInput = polygons->FindInput(FUDaeGeometryInput::POSITION);
			FCDGeometryPolygonsInput* normalsInput = polygons->FindInput(FUDaeGeometryInput::NORMAL);
			if (positionInput == NULL || normalsInput == NULL) continue;
			FCDGeometrySource* positionSource = positionInput->GetSource();
			FCDGeometrySource* normalsSource = normalsInput->GetSource();
			FCDGeometrySource* texcoordSource = texcoordInput->GetSource();
			if (positionSource == NULL || normalsSource == NULL || texcoordSource == NULL) continue;
			uint32 positionStride = positionSource->GetStride();
			uint32 normalsStride = normalsSource->GetStride();
			uint32 texcoordStride = texcoordSource->GetStride();
			if (positionStride < 3 || normalsStride < 3 || texcoordStride < 2) continue;
			uint32* positionIndices = positionInput->GetIndices();
			uint32* normalsIndices = normalsInput->GetIndices();
			uint32* texcoordIndices = texcoordInput->GetIndices();
			size_t indexCount = positionInput->GetIndexCount();
			if (positionIndices == NULL || normalsIndices == NULL || texcoordIndices == NULL) continue;
			if (indexCount == 0 || indexCount != normalsInput->GetIndexCount() || indexCount != texcoordInput->GetIndexCount()) continue;
			float* positionData = positionSource->GetData();
			float* normalsData = normalsSource->GetData();
			float* texcoordData = texcoordSource->GetData();
			size_t positionDataLength = positionSource->GetDataCount();
			size_t normalsDataLength = normalsSource->GetDataCount();
			size_t texcoordDataLength = texcoordSource->GetDataCount();
			if (positionDataLength == 0 || normalsDataLength == 0 || texcoordDataLength == 0) continue;

			// Iterate of the faces of the polygons set. This includes holes.
			size_t faceCount = polygons->GetFaceVertexCountCount();
			size_t faceVertexOffset = 0;
			for (size_t faceIndex = 0; faceIndex < faceCount; ++faceIndex)
			{
				size_t faceVertexCount = polygons->GetFaceVertexCounts()[faceIndex];
				for (size_t vertexIndex = 0; vertexIndex < faceVertexCount; ++vertexIndex)
				{
					// For each face-vertex pair, retrieve the current/previous/next vertex position/normal/texcoord.
					size_t previousVertexIndex = (vertexIndex > 0) ? vertexIndex - 1 : faceVertexCount - 1;
					size_t nextVertexIndex = (vertexIndex < faceVertexCount - 1) ? vertexIndex + 1 : 0;
					FMVector3& previousPosition = *(FMVector3*)&positionData[positionIndices[faceVertexOffset + previousVertexIndex] * positionStride];
					FMVector2& previousTexcoord = *(FMVector2*)&texcoordData[texcoordIndices[faceVertexOffset + previousVertexIndex] * texcoordStride];
					FMVector3& currentPosition = *(FMVector3*)&positionData[positionIndices[faceVertexOffset + vertexIndex] * positionStride];
					FMVector2& currentTexcoord = *(FMVector2*)&texcoordData[texcoordIndices[faceVertexOffset + vertexIndex] * texcoordStride];
					FMVector3& nextPosition = *(FMVector3*)&positionData[positionIndices[faceVertexOffset + nextVertexIndex] * positionStride];
					FMVector2& nextTexcoord = *(FMVector2*)&texcoordData[texcoordIndices[faceVertexOffset + nextVertexIndex] * texcoordStride];
					FMVector3& normal = *(FMVector3*)&normalsData[normalsIndices[faceVertexOffset + vertexIndex] * normalsStride];

					// The formulae to calculate the tangent-space basis vectors is taken from Maya 7.0 API documentation:
					// "Appendix A: Tangent and binormal vectors".

					// Prepare the edge vectors.
					FMVector3 previousEdge(0.0f, previousTexcoord.x - currentTexcoord.x, previousTexcoord.y - currentTexcoord.y);
					FMVector3 nextEdge(0.0f, nextTexcoord.x - currentTexcoord.x, nextTexcoord.y - currentTexcoord.y);
					FMVector3 previousDisplacement = (previousPosition - currentPosition);
					FMVector3 nextDisplacement = (nextPosition - currentPosition);
					FMVector3 tangent;

					// Calculate the X-coordinate of the tangent vector.
					previousEdge.x = previousDisplacement.x;
					nextEdge.x = nextDisplacement.x;
					FMVector3 crossEdge = nextEdge ^ previousEdge;
					if (IsEquivalent(crossEdge.x, 0.0f)) crossEdge.x = 1.0f; // degenerate
					tangent.x = crossEdge.y / crossEdge.x;

					// Calculate the Y-coordinate of the tangent vector.
					previousEdge.x = previousDisplacement.y;
					nextEdge.x = nextDisplacement.y;
					crossEdge = nextEdge ^ previousEdge;
					if (IsEquivalent(crossEdge.x, 0.0f)) crossEdge.x = 1.0f; // degenerate
					tangent.y = crossEdge.y / crossEdge.x;

					// Calculate the Z-coordinate of the tangent vector.
					previousEdge.x = previousDisplacement.z;
					nextEdge.x = nextDisplacement.z;
					crossEdge = nextEdge ^ previousEdge;
					if (IsEquivalent(crossEdge.x, 0.0f)) crossEdge.x = 1.0f; // degenerate
					tangent.z = crossEdge.y / crossEdge.x;

					// Take the normal vector at this face-vertex pair, out of the calculated tangent vector
					tangent = tangent - normal * (tangent * normal);
					tangent.NormalizeIt();

					// Add this tangent to our tangential vertex.
					FUAssert(positionIndices[faceVertexOffset + vertexIndex] < globalVertexCount, continue);
					TangentialVertexList& list = globalVertices[positionIndices[faceVertexOffset + vertexIndex]];
					size_t vertexCount = list.size();
					bool found = false;
					for (size_t v = 0; v < vertexCount; ++v)
					{
						if (IsEquivalent(list[v].normal, normal) && IsEquivalent(list[v].texCoord, currentTexcoord))
						{
							list[v].tangent += tangent;
							list[v].count++;
							found = true;
						}
					}
					if (!found)
					{
						TangentialVertex v;
						v.count = 1; v.normal = normal; v.tangent = tangent; v.texCoord = currentTexcoord;
						v.tangentId = v.binormalId = ~(uint32)0;
						list.push_back(v);
					}
				}
				faceVertexOffset += faceVertexCount;
			}
		}

		FCDGeometrySource* tangentSource = NULL;
		FCDGeometrySource* binormalSource = NULL;
		FloatList tangentData;
		FloatList binormalData;
		UInt32List tangentCompressionIndices;
		UInt32List binormalCompressionIndices;

		// Iterate over the polygons again: this time create the source/inputs for the tangents and binormals.
		for (size_t i = 0; i < polygonsCount; ++i)
		{
			FCDGeometryPolygons* polygons = mesh->GetPolygons(i);

			// Verify that this polygons set uses the given texture coordinate source.
			FCDGeometryPolygonsInput* texcoordInput = polygons->FindInput(texcoordSource);
			if (texcoordInput == NULL) continue;

			// Retrieve the data and index buffer of positions/normals/texcoords for this polygons set.
			FCDGeometryPolygonsInput* positionInput = polygons->FindInput(FUDaeGeometryInput::POSITION);
			FCDGeometryPolygonsInput* normalsInput = polygons->FindInput(FUDaeGeometryInput::NORMAL);
			if (positionInput == NULL || normalsInput == NULL) continue;
			FCDGeometrySource* normalsSource = normalsInput->GetSource();
			FCDGeometrySource* texcoordSource = texcoordInput->GetSource();
			if (normalsSource == NULL || texcoordSource == NULL) continue;
			uint32 normalsStride = normalsSource->GetStride();
			uint32 texcoordStride = texcoordSource->GetStride();
			if (normalsStride < 3 || texcoordStride < 2) continue;
			uint32* positionIndices = positionInput->GetIndices();
			uint32* normalsIndices = normalsInput->GetIndices();
			uint32* texcoordIndices = texcoordInput->GetIndices();
			size_t indexCount = positionInput->GetIndexCount();
			if (positionIndices == NULL || normalsIndices == NULL || texcoordIndices == NULL) continue;
			if (indexCount == 0 || indexCount != normalsInput->GetIndexCount() || indexCount != texcoordInput->GetIndexCount()) continue;
			float* normalsData = normalsSource->GetData();
			float* texcoordData = texcoordSource->GetData();
			size_t normalsDataLength = normalsSource->GetDataCount();
			size_t texcoordDataLength = texcoordSource->GetDataCount();
			if (normalsDataLength == 0 || texcoordDataLength == 0) continue;

			// Create the texture tangents/binormals sources
			if (tangentSource == NULL)
			{
				tangentSource = mesh->AddSource(FUDaeGeometryInput::TEXTANGENT);
				tangentSource->SetDaeId(texcoordSource->GetDaeId() + "-tangents");
				tangentData.reserve(texcoordSource->GetDataCount());
				if (generateBinormals)
				{
					binormalSource = mesh->AddSource(FUDaeGeometryInput::TEXBINORMAL);
					binormalSource->SetDaeId(texcoordSource->GetDaeId() + "-binormals");
					binormalData.reserve(tangentSource->GetDataCount());
				}
			}

			// Calculate the next available offset
			uint32 inputOffset = 0;
			size_t inputCount = polygons->GetInputCount();
			FCDGeometryPolygonsInput** inputs = polygons->GetInputs();
			for (size_t j = 0; j < inputCount; ++j)
			{
				inputOffset = max(inputOffset, inputs[j]->GetOffset());
			}

			// Create the polygons set input for both the tangents and binormals
			FCDGeometryPolygonsInput* tangentInput = polygons->AddInput(tangentSource, inputOffset + 1);
			tangentInput->SetSet(texcoordInput->GetSet());
			tangentInput->ReserveIndexCount(indexCount);
			FCDGeometryPolygonsInput* binormalInput = NULL;
			if (binormalSource != NULL)
			{
				binormalInput = polygons->AddInput(binormalSource, inputOffset + 2);
				binormalInput->SetSet(tangentInput->GetSet());
				binormalInput->ReserveIndexCount(indexCount);
			}

			// Iterate of the faces of the polygons set. This includes holes.
			size_t vertexCount = positionInput->GetIndexCount();
			size_t faceVertexOffset = 0;
			for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
			{
				// For each face-vertex pair retrieve the current vertex normal&texcoord.
				FMVector2& texcoord = *(FMVector2*)&texcoordData[texcoordIndices[faceVertexOffset + vertexIndex] * texcoordStride];
				FMVector3& normal = *(FMVector3*)&normalsData[normalsIndices[faceVertexOffset + vertexIndex] * normalsStride];

				FUAssert(positionIndices[faceVertexOffset + vertexIndex] < globalVertexCount, continue);
				TangentialVertexList& list = globalVertices[positionIndices[faceVertexOffset + vertexIndex]];
				size_t vertexCount = list.size();
				for (size_t v = 0; v < vertexCount; ++v)
				{
					if (IsEquivalent(list[v].normal, normal) && IsEquivalent(list[v].texCoord, texcoord))
					{
						if (list[v].tangentId == ~(uint32)0)
						{
							list[v].tangent /= (float) list[v].count; // Average the tangent.
							list[v].tangent.Normalize();
							list[v].tangentId = CompressSortedVector(list[v].tangent, tangentData, tangentCompressionIndices);
						}
						tangentInput->AddIndex(list[v].tangentId);

						if (binormalInput != NULL)
						{
							if (list[v].binormalId == ~(uint32)0)
							{
								// Calculate and store the binormal.
								FMVector3 binormal = (list[v].normal ^ list[v].tangent).Normalize();
								uint32 compressedIndex = CompressSortedVector(binormal, binormalData, binormalCompressionIndices);
								list[v].binormalId = compressedIndex;
							}
							binormalInput->AddIndex(list[v].binormalId);
						}
					}
				}
			}
		}

		if (tangentSource != NULL) tangentSource->SetData(tangentData, 3);
		if (binormalSource != NULL) binormalSource->SetData(binormalData, 3);
	}

	struct HashIndexMapItem { UInt32List allValues; UInt32List newIndex; };
	typedef fm::vector<UInt32List> UInt32ListList;
	typedef fm::pvector<FCDGeometryPolygonsInput> InputList;
	typedef fm::map<uint32, HashIndexMapItem> HashIndexMap;

	void GenerateUniqueIndices(FCDGeometryMesh* mesh, FCDGeometryPolygons* polygonsToProcess, FCDGeometryIndexTranslationMap* translationMap)
	{
		// Prepare a list of unique index buffers.
		size_t polygonsCount = mesh->GetPolygonsCount();
		if (polygonsCount == 0) return;
		UInt32ListList indexBuffers; indexBuffers.resize(polygonsCount);
		size_t totalVertexCount = 0;

		// Fill in the index buffers for each polygons set.
		for (size_t p = 0; p < polygonsCount; ++p)
		{
			UInt32List& indexBuffer = indexBuffers[p];
			FCDGeometryPolygons* polygons = mesh->GetPolygons(p);
			// DO NOT -EVER- TOUCH MY INDICES - (Says Psuedo-FCDGeometryPoints)
			// Way to much code assumes (and carefully guards) the existing sorted structure
			if (polygons->GetPrimitiveType() == FCDGeometryPolygons::POINTS) return;
			if (polygonsToProcess != NULL && polygons != polygonsToProcess) continue;

			// Find all the indices list to determine the hash size.
			InputList idxOwners;
			size_t inputCount = polygons->GetInputCount();
			FCDGeometryPolygonsInput** inputs = polygons->GetInputs();
			for (size_t i = 0; i < inputCount; ++i)
			{
				if (inputs[i]->OwnsIndices())
				{
					// Drop index lists with the wrong number of values and avoid repeats
					FUAssert(idxOwners.empty() || idxOwners.front()->GetIndexCount() == inputs[i]->GetIndexCount(), continue);
					if (idxOwners.find(inputs[i]) == idxOwners.end())
					{
						idxOwners.push_back(inputs[i]);
					}
				}
			}
			size_t listCount = idxOwners.size();
			if (listCount == 0) continue; // no inputs?

			// Set-up a simple hashing function.
			UInt32List hashingFunction;
			uint32 hashSize = (uint32) listCount;
			hashingFunction.reserve(hashSize);
			for (uint32 h = 0; h < hashSize; ++h) hashingFunction.push_back(32 * h / hashSize);

			// Iterate over the index lists, hashing/merging the indices.
			HashIndexMap hashMap;
			size_t originalIndexCount = idxOwners.front()->GetIndexCount();
			indexBuffer.reserve(originalIndexCount);
			for (size_t i = 0; i < originalIndexCount; ++i)
			{
				// Generate the hash value for this vertex-face pair.
				uint32 hashValue = 0;
				for (size_t l = 0; l < listCount; ++l)
				{
					hashValue ^= (idxOwners[l]->GetIndices()[i]) << hashingFunction[l];
				}

				// Look for this value in the already-collected ones.
				HashIndexMap::iterator it = hashMap.find(hashValue);
				HashIndexMapItem* hashItem;
				uint32 newIndex = (uint32) totalVertexCount;
				if (it != hashMap.end())
				{
					hashItem = &((*it).second);
					size_t repeatCount = hashItem->allValues.size() / listCount;
					for (size_t r = 0; r < repeatCount && newIndex == totalVertexCount; ++r)
					{
						size_t l;
						for (l = 0; l < listCount; ++l)
						{
							if (idxOwners[l]->GetIndices()[i] != hashItem->allValues[r * listCount + l]) break;
						}
						if (l == listCount)
						{
							// We have a match: re-use this index.
							newIndex = hashItem->newIndex[r];
						}
					}
				}
				else
				{
					HashIndexMap::iterator k = hashMap.insert(hashValue, HashIndexMapItem());
					hashItem = &k->second;
				}

				if (newIndex == totalVertexCount)
				{
					// Append this new value/index to the hash map item and to the index buffer.
					for (size_t l = 0; l < listCount; ++l)
					{
						hashItem->allValues.push_back(idxOwners[l]->GetIndices()[i]);
					}
					hashItem->newIndex.push_back(newIndex);
					totalVertexCount++;
				}
				indexBuffer.push_back(newIndex);
			}
		}

		// De-reference the source data so that all the vertex data match the new indices.
		size_t meshSourceCount = mesh->GetSourceCount();
		for (size_t d = 0; d < meshSourceCount; ++d)
		{
			FCDGeometrySource* oldSource = mesh->GetSource(d);
			uint32 stride = oldSource->GetStride();
			const float* oldVertexData = oldSource->GetData();
			bool isPositionSource = oldSource->GetType() == FUDaeGeometryInput::POSITION && translationMap != NULL;
			FloatList vertexBuffer;
			vertexBuffer.resize(stride * totalVertexCount, 0.0f);

			// When processing just one polygons set, duplicate the source
			// so that the other polygons set can correctly point to the original source.
			FCDGeometrySource* newSource = (polygonsToProcess != NULL) ? mesh->AddSource(oldSource->GetType()) : oldSource;

			FCDAnimatedList newAnimatedList;
			newAnimatedList.clear();
			for (size_t p = 0; p < polygonsCount; ++p)
			{
				const UInt32List& indexBuffer = indexBuffers[p];
				FCDGeometryPolygons* polygons = mesh->GetPolygons(p);
				if (polygonsToProcess != NULL && polygonsToProcess != polygons) continue;
				FCDGeometryPolygonsInput* oldInput = polygons->FindInput(oldSource);
				if (oldInput == NULL) continue;

				// Retrieve the old list of indices and de-reference the data values.
				uint32* oldIndexList = oldInput->GetIndices();
				size_t oldIndexCount = oldInput->GetIndexCount();
				if (oldIndexList == NULL || oldIndexCount == 0) continue;

				size_t indexCount = min(oldIndexCount, indexBuffer.size());
				for (size_t i = 0; i < indexCount; ++i)
				{
					uint32 newIndex = indexBuffer[i];
					uint32 oldIndex = oldIndexList[i];

					FCDAnimatedList& animatedValues = 
							oldSource->GetAnimatedValues();
					FCDAnimated* oldAnimated = NULL;
					for (size_t j = 0; j < animatedValues.size(); j++)
					{
						FCDAnimated* animated = animatedValues[j];
						if (animated->GetValue(0) == 
								&(oldVertexData[stride * oldIndex]))
						{
							oldAnimated = animated;
							break;
						}
					}
					if (oldAnimated != NULL)
					{
						FCDAnimated* newAnimated = 
								oldAnimated->Clone(oldAnimated->GetDocument());
						newAnimated->SetArrayElement(newIndex);
						newAnimatedList.push_back(newAnimated);
					}

					// [GLaforte - 12-10-2006] Potential performance optimization: this may copy the same data over itself many times.
					for (uint32 s = 0; s < stride; ++s)
					{
						vertexBuffer[stride * newIndex + s] = oldVertexData[stride * oldIndex + s];

						// Add this value to the vertex position translation map.
						if (isPositionSource)
						{
							FCDGeometryIndexTranslationMap::iterator itU = translationMap->find(oldIndex);
							if (itU == translationMap->end()) { itU = translationMap->insert(oldIndex, UInt32List()); }
							UInt32List::iterator itF = itU->second.find(newIndex);
							if (itF == itU->second.end()) itU->second.push_back(newIndex);
						}
					}
				}

				if (polygonsToProcess != NULL)
				{
					// Change the relevant input, if it exists, to point towards the new source.
					uint32 set = oldInput->GetSet();
					SAFE_RELEASE(oldInput);
					FCDGeometryPolygonsInput* newInput = polygons->AddInput(newSource, 0);
					newInput->SetSet(set);
				}
			}

			// Set the compiled data in the source.
			newSource->SetData(vertexBuffer, stride);
			FCDAnimatedList& animatedList = newSource->GetAnimatedValues();
			animatedList.clear();
			for(FCDAnimatedList::iterator it = newAnimatedList.begin();
					it != newAnimatedList.end(); it++)
			{
				animatedList.push_back(*it);
			}
		}

		// find sources with non default Set since they cannot be per-vertex
		FUObjectContainer<FCDGeometrySource> setSources;
		for (size_t p = 0; p < polygonsCount; ++p)
		{
			FCDGeometryPolygons* polygons = mesh->GetPolygons(p);
			if (polygonsToProcess != NULL && polygons != polygonsToProcess) continue;
			size_t inputCount = polygons->GetInputCount();
			for (size_t inputIndex = 0; inputIndex < inputCount; inputIndex++)
			{
				FCDGeometryPolygonsInput* input = polygons->GetInput(inputIndex);
				if (input->GetSet() != -1)
				{
					FCDGeometrySource* source = input->GetSource();
					if (setSources.find(source) == setSources.end())
					{
						setSources.push_back(source);
					}
				}
			}
		}

		if (polygonsToProcess == NULL)
		{
			// Next, make all the sources per-vertex.
			size_t _sourceCount = mesh->GetSourceCount();
			for (size_t s = 0; s < _sourceCount; ++s)
			{
				FCDGeometrySource* it = mesh->GetSource(s);
				if (!mesh->IsVertexSource(it) && (setSources.find(it) == setSources.end()))
				{
					mesh->AddVertexSource(it);
				}
			}
		}

		while (!setSources.empty())
		{
			setSources.pop_back();
		}

		// Enforce the index buffers.
		for (size_t p = 0; p < polygonsCount; ++p)
		{
			const UInt32List& indexBuffer = indexBuffers[p];
			FCDGeometryPolygons* polygons = mesh->GetPolygons(p);
			if (polygonsToProcess != NULL && polygons != polygonsToProcess) continue;

			size_t inputCount = polygons->GetInputCount();
			for (size_t i = 0; i < inputCount; i++)
			{
				FCDGeometryPolygonsInput* anyInput = polygons->GetInput(i);
				if (anyInput->GetSource()->GetDataCount() == 0) continue;

				anyInput->SetIndices(&indexBuffer.front(), indexBuffer.size());
			}
		}
	}

	// Splits the mesh's polygons sets to ensure that none of them have
	// more than a given number of indices within their index buffers.
	void FitIndexBuffers(FCDGeometryMesh* mesh, size_t maximumIndexCount)
	{
		// Iterate over the original polygons sets, looking for ones that are too big.
		// When a polygon set is too big, a new polygon set will be added, but at the end of the list.
		size_t originalPolygonCount = mesh->GetPolygonsCount();
		for (size_t i = 0; i < originalPolygonCount; ++i)
		{
			// Iterate a over the face-vertex counts of the polygons set in order
			// to find at which face to break this polygons set.
			FCDGeometryPolygons* polygons = mesh->GetPolygons(i);
			if (polygons->GetPrimitiveType() == FCDGeometryPolygons::POINTS) continue;

			size_t faceCount = polygons->GetFaceVertexCountCount();
			if (faceCount == 0) continue;
			UInt32List faceVertexCounts(polygons->GetFaceVertexCounts(), faceCount);
			FCDGeometryPolygonsInput** inputs = polygons->GetInputs();
			size_t inputCount = polygons->GetInputCount();

			UInt32List::iterator splitIt = faceVertexCounts.end();
			uint32 faceVertexCount = 0;
			for (splitIt = faceVertexCounts.begin(); splitIt != faceVertexCounts.end(); ++splitIt)
			{
				if (faceVertexCount + (*splitIt) > maximumIndexCount) break;
				faceVertexCount += (*splitIt);
			}
			if (splitIt == faceVertexCounts.end()) continue; // Polygons sets fit correctly.
			size_t splitIndexCount = (size_t) faceVertexCount;
			size_t splitFaceCount = splitIt - faceVertexCounts.begin();

			size_t faceCopyStart = splitFaceCount;
			size_t faceCopyEnd = faceCopyStart;
			size_t faceVertexCopyStart = splitIndexCount;
			size_t faceVertexCopyEnd = faceVertexCopyStart;
			while (faceCopyEnd < faceCount)
			{
				// Create a new polygons set and copy the basic information from the first polygons set.
				FCDGeometryPolygons* polygonsCopy = mesh->AddPolygons();
				polygonsCopy->SetMaterialSemantic(polygons->GetMaterialSemantic());

				// Figure out which faces will be moved to this polygons set.
				faceVertexCount = 0;
				for (; faceCopyEnd < faceCount; ++faceCopyEnd)
				{
					uint32 localCount = faceVertexCounts[faceCopyEnd];
					if (faceVertexCount + localCount > maximumIndexCount) break;
					faceVertexCount += localCount;
				}
				faceVertexCopyEnd += faceVertexCount;

				FUAssert(faceVertexCopyEnd > faceVertexCopyStart, continue);
				FUAssert(faceCopyEnd > faceCopyStart, continue);

				// Create the inputs and their indices over in the new polygons set.
				for (size_t j = 0; j < inputCount; ++j)
				{
					FCDGeometrySource* source = inputs[j]->GetSource();
					FCDGeometryPolygonsInput* inputCopy;
					if (!mesh->IsVertexSource(source)) inputCopy = polygonsCopy->AddInput(source, inputs[j]->GetOffset());
					else inputCopy = polygonsCopy->FindInput(source);
					FUAssert(inputCopy != NULL, continue);

					// For owners, copy the indices over.
					size_t indexCopyCount = inputCopy->GetIndexCount();
					if (indexCopyCount == 0)
					{
						uint32* indices = inputs[j]->GetIndices();
						inputCopy->SetIndices(indices + faceVertexCopyStart, faceVertexCopyEnd - faceVertexCopyStart);
					}
				}

				// Copy the face-vertex counts over to the new polygons set
				// And increment the copy counters.
				size_t faceCopyCount = faceCopyEnd - faceCopyStart;
				polygonsCopy->SetFaceVertexCountCount(faceCopyCount);
				memcpy(polygonsCopy->GetFaceVertexCounts(), &(*(faceVertexCounts.begin() + faceCopyStart)), faceCopyCount * sizeof(uint32));
				faceCopyStart = faceCopyEnd;
				faceVertexCopyStart = faceVertexCopyEnd;
			}

			// Remove the faces that were split away and their indices.
			for (size_t j = 0; j < inputCount; ++j)
			{
				if (inputs[j]->OwnsIndices())
				{
					inputs[j]->SetIndexCount(splitIndexCount);
				}
			}
			polygons->SetFaceVertexCountCount(splitFaceCount);
		}

		mesh->Recalculate();
	}

	// Reverses all the normals of a mesh.
	void ReverseNormals(FCDGeometryMesh* mesh)
	{
		size_t sourceCount = mesh->GetSourceCount();
		for (size_t i = 0; i < sourceCount; ++i)
		{
			FCDGeometrySource* source = mesh->GetSource(i);
			if (source->GetType() == FUDaeGeometryInput::NORMAL || source->GetType() == FUDaeGeometryInput::GEOTANGENT
				|| source->GetType() == FUDaeGeometryInput::GEOBINORMAL || source->GetType() == FUDaeGeometryInput::TEXTANGENT
				|| source->GetType() == FUDaeGeometryInput::TEXBINORMAL)
			{
				float* v = source->GetData();
				size_t dataCount = source->GetDataCount();
				for (size_t it = 0; it < dataCount; ++it)
				{
					*(v++) *= -1.0f;
				}
			}
		}
	}
}
