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
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FCDocument/FCDMorphController.h"
#include "FCDocument/FCDSceneNode.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDMorphController
//

ImplementObjectType(FCDMorphController);

FCDMorphController::FCDMorphController(FCDocument* document, FCDController* _parent)
:	FCDObject(document), parent(_parent)
{
}

FCDMorphController::~FCDMorphController()
{
	parent = NULL;
}

// Changes the base target of the morpher
void FCDMorphController::SetBaseTarget(FCDEntity* entity)
{
	baseTarget = NULL;

	// Retrieve the actual base entity, as you can chain controllers.
	FCDEntity* baseEntity = entity;
	if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::CONTROLLER)
	{
		baseEntity = ((FCDController*) baseEntity)->GetBaseGeometry();
	}
	if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::GEOMETRY)
	{
		baseTarget = entity;

		// Remove the old morph targets which are not similar, anymore, to the new base entity.
		for (size_t i = 0; i < morphTargets.size();)
		{
			if (IsSimilar(morphTargets[i]->GetGeometry()))
			{
				++i;
			}
			else
			{
				morphTargets.release(morphTargets[i]);
			}
		}
	}
	else
	{
		// The new base target is not valid.
		morphTargets.clear();
	}

	SetDirtyFlag();
}

// Adds a new morph target.
FCDMorphTarget* FCDMorphController::AddTarget(FCDGeometry* geometry, float weight)
{
	FCDMorphTarget* target = NULL;
	// It is legal to add targets with out a base geometry
	if (baseTarget == NULL || IsSimilar(geometry))
	{
		target = morphTargets.Add(GetDocument(), this);
		target->SetGeometry(geometry);
		target->SetWeight(weight);
	}
	SetDirtyFlag();
	return target;
}

// Retrieves whether a given entity is similar to the base target.
bool FCDMorphController::IsSimilar(FCDEntity* entity)
{
	bool similar = false;
	if (entity != NULL && baseTarget != NULL)
	{
		size_t vertexCount = 0;
		bool isMesh = false;
		bool isSpline = false;

		// Find the number of vertices in the base target
		FCDEntity* baseEntity = baseTarget;
		if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::CONTROLLER)
		{
			baseEntity = ((FCDController*) baseEntity)->GetBaseGeometry();
		}
		if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::GEOMETRY)
		{
			FCDGeometry* g = (FCDGeometry*) baseEntity;
			if (g->IsMesh())
			{
				isMesh = true;
				FCDGeometryMesh* m = g->GetMesh();
				FCDGeometrySource* positions = m->GetPositionSource();
				if (positions != NULL)
				{
					vertexCount = positions->GetValueCount();
				}
			}

			if (g->IsSpline())
			{
				isSpline = true;
				FCDGeometrySpline* s = g->GetSpline();
				vertexCount = s->GetTotalCVCount();
			}
		}


		// Find the number of vertices in the given entity
		baseEntity = entity;
		if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::CONTROLLER)
		{
			baseEntity = ((FCDController*) baseEntity)->GetBaseGeometry();
		}
		if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::GEOMETRY)
		{
			FCDGeometry* g = (FCDGeometry*) baseEntity;
			if (g->IsMesh() && isMesh)
			{
				FCDGeometryMesh* m = g->GetMesh();
				FCDGeometrySource* positions = m->GetPositionSource();
				if (positions != NULL)
				{
					similar = (vertexCount == positions->GetValueCount());
				}
			}

			if (g->IsSpline() && isSpline)
			{
				FCDGeometrySpline* s = g->GetSpline();
				similar = (vertexCount == s->GetTotalCVCount());
			}
		}
	}

	return similar;
}

// Load this controller from a Collada <controller> node
bool FCDMorphController::LoadFromXML(xmlNode* morphNode)
{
	bool status = true;
	if (!IsEquivalent(morphNode->name, DAE_CONTROLLER_MORPH_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_CONTROlLER_LIB_NODE, morphNode->line);
		return status;
	}

	// Parse in the morph method
	fm::string methodValue = ReadNodeProperty(morphNode, DAE_METHOD_ATTRIBUTE);
	method = FUDaeMorphMethod::FromString(methodValue);
	if (method == FUDaeMorphMethod::UNKNOWN)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_MC_PROC_METHOD, morphNode->line);
	}

	// Find the base geometry, this is linked after load.
	targetId = ReadNodeSource(morphNode);

	// Find the <targets> element and process its inputs
	xmlNode* targetsNode = FindChildByType(morphNode, DAE_TARGETS_ELEMENT);
	if (targetsNode == NULL)
	{
		//return status.Fail(FS("Cannot find necessary <targets> element for morph controller: ") + TO_FSTRING(parent->GetDaeId()), morphNode->line);
		FUError::Error(FUError::ERROR, FUError::ERROR_MISSING_ELEMENT, morphNode->line);

	}
	xmlNodeList inputNodes;
	FindChildrenByType(targetsNode, DAE_INPUT_ELEMENT, inputNodes);

	// Find the TARGET and WEIGHT input necessary sources
	xmlNode* targetSourceNode = NULL,* weightSourceNode = NULL;
	for (xmlNodeList::iterator it = inputNodes.begin(); it != inputNodes.end(); ++it)
	{
		xmlNode* inputNode = (*it);
		fm::string semantic = ReadNodeSemantic(inputNode);
		fm::string sourceId = ReadNodeSource(inputNode);
		if (semantic == DAE_WEIGHT_MORPH_INPUT || semantic == DAE_WEIGHT_MORPH_INPUT_DEPRECATED)
		{
			weightSourceNode = FindChildById(morphNode, sourceId);
		}
		else if (semantic == DAE_TARGET_MORPH_INPUT || semantic == DAE_TARGET_MORPH_INPUT_DEPRECATED)
		{
			targetSourceNode = FindChildById(morphNode, sourceId);
		}
		else
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_MORPH_TARGET_TYPE, inputNode->line);
		}
	}
	if (targetSourceNode == NULL || weightSourceNode == NULL)
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_MISSING_INPUT, targetsNode->line);
		return status;
	}

	// Read in the sources
	StringList morphTargetIds;
	ReadSource(targetSourceNode, morphTargetIds);
	FloatList weights;
	ReadSource(weightSourceNode, weights);
	size_t targetCount = morphTargetIds.size();
	if (weights.size() != targetCount)
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_SOURCE_SIZE, targetSourceNode->line);
	}

	// Find the target geometries and build the morph targets
	morphTargets.reserve(targetCount);
	for (int32 i = 0; i < (int32) targetCount; ++i)
	{
		FCDGeometry* targetGeometry = GetDocument()->FindGeometry(morphTargetIds[i]);
		if (targetGeometry == NULL)
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_TARGET_GEOMETRY_MISSING, morphNode->line);
		}
		FCDMorphTarget* morphTarget = AddTarget(targetGeometry, weights[i]);

		// Record the morphing weight as animatable
		FCDAnimatedFloat::Create(GetDocument(), weightSourceNode, &morphTarget->GetWeight(), i);
	}

	SetDirtyFlag();
	return status;
}

// Write out this controller to a COLLADA XML node tree
xmlNode* FCDMorphController::WriteToXML(xmlNode* parentNode) const
{
	size_t targetCount = GetTargetCount();

	// Create the <morph> node and set its attributes
	xmlNode* morphNode = AddChild(parentNode, DAE_CONTROLLER_MORPH_ELEMENT);
	AddAttribute(morphNode, DAE_METHOD_ATTRIBUTE, FUDaeMorphMethod::ToString(method));
	if (baseTarget != NULL)
	{
		AddAttribute(morphNode, DAE_SOURCE_ATTRIBUTE, fm::string("#") + baseTarget->GetDaeId());
	}

	// Gather up the morph target ids and the morphing weights
	StringList targetIds; targetIds.reserve(targetCount);
	FloatList weights; weights.reserve(targetCount);
	for (FCDMorphTargetContainer::const_iterator it = morphTargets.begin(); it != morphTargets.end(); ++it)
	{
		const FCDMorphTarget* t = (*it);
		targetIds.push_back(t->GetGeometry() != NULL ? t->GetGeometry()->GetDaeId() : DAEERR_UNKNOWN_IDREF);
		weights.push_back(t->GetWeight());
	}

	// Export the target id source
	FUSStringBuilder targetSourceId(parent->GetDaeId()); targetSourceId.append("-targets");
	AddSourceIDRef(morphNode, targetSourceId.ToCharPtr(), targetIds, DAE_TARGET_MORPH_INPUT);

	// Export the weight source
	FUSStringBuilder weightSourceId(parent->GetDaeId()); weightSourceId.append("-morph_weights");
	xmlNode* weightSourceNode = AddSourceFloat(morphNode, weightSourceId.ToCharPtr(), weights, DAE_WEIGHT_MORPH_INPUT);

	// Export the <targets> elements
	xmlNode* targetsNode = AddChild(morphNode, DAE_TARGETS_ELEMENT);
	AddInput(targetsNode, targetSourceId.ToCharPtr(), DAE_TARGET_MORPH_INPUT);
	AddInput(targetsNode, weightSourceId.ToCharPtr(), DAE_WEIGHT_MORPH_INPUT);

	// Record the morphing weight animations
	for (int32 i = 0; i < (int32) targetCount; ++i)
	{
		const FCDMorphTarget* t = morphTargets[i];
		GetDocument()->WriteAnimatedValueToXML(&t->GetWeight(), weightSourceNode, "morphing_weights", i);
	}

	return morphNode;
}

bool FCDMorphController::LinkImport()
{
	if(GetBaseTarget() == NULL)
	{
		FCDEntity* baseTarget = GetDocument()->FindGeometry(targetId);
		if (baseTarget == NULL) baseTarget = GetDocument()->FindController(targetId);
		if (baseTarget == NULL)
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_MC_BASE_TARGET_MISSING, 0);
			return false;
		}

		SetBaseTarget(baseTarget);

		targetId.clear();
	}
	return true;
}

//
// FCDMorphTarget
//

ImplementObjectType(FCDMorphTarget);

FCDMorphTarget::FCDMorphTarget(FCDocument* document, FCDMorphController* _parent)
:	FCDObject(document), parent(_parent)
,	weight(0.0f)
{
}

FCDMorphTarget::~FCDMorphTarget()
{
	parent = NULL;
	weight = 0.0f;
}

void FCDMorphTarget::SetGeometry(FCDGeometry* _geometry)
{
	// Let go of the old geometry
	geometry = NULL;

	// Check if this geometry is similar to the controller base target
	if (GetParent()->GetBaseTarget() == NULL || GetParent()->IsSimilar(_geometry))
	{
		geometry = _geometry;
	}
	SetDirtyFlag();
}

FCDAnimated* FCDMorphTarget::GetAnimatedWeight()
{
	return GetDocument()->FindAnimatedValue(&weight);
}
const FCDAnimated* FCDMorphTarget::GetAnimatedWeight() const
{
	return GetDocument()->FindAnimatedValue(&weight);
}

bool FCDMorphTarget::IsAnimated() const
{
	return GetDocument()->IsValueAnimated(&weight);
}
