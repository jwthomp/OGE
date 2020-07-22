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
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSkinController.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDSkinController
//

ImplementObjectType(FCDSkinController);

FCDSkinController::FCDSkinController(FCDocument* document, FCDController* _parent)
:	FCDObject(document), parent(_parent)
,	bindShapeTransform(FMMatrix44::Identity)
{
}

FCDSkinController::~FCDSkinController()
{
	parent = NULL;
}

void FCDSkinController::SetTarget(FCDEntity* _target)
{
	target = NULL;
	influences.clear();
	SetDirtyFlag();

	// Retrieve the actual base entity, as you can chain controllers.
	FCDEntity* baseEntity = _target;
	if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::CONTROLLER)
	{
		baseEntity = ((FCDController*) baseEntity)->GetBaseGeometry();
	}

	if (baseEntity == NULL || baseEntity->GetType() != FCDEntity::GEOMETRY)
	{
		// The new target is no good!
		return;
	}

	target = _target;
	FCDGeometry* geometry = (FCDGeometry*) baseEntity;

	// Retrieve the new vertex count
	size_t vertexCount = 0;
	if (geometry->IsMesh())
	{
		FCDGeometryMesh* mesh = geometry->GetMesh();
		FCDGeometrySource* positionSource = mesh->GetPositionSource();
		if (positionSource != NULL)
		{
			vertexCount = positionSource->GetValueCount();
		}
	}
	else if (geometry->IsSpline())
	{
		FCDGeometrySpline* spline = geometry->GetSpline();
		vertexCount = spline->GetTotalCVCount();
	}

	// Modify the list of influences to match the new target's vertex count.
	SetInfluenceCount(vertexCount);
}

void FCDSkinController::SetInfluenceCount(size_t count)
{
	// None of the list structures are allocated directly: resize() will work fine.
	influences.resize(count);
	SetDirtyFlag();
}

void FCDSkinController::SetJointCount(size_t count)
{
	joints.resize(count);
	SetDirtyFlag();
}

FCDSkinControllerJoint* FCDSkinController::AddJoint(const fm::string jSubId, const FMMatrix44& bindPose)
{
	SetJointCount(GetJointCount() + 1);
	FCDSkinControllerJoint* joint = &joints.back();
	joint->SetId(jSubId);
	joint->SetBindPoseInverse(bindPose);
	SetDirtyFlag();
	return joint;
}


// Reduce the number of joints influencing each vertex to a maximum count
void FCDSkinController::ReduceInfluences(uint32 maxInfluenceCount, float minimumWeight)
{
	// Pre-cache an empty weight list to the reduced count
	fm::vector<FCDJointWeightPair> reducedWeights;
	reducedWeights.reserve(maxInfluenceCount + 1);

	for (FCDSkinControllerVertex* itM = influences.begin(); itM != influences.end(); ++itM)
	{
		FCDSkinControllerVertex& influence = (*itM);
		size_t oldInfluenceCount = influence.GetPairCount();

		// Reduce the weights, keeping only the more important ones using a sorting algorithm.
		// Also, calculate the current total of the weights, to re-normalize the reduced weights
		float oldTotal = 0.0f;
		reducedWeights.clear();
		for (size_t i = 0; i < oldInfluenceCount; ++i)
		{
			FCDJointWeightPair* pair = influence.GetPair(i);
			if (pair->weight >= minimumWeight)
			{
				FCDJointWeightPair* itRW = reducedWeights.begin();
				for (; itRW != reducedWeights.end() && (*itRW).weight > pair->weight; ++itRW) {}
				if (itRW != reducedWeights.end() || reducedWeights.size() <= maxInfluenceCount)
				{
					reducedWeights.insert(itRW, *pair);
					if (reducedWeights.size() > maxInfluenceCount) reducedWeights.pop_back();
				}
			}
			oldTotal += pair->weight;
		}

		size_t newInfluenceCount = reducedWeights.size();
		if (oldInfluenceCount > newInfluenceCount)
		{
			// Replace the old weights and re-normalize to their old total
			influence.SetPairCount(newInfluenceCount);
			for (size_t i = 0; i < newInfluenceCount; ++i) (*(influence.GetPair(i))) = reducedWeights[i];

			float newTotal = 0.0f;
			for (size_t i = 0; i < newInfluenceCount; ++i) newTotal += influence.GetPair(i)->weight;
			float renormalizingFactor = oldTotal / newTotal;
			for (size_t i = 0; i < newInfluenceCount; ++i) influence.GetPair(i)->weight *= renormalizingFactor;
		}
	}

	SetDirtyFlag();
}

// Load this controller from a COLLADA <controller> node
bool FCDSkinController::LoadFromXML(xmlNode* skinNode)
{
	bool status = true;
	if (!IsEquivalent(skinNode->name, DAE_CONTROLLER_SKIN_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_CONTROlLER_LIB_NODE, skinNode->line);
		return status;
	}

	// Read in the <bind_shape_matrix> element
	xmlNode* bindShapeTransformNode = FindChildByType(skinNode, DAE_BINDSHAPEMX_SKIN_PARAMETER);
	if (bindShapeTransformNode == NULL) bindShapeTransform = FMMatrix44::Identity;
	else
	{
		const char* content = ReadNodeContentDirect(bindShapeTransformNode);
		FUStringConversion::ToMatrix(&content, bindShapeTransform);
	}

	// Find the target geometry, this is linked post-load
	targetId = ReadNodeProperty(skinNode, DAE_SOURCE_ATTRIBUTE);

	// Retrieve the <joints> element and the <vertex_weights> element
	xmlNode* jointsNode = FindChildByType(skinNode, DAE_JOINTS_ELEMENT);
	xmlNode* combinerNode = FindChildByType(skinNode, DAE_WEIGHTS_ELEMENT);

	// Verify that we have the necessary data structures: bind-shape, <joints> elements, <combiner> element
	if (jointsNode == NULL || combinerNode == NULL)
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_MISSING_ELEMENT, skinNode->line);
		return status = false;
	}

	// Gather the inputs for the <joints> element and the <combiner> element
	xmlNode* firstCombinerValueNode = NULL;
	xmlNodeList skinningInputNodes;
	FindChildrenByType(jointsNode, DAE_INPUT_ELEMENT, skinningInputNodes);
	uint32 combinerValueCount = ReadNodeCount(combinerNode);
	for (xmlNode* child = combinerNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;
		if (IsEquivalent(child->name, DAE_INPUT_ELEMENT)) skinningInputNodes.push_back(child);
		else if (IsEquivalent(child->name, DAE_VERTEX_ELEMENT) || IsEquivalent(child->name, DAE_VERTEXCOUNT_ELEMENT))
		{ 
			firstCombinerValueNode = child;
			break;
		}
	}
	if (firstCombinerValueNode == NULL)
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_MISSING_ELEMENT, combinerNode->line);
	}

	// Process these inputs
	FloatList weights;
	StringList jointSubIds;
	FMMatrix44List invertedBindPoses;
	int32 jointIdx = 0, weightIdx = 1;
	for (xmlNodeList::iterator it = skinningInputNodes.begin(); it != skinningInputNodes.end(); ++it)
	{
		fm::string semantic = ReadNodeSemantic(*it);
		fm::string sourceId = ReadNodeSource(*it);
		xmlNode* sourceNode = FindChildById(skinNode, sourceId);

		if (semantic == DAE_JOINT_SKIN_INPUT)
		{
			fm::string idx = ReadNodeProperty(*it, DAE_OFFSET_ATTRIBUTE);
			if (!idx.empty()) jointIdx = FUStringConversion::ToInt32(idx);
			if (!jointSubIds.empty()) continue;
			ReadSource(sourceNode, jointSubIds);
		}
		else if (semantic == DAE_BINDMATRIX_SKIN_INPUT)
		{
			if (!invertedBindPoses.empty())
			{
				FUError::Error(FUError::ERROR, FUError::ERROR_IB_MATRIX_MISSING, (*it)->line);
			}

			// Read in the bind-pose matrices <source> element
			ReadSource(sourceNode, invertedBindPoses);
		}
		else if (semantic == DAE_WEIGHT_SKIN_INPUT)
		{
			fm::string idx = ReadNodeProperty(*it, DAE_OFFSET_ATTRIBUTE);
			if (!idx.empty()) weightIdx = FUStringConversion::ToInt32(idx);

			// Read in the weights <source> element
			ReadSource(sourceNode, weights);
		}
	}

	// Parse the <vcount> and the <v> elements
	UInt32List combinerVertexCounts; combinerVertexCounts.reserve(combinerValueCount);
	Int32List combinerVertexIndices; combinerVertexIndices.reserve(combinerValueCount * 5);

	// The <vcount> and the <v> elements are ordered. Read the <vcount> element first.
	if (!IsEquivalent(firstCombinerValueNode->name, DAE_VERTEXCOUNT_ELEMENT))
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_VCOUNT_MISSING, firstCombinerValueNode->line);
	}
	const char* content = ReadNodeContentDirect(firstCombinerValueNode);
	FUStringConversion::ToUInt32List(content, combinerVertexCounts);

	// Read the <v> element second.
	xmlNode* vNode = firstCombinerValueNode->next;
	while (vNode != NULL && vNode->type != XML_ELEMENT_NODE) vNode = vNode->next;
	if (vNode == NULL || !IsEquivalent(vNode->name, DAE_VERTEX_ELEMENT))
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_V_ELEMENT_MISSING, vNode->line);
	}
	content = ReadNodeContentDirect(vNode);
	FUStringConversion::ToInt32List(content, combinerVertexIndices);
	size_t combinerVertexIndexCount = combinerVertexIndices.size();

	// Validate the inputs
	if (jointSubIds.size() != invertedBindPoses.size())
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_JC_BPMC_NOT_EQUAL, skinNode->line);
	}
	if (combinerVertexCounts.size() != combinerValueCount)
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_INVALID_VCOUNT, skinNode->line);
	}

	// Setup the joint-weight-vertex matches
	influences.resize(combinerValueCount);
	size_t jointCount = jointSubIds.size(), weightCount = weights.size(), offset = 0;
	for (size_t j = 0; j < combinerValueCount; ++j)
	{
		FCDSkinControllerVertex& vert = influences[j];
		uint32 localValueCount = combinerVertexCounts[j];
		vert.SetPairCount(localValueCount);
		for (size_t i = 0; i < localValueCount && offset < combinerVertexIndexCount - 1; ++i)
		{
			FCDJointWeightPair* pair = vert.GetPair(i);
			pair->jointIndex = combinerVertexIndices[offset + jointIdx];
			if (pair->jointIndex < -1 || pair->jointIndex >= (int32) jointCount)
			{
				FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_JOINT_INDEX);
				pair->jointIndex = -1;
			}
			uint32 weightIndex = combinerVertexIndices[offset + weightIdx];
			if (weightIndex >= weightCount)
			{
				FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_WEIGHT_INDEX);
				weightIndex = 0;
			}
			pair->weight = weights[weightIndex];
			offset += 2;
		}
	}

	// Normalize the weights, per-vertex, to 1 (or 0)
	// This step is still being debated as necessary or not, for COLLADA 1.4.
	for (FCDSkinControllerVertex* it = influences.begin(); it != influences.end(); ++it)
	{
		FCDSkinControllerVertex& pair = (*it);
		float weightSum = 0.0f;
		for (size_t i = 0; i < pair.GetPairCount(); ++i) weightSum += pair.GetPair(i)->weight;
		if (IsEquivalent(weightSum, 0.0f) || IsEquivalent(weightSum, 1.0f)) continue;

		float invWeightSum = 1.0f / weightSum;
		for (size_t i = 0; i < pair.GetPairCount(); ++i) pair.GetPair(i)->weight *= invWeightSum;
	}

	// Setup the joints.
	joints.resize(jointCount);
	for (size_t i = 0; i < jointCount; ++i)
	{
		joints[i].SetId(jointSubIds[i]);
		joints[i].SetBindPoseInverse(invertedBindPoses[i]);
	}

	SetDirtyFlag();
	return status;
}

// Write out this controller to a COLLADA XML node tree
xmlNode* FCDSkinController::WriteToXML(xmlNode* parentNode) const
{
	// Create the <skin> element
	xmlNode* skinNode = AddChild(parentNode, DAE_CONTROLLER_SKIN_ELEMENT);
	if (target != NULL) AddAttribute(skinNode, DAE_SOURCE_ATTRIBUTE, fm::string("#") + target->GetDaeId());

	// Create the <bind_shape_matrix> element
	fm::string bindShapeMatrixString = FUStringConversion::ToString(bindShapeTransform);
	AddChild(skinNode, DAE_BINDSHAPEMX_SKIN_PARAMETER, bindShapeMatrixString);

	// Prepare the joint source information
	StringList jointSubIds; jointSubIds.reserve(joints.size());
	FMMatrix44List jointBindPoses; jointBindPoses.reserve(joints.size());
	for (const FCDSkinControllerJoint* it = joints.begin(); it != joints.end(); ++it)
	{
		jointSubIds.push_back((*it).GetId());
		jointBindPoses.push_back((*it).GetBindPoseInverse());
	}
	
	// Create the joint source.
	FUSStringBuilder jointSourceId(parent->GetDaeId()); jointSourceId += "-joints";
	AddSourceString(skinNode, jointSourceId.ToCharPtr(), jointSubIds, DAE_JOINT_SKIN_INPUT);
	
	// Create the joint bind matrix source
	FUSStringBuilder jointBindSourceId(parent->GetDaeId()); jointBindSourceId += "-bind_poses";
	AddSourceMatrix(skinNode, jointBindSourceId.ToCharPtr(), jointBindPoses);

	// Create the weight source
	FloatList weights;
	weights.push_back(1.0f);
	for (const FCDSkinControllerVertex* itW = influences.begin(); itW != influences.end(); ++itW)
	{
		const FCDSkinControllerVertex& vertex = (*itW);
		for (size_t i = 0; i < vertex.GetPairCount(); ++i)
		{
			float w = vertex.GetPair(i)->weight;
			if (!IsEquivalent(w, 1.0f)) weights.push_back(w);
		}
	}
	FUSStringBuilder weightSourceId(parent->GetDaeId()); weightSourceId += "-weights";
	AddSourceFloat(skinNode, weightSourceId.ToCharPtr(), weights, DAE_WEIGHT_SKIN_INPUT);

	// Create the <joints> element
	xmlNode* jointsNode = AddChild(skinNode, DAE_JOINTS_ELEMENT);
	AddInput(jointsNode, jointSourceId.ToCharPtr(), DAE_JOINT_SKIN_INPUT);
	AddInput(jointsNode, jointBindSourceId.ToCharPtr(), DAE_BINDMATRIX_SKIN_INPUT);

	// Create the <vertex_weights> element
	xmlNode* matchesNode = AddChild(skinNode, DAE_WEIGHTS_ELEMENT);
	AddInput(matchesNode, jointSourceId.ToCharPtr(), DAE_JOINT_SKIN_INPUT, 0);
	AddInput(matchesNode, weightSourceId.ToCharPtr(), DAE_WEIGHT_SKIN_INPUT, 1);
	AddAttribute(matchesNode, DAE_COUNT_ATTRIBUTE, GetInfluenceCount());

	// Generate the vertex count and match value strings and export the <v> and <vcount> elements
	FUSStringBuilder vertexCounts; vertexCounts.reserve(1024);
	FUSStringBuilder vertexMatches; vertexMatches.reserve(1024);
	uint32 weightOffset = 1;
	for (const FCDSkinControllerVertex* itW = influences.begin(); itW != influences.end(); ++itW)
	{
		const FCDSkinControllerVertex& vertex = (*itW);
		vertexCounts.append((uint32) vertex.GetPairCount()); vertexCounts.append(' ');
		for (size_t i = 0; i < vertex.GetPairCount(); ++i)
		{
			const FCDJointWeightPair* pair = vertex.GetPair(i);
			vertexMatches.append(pair->jointIndex); vertexMatches.append(' ');
			if (!IsEquivalent(pair->weight, 1.0f)) vertexMatches.append(weightOffset++);
			else vertexMatches.append('0');
			vertexMatches.append(' ');
		}
	}
	if (!vertexMatches.empty()) vertexMatches.pop_back();
	AddChild(matchesNode, DAE_VERTEXCOUNT_ELEMENT, vertexCounts);
	AddChild(matchesNode, DAE_VERTEX_ELEMENT, vertexMatches);
	return skinNode;
}

bool FCDSkinController::LinkImport()
{
	if(GetTarget() == NULL)
	{
		target = GetDocument()->FindGeometry(targetId);
		if (target == NULL) target = GetDocument()->FindController(targetId);
		if (target == NULL)
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_CONTROLLER_TARGET_MISSING, 0);
			return false;
		}

		targetId.clear();
	}
	return true;
}


//
// FCDSkinControllerVertex
//


void FCDSkinControllerVertex::SetPairCount(size_t count)
{
	pairs.resize(count);
}

void FCDSkinControllerVertex::AddPair(int32 jointIndex, float weight)
{
	pairs.push_back(FCDJointWeightPair(jointIndex, weight));
}

//
// FCDSkinControllerJoint
//

void FCDSkinControllerJoint::SetId(const fm::string& _id)
{
	// Do not inline, since the line below does memory allocation.
	id = _id;
}
