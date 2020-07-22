/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUDaeEnum.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDGeometrySource);

FCDGeometrySource::FCDGeometrySource(FCDocument* document, FUDaeGeometryInput::Semantic type)
:	FCDObjectWithId(document, "GeometrySource")
,   stride(0), sourceNode(NULL), sourceType(type)
,	extra(NULL)
{
}

FCDGeometrySource::~FCDGeometrySource()
{
	animatedValues.clear();
	SAFE_RELEASE(extra);
}

void FCDGeometrySource::SetDataCount(size_t count)
{

	sourceData.resize(count);
	SetDirtyFlag();
}

FCDGeometrySource* FCDGeometrySource::Clone(FCDGeometrySource* clone) const
{
	if (clone == NULL) clone = new FCDGeometrySource(const_cast<FCDocument*>(GetDocument()), sourceType);
	FCDObjectWithId::Clone(clone);
	clone->name = name;

	// Clone the data of this source.
	clone->stride = stride;
	clone->sourceData = sourceData;
	clone->sourceType = sourceType;

	// Clone the extra information.
	if (extra != NULL)
	{
		extra->Clone(clone->GetExtra());
	}

	return clone;	
}

void FCDGeometrySource::SetData(const FloatList& _sourceData, uint32 _sourceStride, size_t offset, size_t count)
{
	// Remove all the data currently held by the source.
	sourceData.clear();
	stride = _sourceStride;

	// Check the given bounds
	size_t beg = min(offset, _sourceData.size()), end;
	if (count == 0) end = _sourceData.size();
	else end = min(count + offset, _sourceData.size());
	sourceData.insert(sourceData.begin(), _sourceData.begin() + beg, _sourceData.begin() + end);

	SetDirtyFlag();
}

void FCDGeometrySource::SetType(FUDaeGeometryInput::Semantic type)
{
	sourceType = type;
	animatedValues.clear();

	// Most types should remain un-animated
	if (sourceType != FUDaeGeometryInput::POSITION && sourceType != FUDaeGeometryInput::COLOR) return;

	// Look for an animation on this source's objects
	Int32List animatedIndices;
	GetDocument()->FindAnimationChannelsArrayIndices(sourceNode, animatedIndices);
	for (Int32List::iterator itA = animatedIndices.begin(); itA != animatedIndices.end(); ++itA)
	{
		// Check for repeated animated indices
		Int32List::iterator itB = animatedIndices.begin();
		for (; itB != itA && (*itA) != (*itB); ++itB) {}
		if (itB != itA) continue;

		FCDAnimated* animated = NULL;
		if (sourceType == FUDaeGeometryInput::POSITION)
		{
			animated = FCDAnimatedPoint3::Create(GetDocument(), sourceNode, (FMVector3*)&(sourceData[(*itA) * stride]), *itA);
		}
		else if (stride == 4)
		{
			animated = FCDAnimatedColor::Create(GetDocument(), sourceNode, (FMVector4*)&(sourceData[(*itA) * stride]), *itA);
		}
		else if (stride == 3)
		{
			animated = FCDAnimatedColor::Create(GetDocument(), sourceNode, (FMVector3*)&(sourceData[(*itA) * stride]), *itA);
		}
		else if (stride == 1)
		{
			animated = FCDAnimatedFloat::Create(GetDocument(), sourceNode, &(sourceData[(*itA)]), *itA);
		}

		// Keep track of these animated values
		if (animated != NULL) animatedValues.push_back(animated);
	}
	SetDirtyFlag();
}

FCDExtra* FCDGeometrySource::GetExtra()
{
	return (extra != NULL) ? extra : extra = new FCDExtra(GetDocument(), this);
}

// Read in the <source> node of the COLLADA document
bool FCDGeometrySource::LoadFromXML(xmlNode* _sourceNode)
{
	bool status = true;
	sourceNode = _sourceNode;

	// Read in the name and id of the source
	name = TO_FSTRING(ReadNodeName(sourceNode));
	fm::string id = ReadNodeId(sourceNode);
	if (id.empty())
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_GEOMETRY_SOURCE_ID, sourceNode->line);
	}
	SetDaeId(id);
	if (!id.empty() && GetDaeId() != id)
	{
		//return status.Fail(FS("Geometry source has duplicate 'id': ") + TO_FSTRING(id), sourceNode->line);
		FUError::Error(FUError::ERROR, FUError::ERROR_DUPLICATE_ID, sourceNode->line);
	}

	// Read in the source data
	stride = ReadSource(sourceNode, sourceData);
	if (stride == 0)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_EMPTY_SOURCE, sourceNode->line);
	}

	// If the <source> element has non-common techniques: we need to parse the extra information from them.
	if (extra == NULL) extra = new FCDExtra(GetDocument(), this);
	extra->LoadFromXML(sourceNode);
	if (extra->GetDefaultType()->GetTechniqueCount() == 0)
	{
		SAFE_RELEASE(extra);
	}

	return status;
}

// Write out the <source> node to the COLLADA XML tree
xmlNode* FCDGeometrySource::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* sourceNode = NULL;

	// Export the source directly, using the correct parameters and the length factor
	switch (sourceType)
	{
	case FUDaeGeometryInput::POSITION: sourceNode = AddSourceFloat(parentNode, GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::NORMAL: sourceNode = AddSourceFloat(parentNode, GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::GEOTANGENT: sourceNode = AddSourceFloat(parentNode, GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::GEOBINORMAL: sourceNode = AddSourceFloat(parentNode, GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::TEXCOORD: sourceNode = AddSourceFloat(parentNode, GetDaeId(), sourceData, stride, FUDaeAccessor::STPQ); break;
	case FUDaeGeometryInput::TEXTANGENT: sourceNode = AddSourceFloat(parentNode, GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::TEXBINORMAL: sourceNode = AddSourceFloat(parentNode, GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::UV: sourceNode = AddSourceFloat(parentNode, GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::COLOR: sourceNode = AddSourceFloat(parentNode, GetDaeId(), sourceData, stride, FUDaeAccessor::RGBA); break;
	case FUDaeGeometryInput::EXTRA: sourceNode = AddSourceFloat(parentNode, GetDaeId(), sourceData, stride, NULL); break;
	case FUDaeGeometryInput::UNKNOWN: sourceNode = AddSourceFloat(parentNode, GetDaeId(), sourceData, stride, NULL); break;

	case FUDaeGeometryInput::VERTEX: // Refuse to export these sources
	default: break;
	}

	if (!name.empty())
	{
		AddAttribute(sourceNode, DAE_NAME_ATTRIBUTE, name);
	}

	if (extra != NULL)
	{
		extra->WriteTechniquesToXML(sourceNode);
	}

	for (FCDAnimatedList::const_iterator itA = animatedValues.begin(); itA != animatedValues.end(); ++itA)
	{
		GetDocument()->WriteAnimatedValueToXML((*itA), sourceNode, "");
	}

	return sourceNode;
}
