/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDControllerTools.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDMorphController.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"

namespace FCDControllerTools
{
	void ApplyTranslationMap(FCDSkinController* controller, FCDGeometryIndexTranslationMap& translationMap)
	{
		FCDSkinControllerVertex* influences = controller->GetVertexInfluences();
		size_t influenceCount = controller->GetInfluenceCount();

		// make a copy of the influences
		FCDSkinControllerVertex* copiedInfluences = new FCDSkinControllerVertex[influenceCount];
		for (size_t i = 0; i < influenceCount; i++)
		{
			copiedInfluences[i] = influences[i];
		}

		/* Failing on this assertion is probably because the translationMap was
		   not created by GenerateUniqueIndices for the target of the given
		   controller. It can also be caused by calling this method multiple
		   times for the same translationMap. */
		FUAssert(translationMap.size() <= influenceCount, return;);
		FUAssert(influenceCount > 0, return;);

		// find the largest index
		uint32 largest = 0;
		for (size_t i = 0; i < influenceCount; i++)
		{
			UInt32List& curList = translationMap[(uint32)i];
			for (UInt32List::iterator it = curList.begin(); 
					it != curList.end(); it++)
			{
				largest = max(largest, *it);
			}
		}

		// Set the new influences
		controller->SetInfluenceCount(largest + 1);

//#define FCDCONTROLLERTOOLS_DEBUG
#ifdef FCDCONTROLLERTOOLS_DEBUG
		for (size_t i = 0; i < influenceCount; ++i)
		{
			// Reset all the influences.
			FCDSkinControllerVertex* vertex = controller->GetVertexInfluence(i);
			vertex->SetPairCount(0);
		}
#endif // FCDCONTROLLERTOOLS_DEBUG
		
		for (size_t i = 0; i < influenceCount; i++)
		{
			FCDSkinControllerVertex& newInfluence = copiedInfluences[i];
			size_t pairCount = newInfluence.GetPairCount();

			UInt32List& curList = translationMap[(uint32)i];
			for (UInt32List::iterator it = curList.begin(); it != curList.end(); it++)
			{
				FCDSkinControllerVertex* vertex = controller->GetVertexInfluence(*it);
				vertex->SetPairCount(pairCount);
				for (size_t j = 0; j < pairCount; j++)
				{
					FCDJointWeightPair* pair = newInfluence.GetPair(j);
					FCDJointWeightPair* p = vertex->GetPair(j);
					p->jointIndex = pair->jointIndex;
					p->weight = pair->weight;
				}
			}
		}

#ifdef FCDCONTROLLERTOOLS_DEBUG
		for (size_t i = 0; i < influenceCount; ++i)
		{
			// Reset all the influences.
			FCDSkinControllerVertex* vertex = controller->GetVertexInfluence(i);
			FUAssert(vertex->GetPairCount() > 0, continue);
			size_t pairCount = vertex->GetPairCount();
			float total = 0.0f;
			for (size_t j = 0; j < pairCount; ++j) total += vertex->GetPair(j)->weight;
			FUAssert(IsEquivalent(total, 1.0f), continue);
		}
#endif // FCDCONTROLLERTOOLS_DEBUG

		SAFE_DELETE_ARRAY(copiedInfluences);
	}

	void FCOLLADA_EXPORT CloneMorphControllers(FCDocument* document)
	{
		FCDControllerLibrary* controllerLibrary = document->GetControllerLibrary();
		FCDGeometryLibrary* geometryLibrary = document->GetGeometryLibrary();

		size_t entityCount = controllerLibrary->GetEntityCount();
		for (size_t i = 0; i < entityCount; i++)
		{
			FCDController* controller = controllerLibrary->GetEntity(i);
			if (!controller->IsMorph()) continue;

			FCDGeometry* baseGeom = controller->GetBaseGeometry();
			if (baseGeom == NULL || !baseGeom->IsMesh()) continue;
			FCDGeometryMesh* baseGeomMesh = baseGeom->GetMesh();
			
			// clone the targets and take tessellation info from base
			FCDMorphTargetContainer& targets = 
					controller->GetMorphController()->GetTargets();
			for (FCDMorphTargetContainer::iterator targetsIt = targets.begin();
					targetsIt != targets.end(); targetsIt++)
			{
				FCDGeometry* oldGeometry = (*targetsIt)->GetGeometry();

				FCDGeometry* geometry = geometryLibrary->AddEntity();
				oldGeometry->Clone(geometry, true);
				(*targetsIt)->SetGeometry(geometry);

				FCDGeometryMesh* geomMesh = geometry->GetMesh();

				for (size_t i = 0; i < baseGeomMesh->GetPolygonsCount(); i++)
				{
					FCDGeometryPolygons* basePolygon = 
							baseGeomMesh->GetPolygons(i);

					// add the corresponding polygon if one does not exits
					FCDGeometryPolygons* newPolygon;
					if (i < geomMesh->GetPolygonsCount())
					{
						newPolygon = geomMesh->GetPolygons(i);
					}
					else
					{
						newPolygon = geomMesh->AddPolygons();
					}

					newPolygon->SetMaterialSemantic(
							basePolygon->GetMaterialSemantic());

					// make sure base and targets have same inputs and indices
					size_t newInputCount = newPolygon->GetInputCount();
					size_t baseInputCount = basePolygon->GetInputCount();
					for (size_t j = 0; j < baseInputCount; j++)
					{
						FCDGeometryPolygonsInput* baseInput = 
								basePolygon->GetInput(j);
						FCDGeometryPolygonsInput* newInput =
								newPolygon->FindInput(
										baseInput->GetSemantic());
						if (newInput == NULL)
						{
							newInput = newPolygon->AddInput(
									baseInput->GetSource(), 
									(uint32)newInputCount);
							newInputCount++;
						}
						newInput->SetIndices(baseInput->GetIndices(), 
								baseInput->GetIndexCount());
					}

					newPolygon->SetFaceVertexCountCount(0);
					uint32* vertexFaceCounts = 
							basePolygon->GetFaceVertexCounts();
					size_t vertexFaceCountsCount = 
							basePolygon->GetFaceVertexCountCount();
					for (size_t j = 0; j < vertexFaceCountsCount; j++)
					{
						newPolygon->AddFaceVertexCount(vertexFaceCounts[j]);
					}

					newPolygon->SetFaceOffset(basePolygon->GetFaceOffset());
					newPolygon->SetHoleOffset(basePolygon->GetHoleOffset());
					newPolygon->Recalculate();
				}						
				geomMesh->Recalculate();
			}
		}
	}
}

