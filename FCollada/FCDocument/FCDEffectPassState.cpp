/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDEffectPassState.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeSyntax.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

// 
// Translation Tables
//

static const size_t dataSizeTable[FUDaePassState::COUNT] = 
{
	8, 8, 16, 4, 8, 8, 4, 4, 4, 4,							// [  0-  9]
	4, 4, 4, 8, 4, 6, 12, 10, 16, 5,						// [ 10- 19]
	2, 17, 17, 17, 17, 5, 5, 5, 5, 17, 5,					// [ 20- 30]
	5, 5, 5, 5, 5, 5, 2, 2, 2, 2, 2, 2, 17, 256,			// [ 31- 44]
	17, 2, 16, 16, 4, 4, 4, 8, 1, 8,						// [ 45- 54]
	4, 4, 4, 16, 16, 1, 4, 4,								// [ 55- 62]
	16, 16, 16, 4, 16, 64, 12, 4, 4, 4, 4,					// [ 63- 73]
	8, 64, 16, 4, 1, 1, 1, 1, 1, 1,							// [ 74- 83]
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,						// [ 84- 94]
	1, 1, 1, 1, 1, 1, 1, 1,									// [ 95-102]
	1, 1, 1, 1, 1											// [103-107]
};

//
// FCDEffectPassState
//

ImplementObjectType(FCDEffectPassState);

FCDEffectPassState::FCDEffectPassState(FCDocument* document, FUDaePassState::State renderState)
:	FCDObject(document)
,	type(renderState)
,	data(NULL), dataSize(0)
{
	// Use the translation table to figure out the necessary memory to hold the data.
	if (renderState >= 0 && renderState < FUDaePassState::COUNT) dataSize = dataSizeTable[type];
	else FUFail(dataSize = 1);

	// Allocate the data array right away and only once.
	data = new uint8[dataSize];

	// Set the render state to its default value(s)
	SetDefaultValue();
}

void FCDEffectPassState::SetDefaultValue()
{

#define SET_VALUE(offset, valueType, actualValue) *((valueType*)(data + offset)) = actualValue;
#define SET_ENUM(offset, nameSpace, actualValue) *((uint32*)(data + offset)) = nameSpace::actualValue;

	switch (type)
	{
	case FUDaePassState::ALPHA_FUNC:
		SET_ENUM(0, FUDaePassStateFunction, ALWAYS);
		SET_VALUE(4, float, 0.0f);
		break;

	case FUDaePassState::BLEND_FUNC:
		SET_ENUM(0, FUDaePassStateBlendType, ONE);
		SET_ENUM(4, FUDaePassStateBlendType, ZERO);
		break;

	case FUDaePassState::BLEND_FUNC_SEPARATE:
		SET_ENUM(0, FUDaePassStateBlendType, ONE);
		SET_ENUM(4, FUDaePassStateBlendType, ZERO);
		SET_ENUM(8, FUDaePassStateBlendType, ONE);
		SET_ENUM(12, FUDaePassStateBlendType, ZERO);
		break;

	case FUDaePassState::BLEND_EQUATION:
		SET_ENUM(0, FUDaePassStateBlendEquation, ADD);
		break;

	case FUDaePassState::BLEND_EQUATION_SEPARATE:
		SET_ENUM(0, FUDaePassStateBlendEquation, ADD);
		SET_ENUM(4, FUDaePassStateBlendEquation, ADD);
		break;

	case FUDaePassState::COLOR_MATERIAL:
		SET_ENUM(0, FUDaePassStateFaceType, FRONT_AND_BACK);
		SET_ENUM(4, FUDaePassStateMaterialType, AMBIENT_AND_DIFFUSE);
		break;

	case FUDaePassState::CULL_FACE:
		SET_ENUM(0, FUDaePassStateFaceType, BACK);
		break;

	case FUDaePassState::DEPTH_FUNC:
		SET_ENUM(0, FUDaePassStateFunction, ALWAYS);
		break;

	case FUDaePassState::FOG_MODE:
		SET_ENUM(0, FUDaePassStateFogType, EXP);
		break;

	case FUDaePassState::FOG_COORD_SRC:
		SET_ENUM(0, FUDaePassStateFogCoordinateType, FOG_COORDINATE);
		break;

	case FUDaePassState::FRONT_FACE:
		SET_ENUM(0, FUDaePassStateFrontFaceType, COUNTER_CLOCKWISE);
		break;

	case FUDaePassState::LIGHT_MODEL_COLOR_CONTROL:
		SET_ENUM(0, FUDaePassStateLightModelColorControlType, SINGLE_COLOR);
		break;

	case FUDaePassState::LOGIC_OP:
		SET_ENUM(0, FUDaePassStateLogicOperation, COPY);
		break;

	case FUDaePassState::POLYGON_MODE:
		SET_ENUM(0, FUDaePassStateFaceType, FRONT_AND_BACK);
		SET_ENUM(4, FUDaePassStatePolygonMode, FILL);
		break;

	case FUDaePassState::SHADE_MODEL:
		SET_ENUM(0, FUDaePassStateShadeModel, SMOOTH);
		break;

	case FUDaePassState::STENCIL_FUNC:
		SET_ENUM(0, FUDaePassStateFunction, ALWAYS);
		SET_VALUE(4, uint8, 0);
		SET_VALUE(5, uint8, 0xFF);
		break;

	case FUDaePassState::STENCIL_OP:
		SET_ENUM(0, FUDaePassStateStencilOperation, KEEP);
		SET_ENUM(4, FUDaePassStateStencilOperation, KEEP);
		SET_ENUM(8, FUDaePassStateStencilOperation, KEEP);
		break;

	case FUDaePassState::STENCIL_FUNC_SEPARATE:
		SET_ENUM(0, FUDaePassStateFunction, ALWAYS);
		SET_ENUM(4, FUDaePassStateFunction, ALWAYS);
		SET_VALUE(8, uint8, 0);
		SET_VALUE(9, uint8, 0xFF);
		break;

	case FUDaePassState::STENCIL_OP_SEPARATE:
		SET_ENUM(0, FUDaePassStateFaceType, FRONT_AND_BACK);
		SET_ENUM(4, FUDaePassStateStencilOperation, KEEP);
		SET_ENUM(8, FUDaePassStateStencilOperation, KEEP);
		SET_ENUM(12, FUDaePassStateStencilOperation, KEEP);
		break;

	case FUDaePassState::STENCIL_MASK_SEPARATE:
		SET_ENUM(0, FUDaePassStateFaceType, FRONT_AND_BACK);
		SET_VALUE(4, uint8, 0xFF);
		break;

	case FUDaePassState::LIGHT_ENABLE:
		SET_VALUE(0, uint8, 0);
		SET_VALUE(1, bool, false);
		break;

	case FUDaePassState::LIGHT_AMBIENT:
		SET_VALUE(0, uint8, 0);
		SET_VALUE(1, FMVector4, FMVector4(0,0,0,1));
		break;

	case FUDaePassState::LIGHT_DIFFUSE:
	case FUDaePassState::LIGHT_SPECULAR:
	case FUDaePassState::TEXTURE_ENV_COLOR:
	case FUDaePassState::CLIP_PLANE:
		SET_VALUE(0, uint8, 0);
		SET_VALUE(1, FMVector4, FMVector4::Zero);
		break;

	case FUDaePassState::LIGHT_POSITION:
		SET_VALUE(0, uint8, 0);
		SET_VALUE(1, FMVector4, FMVector4(0,0,1,0));
		break;

	case FUDaePassState::LIGHT_CONSTANT_ATTENUATION:
		SET_VALUE(0, uint8, 0);
		SET_VALUE(1, float, 1.0f);
		break;

	case FUDaePassState::LIGHT_LINEAR_ATTENUATION:
		SET_VALUE(0, uint8, 0);
		SET_VALUE(1, float, 0.0f);
		break;

	case FUDaePassState::LIGHT_QUADRATIC_ATTENUATION:
		SET_VALUE(0, uint8, 0);
		SET_VALUE(1, float, 0.0f);
		break;

	case FUDaePassState::LIGHT_SPOT_CUTOFF:
		SET_VALUE(0, uint8, 0);
		SET_VALUE(1, float, 180.0f);
		break;

	case FUDaePassState::LIGHT_SPOT_DIRECTION:
		SET_VALUE(0, uint8, 0);
		SET_VALUE(1, FMVector3, FMVector3(0,0,-1));
		break;

	case FUDaePassState::LIGHT_SPOT_EXPONENT:
		SET_VALUE(0, uint8, 0);
		SET_VALUE(1, float, 0.0f);
		break;

	case FUDaePassState::TEXTURE1D:
	case FUDaePassState::TEXTURE2D:
	case FUDaePassState::TEXTURE3D:
	case FUDaePassState::TEXTURECUBE:
	case FUDaePassState::TEXTURERECT:
	case FUDaePassState::TEXTUREDEPTH:
		SET_VALUE(0, uint8, 0);
		SET_VALUE(1, uint32, 0);
		break;

	case FUDaePassState::TEXTURE1D_ENABLE:
	case FUDaePassState::TEXTURE2D_ENABLE:
	case FUDaePassState::TEXTURE3D_ENABLE:
	case FUDaePassState::TEXTURECUBE_ENABLE:
	case FUDaePassState::TEXTURERECT_ENABLE:
	case FUDaePassState::TEXTUREDEPTH_ENABLE:
	case FUDaePassState::CLIP_PLANE_ENABLE:
		SET_VALUE(0, uint8, 0);
		SET_VALUE(1, bool, false);
		break;

	case FUDaePassState::TEXTURE_ENV_MODE:
		memset(data, 0, dataSize);
		break;

	case FUDaePassState::BLEND_COLOR:
	case FUDaePassState::CLEAR_COLOR:
	case FUDaePassState::FOG_COLOR:
	case FUDaePassState::SCISSOR:
		SET_VALUE(0, FMVector4, FMVector4::Zero);
		break;

	case FUDaePassState::LIGHT_MODEL_AMBIENT:
	case FUDaePassState::MATERIAL_AMBIENT:
		SET_VALUE(0, FMVector4, FMVector4(0.2f,0.2f,0.2f,1.0f));
		break;

	case FUDaePassState::MATERIAL_DIFFUSE:
		SET_VALUE(0, FMVector4, FMVector4(0.8f,0.8f,0.8f,1.0f));
		break;

	case FUDaePassState::MATERIAL_EMISSION:
	case FUDaePassState::MATERIAL_SPECULAR:
		SET_VALUE(0, FMVector4, FMVector4(0,0,0,1));
		break;

	case FUDaePassState::POINT_DISTANCE_ATTENUATION:
		SET_VALUE(0, FMVector3, FMVector3(1,0,0));
		break;

	case FUDaePassState::DEPTH_BOUNDS:
	case FUDaePassState::DEPTH_RANGE:
		SET_VALUE(0, FMVector2, FMVector2(0,1));
		break;

	case FUDaePassState::POLYGON_OFFSET:
		SET_VALUE(0, FMVector2, FMVector2(0,0));
		break;

	case FUDaePassState::DEPTH_MASK:
		SET_VALUE(0, bool, true);
		break;

	case FUDaePassState::CLEAR_STENCIL:
		SET_VALUE(0, uint32, 0);
		break;

	case FUDaePassState::STENCIL_MASK:
		SET_VALUE(0, uint32, 0xFFFFFFFF);
		break;

	case FUDaePassState::CLEAR_DEPTH:
	case FUDaePassState::FOG_DENSITY:
	case FUDaePassState::FOG_END:
	case FUDaePassState::LINE_WIDTH:
	case FUDaePassState::POINT_FADE_THRESHOLD_SIZE:
	case FUDaePassState::POINT_SIZE:
	case FUDaePassState::POINT_SIZE_MAX:
		SET_VALUE(0, float, 1.0f);
		break;

	case FUDaePassState::FOG_START:
	case FUDaePassState::MATERIAL_SHININESS:
	case FUDaePassState::POINT_SIZE_MIN:
		SET_VALUE(0, float, 0.0f);
		break;

	case FUDaePassState::COLOR_MASK:
		SET_VALUE(0, bool, true);
		SET_VALUE(1, bool, true);
		SET_VALUE(2, bool, true);
		SET_VALUE(3, bool, true);
		break;

	case FUDaePassState::LINE_STIPPLE:
		SET_VALUE(0, uint16, 1);
		SET_VALUE(2, uint16, 0xFF);
		break;

	case FUDaePassState::MODEL_VIEW_MATRIX:
	case FUDaePassState::PROJECTION_MATRIX:
		SET_VALUE(0, FMMatrix44, FMMatrix44::Identity);
		break;

	case FUDaePassState::LIGHTING_ENABLE:
	case FUDaePassState::ALPHA_TEST_ENABLE:
	case FUDaePassState::AUTO_NORMAL_ENABLE:
	case FUDaePassState::BLEND_ENABLE:
	case FUDaePassState::COLOR_LOGIC_OP_ENABLE:
	case FUDaePassState::CULL_FACE_ENABLE:
	case FUDaePassState::DEPTH_BOUNDS_ENABLE:
	case FUDaePassState::DEPTH_CLAMP_ENABLE:
	case FUDaePassState::DEPTH_TEST_ENABLE:
	case FUDaePassState::DITHER_ENABLE:
	case FUDaePassState::FOG_ENABLE:
	case FUDaePassState::LIGHT_MODEL_LOCAL_VIEWER_ENABLE:
	case FUDaePassState::LIGHT_MODEL_TWO_SIDE_ENABLE:
	case FUDaePassState::LINE_SMOOTH_ENABLE:
	case FUDaePassState::LINE_STIPPLE_ENABLE:
	case FUDaePassState::LOGIC_OP_ENABLE:
	case FUDaePassState::MULTISAMPLE_ENABLE:
	case FUDaePassState::NORMALIZE_ENABLE:
	case FUDaePassState::POINT_SMOOTH_ENABLE:
	case FUDaePassState::POLYGON_OFFSET_FILL_ENABLE:
	case FUDaePassState::POLYGON_OFFSET_LINE_ENABLE:
	case FUDaePassState::POLYGON_OFFSET_POINT_ENABLE:
	case FUDaePassState::POLYGON_SMOOTH_ENABLE:
	case FUDaePassState::POLYGON_STIPPLE_ENABLE:
	case FUDaePassState::RESCALE_NORMAL_ENABLE:
	case FUDaePassState::SAMPLE_ALPHA_TO_COVERAGE_ENABLE:
	case FUDaePassState::SAMPLE_ALPHA_TO_ONE_ENABLE:
	case FUDaePassState::SAMPLE_COVERAGE_ENABLE:
	case FUDaePassState::SCISSOR_TEST_ENABLE:
	case FUDaePassState::STENCIL_TEST_ENABLE:
		SET_VALUE(0, bool, false);
		break;

	case FUDaePassState::COLOR_MATERIAL_ENABLE:
		SET_VALUE(0, bool, true);
		break;
	}

#undef SET_ENUM
#undef SET_VALUE
}

FCDEffectPassState::~FCDEffectPassState()
{
	SAFE_DELETE_ARRAY(data);
	dataSize = 0;
	type = FUDaePassState::INVALID;
}

FCDEffectPassState* FCDEffectPassState::Clone(FCDEffectPassState* clone) const
{
	if (clone == NULL)
	{
		clone = new FCDEffectPassState(const_cast<FCDocument*>(GetDocument()), type);
	}

	// The clone's data array should have been allocated properly.
	FUAssert(dataSize == clone->dataSize, return NULL);
	memcpy(clone->data, data, dataSize);
	return clone;
}

bool FCDEffectPassState::LoadFromXML(xmlNode* stateNode)
{
	bool status = true;

#define NODE_TYPE(offset, node, valueType, convFn) \
	if (node != NULL && HasNodeProperty(node, DAE_VALUE_ATTRIBUTE)) { \
		*((valueType*)(data + offset)) = (valueType) FUStringConversion::convFn(ReadNodeProperty(node, DAE_VALUE_ATTRIBUTE)); } 
#define NODE_INDEX(offset, node) \
	if (node != NULL && HasNodeProperty(node, DAE_INDEX_ATTRIBUTE)) { \
		*((uint8*)(data + offset)) = (uint8) FUStringConversion::ToUInt32(ReadNodeProperty(node, DAE_INDEX_ATTRIBUTE)); } 
#define NODE_ENUM(offset, node, nameSpace) \
	if (node != NULL && HasNodeProperty(node, DAE_VALUE_ATTRIBUTE)) { \
		*((uint32*)(data + offset)) = (uint32) nameSpace::FromString(ReadNodeProperty(node, DAE_VALUE_ATTRIBUTE)); } 

#define CHILD_NODE_TYPE(offset, elementName, valueType, convFn) { \
	xmlNode* node = FindChildByType(stateNode, elementName); \
	NODE_TYPE(offset, node, valueType, convFn); }
#define CHILD_NODE_ENUM(offset, elementName, nameSpace) { \
	xmlNode* node = FindChildByType(stateNode, elementName); \
	NODE_ENUM(offset, node, nameSpace); }

	switch (type)
	{
	case FUDaePassState::ALPHA_FUNC:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FUNC_ELEMENT, FUDaePassStateFunction);
		CHILD_NODE_TYPE(4, DAE_FXSTD_STATE_VALUE_ELEMENT, float, ToFloat);
		break;

	case FUDaePassState::BLEND_FUNC:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_SRC_ELEMENT, FUDaePassStateBlendType);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_DEST_ELEMENT, FUDaePassStateBlendType);
		break;

	case FUDaePassState::BLEND_FUNC_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_SRCRGB_ELEMENT, FUDaePassStateBlendType);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_DESTRGB_ELEMENT, FUDaePassStateBlendType);
		CHILD_NODE_ENUM(8, DAE_FXSTD_STATE_SRCALPHA_ELEMENT, FUDaePassStateBlendType);
		CHILD_NODE_ENUM(12, DAE_FXSTD_STATE_DESTALPHA_ELEMENT, FUDaePassStateBlendType);
		break;

	case FUDaePassState::BLEND_EQUATION:
		NODE_ENUM(0, stateNode, FUDaePassStateBlendEquation);
		break;

	case FUDaePassState::BLEND_EQUATION_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_RGB_ELEMENT, FUDaePassStateBlendEquation);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_ALPHA_ELEMENT, FUDaePassStateBlendEquation);
		break;

	case FUDaePassState::COLOR_MATERIAL:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_MODE_ELEMENT, FUDaePassStateMaterialType);
		break;

	case FUDaePassState::CULL_FACE:
		NODE_ENUM(0, stateNode, FUDaePassStateFaceType);
		break;

	case FUDaePassState::DEPTH_FUNC:
		NODE_ENUM(0, stateNode, FUDaePassStateFunction);
		break;

	case FUDaePassState::FOG_MODE:
		NODE_ENUM(0, stateNode, FUDaePassStateFogType);
		break;

	case FUDaePassState::FOG_COORD_SRC:
		NODE_ENUM(0, stateNode, FUDaePassStateFogCoordinateType);
		break;

	case FUDaePassState::FRONT_FACE:
		NODE_ENUM(0, stateNode, FUDaePassStateFrontFaceType);
		break;

	case FUDaePassState::LIGHT_MODEL_COLOR_CONTROL:
		NODE_ENUM(0, stateNode, FUDaePassStateLightModelColorControlType);
		break;

	case FUDaePassState::LOGIC_OP:
		NODE_ENUM(0, stateNode, FUDaePassStateLogicOperation);
		break;

	case FUDaePassState::POLYGON_MODE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_MODE_ELEMENT, FUDaePassStatePolygonMode);
		break;

	case FUDaePassState::SHADE_MODEL:
		NODE_ENUM(0, stateNode, FUDaePassStateShadeModel);
		break;

	case FUDaePassState::STENCIL_FUNC:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FUNC_ELEMENT, FUDaePassStateFunction);
		CHILD_NODE_TYPE(4, DAE_FXSTD_STATE_REF_ELEMENT, uint8, ToUInt32);
		CHILD_NODE_TYPE(5, DAE_FXSTD_STATE_MASK_ELEMENT, uint8, ToUInt32);
		break;

	case FUDaePassState::STENCIL_OP:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FAIL_ELEMENT, FUDaePassStateStencilOperation);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_ZFAIL_ELEMENT, FUDaePassStateStencilOperation);
		CHILD_NODE_ENUM(8, DAE_FXSTD_STATE_ZPASS_ELEMENT, FUDaePassStateStencilOperation);
		break;

	case FUDaePassState::STENCIL_FUNC_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FRONT_ELEMENT, FUDaePassStateFunction);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_BACK_ELEMENT, FUDaePassStateFunction);
		CHILD_NODE_TYPE(8, DAE_FXSTD_STATE_REF_ELEMENT, uint8, ToUInt32);
		CHILD_NODE_TYPE(9, DAE_FXSTD_STATE_MASK_ELEMENT, uint8, ToUInt32);
		break;

	case FUDaePassState::STENCIL_OP_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_FAIL_ELEMENT, FUDaePassStateStencilOperation);
		CHILD_NODE_ENUM(8, DAE_FXSTD_STATE_ZFAIL_ELEMENT, FUDaePassStateStencilOperation);
		CHILD_NODE_ENUM(12, DAE_FXSTD_STATE_ZPASS_ELEMENT, FUDaePassStateStencilOperation);
		break;

	case FUDaePassState::STENCIL_MASK_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType);
		CHILD_NODE_TYPE(4, DAE_FXSTD_STATE_MASK_ELEMENT, uint8, ToUInt32);
		break;

	case FUDaePassState::LIGHT_AMBIENT:
	case FUDaePassState::LIGHT_DIFFUSE:
	case FUDaePassState::LIGHT_SPECULAR:
	case FUDaePassState::LIGHT_POSITION:
	case FUDaePassState::TEXTURE_ENV_COLOR:
	case FUDaePassState::CLIP_PLANE:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, FMVector4, ToVector4);
		break;

	case FUDaePassState::LIGHT_CONSTANT_ATTENUATION:
	case FUDaePassState::LIGHT_LINEAR_ATTENUATION:
	case FUDaePassState::LIGHT_QUADRATIC_ATTENUATION:
	case FUDaePassState::LIGHT_SPOT_CUTOFF:
	case FUDaePassState::LIGHT_SPOT_EXPONENT:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, float, ToFloat);
		break;

	case FUDaePassState::LIGHT_SPOT_DIRECTION:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, FMVector3, ToVector3);
		break;

	case FUDaePassState::TEXTURE1D:
	case FUDaePassState::TEXTURE2D:
	case FUDaePassState::TEXTURE3D:
	case FUDaePassState::TEXTURECUBE:
	case FUDaePassState::TEXTURERECT:
	case FUDaePassState::TEXTUREDEPTH:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, uint32, ToUInt32);
		break;

	case FUDaePassState::LIGHT_ENABLE:
	case FUDaePassState::TEXTURE1D_ENABLE:
	case FUDaePassState::TEXTURE2D_ENABLE:
	case FUDaePassState::TEXTURE3D_ENABLE:
	case FUDaePassState::TEXTURECUBE_ENABLE:
	case FUDaePassState::TEXTURERECT_ENABLE:
	case FUDaePassState::TEXTUREDEPTH_ENABLE:
	case FUDaePassState::CLIP_PLANE_ENABLE:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, bool, ToBoolean);
		break;

	case FUDaePassState::TEXTURE_ENV_MODE: {
		NODE_INDEX(0, stateNode);
		fm::string value = ReadNodeProperty(stateNode, DAE_VALUE_ATTRIBUTE);
		memcpy(data + 1, value.c_str(), min(value.size(), (size_t) 255));
		data[255] = 0;
		break; }

	case FUDaePassState::BLEND_COLOR:
	case FUDaePassState::CLEAR_COLOR:
	case FUDaePassState::FOG_COLOR:
	case FUDaePassState::LIGHT_MODEL_AMBIENT:
	case FUDaePassState::MATERIAL_AMBIENT:
	case FUDaePassState::MATERIAL_DIFFUSE:
	case FUDaePassState::MATERIAL_EMISSION:
	case FUDaePassState::MATERIAL_SPECULAR:
	case FUDaePassState::SCISSOR:
		NODE_TYPE(0, stateNode, FMVector4, ToVector4);
		break;

	case FUDaePassState::POINT_DISTANCE_ATTENUATION:
		NODE_TYPE(0, stateNode, FMVector3, ToVector3);
		break;

	case FUDaePassState::DEPTH_BOUNDS:
	case FUDaePassState::DEPTH_RANGE:
	case FUDaePassState::POLYGON_OFFSET:
		NODE_TYPE(0, stateNode, FMVector2, ToVector2);
		break;

	case FUDaePassState::CLEAR_STENCIL:
	case FUDaePassState::STENCIL_MASK:
		NODE_TYPE(0, stateNode, uint32, ToUInt32);
		break;

	case FUDaePassState::CLEAR_DEPTH:
	case FUDaePassState::FOG_DENSITY:
	case FUDaePassState::FOG_START:
	case FUDaePassState::FOG_END:
	case FUDaePassState::LINE_WIDTH:
	case FUDaePassState::MATERIAL_SHININESS:
	case FUDaePassState::POINT_FADE_THRESHOLD_SIZE:
	case FUDaePassState::POINT_SIZE:
	case FUDaePassState::POINT_SIZE_MIN:
	case FUDaePassState::POINT_SIZE_MAX:
		NODE_TYPE(0, stateNode, float, ToFloat);
		break;

	case FUDaePassState::COLOR_MASK: {
		fm::string value = ReadNodeProperty(stateNode, DAE_VALUE_ATTRIBUTE);
		BooleanList values;
		FUStringConversion::ToBooleanList(value, values);
		if (values.size() >= 4)
		{
			*(bool*)(data + 0) = values[0];
			*(bool*)(data + 1) = values[1];
			*(bool*)(data + 2) = values[2];
			*(bool*)(data + 3) = values[3];
		}
		break; }

	case FUDaePassState::LINE_STIPPLE: {
		fm::string value = ReadNodeProperty(stateNode, DAE_VALUE_ATTRIBUTE);
		UInt32List values;
		FUStringConversion::ToUInt32List(value, values);
		if (values.size() >= 2)
		{
			*(uint16*)(data + 0) = (uint16) values[0];
			*(uint16*)(data + 2) = (uint16) values[1];
		}
		break; }

	case FUDaePassState::MODEL_VIEW_MATRIX:
	case FUDaePassState::PROJECTION_MATRIX:
		NODE_TYPE(0, stateNode, FMMatrix44, ToMatrix);
		break;

	case FUDaePassState::LIGHTING_ENABLE:
	case FUDaePassState::ALPHA_TEST_ENABLE:
	case FUDaePassState::AUTO_NORMAL_ENABLE:
	case FUDaePassState::BLEND_ENABLE:
	case FUDaePassState::COLOR_LOGIC_OP_ENABLE:
	case FUDaePassState::COLOR_MATERIAL_ENABLE:
	case FUDaePassState::CULL_FACE_ENABLE:
	case FUDaePassState::DEPTH_BOUNDS_ENABLE:
	case FUDaePassState::DEPTH_CLAMP_ENABLE:
	case FUDaePassState::DEPTH_TEST_ENABLE:
	case FUDaePassState::DITHER_ENABLE:
	case FUDaePassState::FOG_ENABLE:
	case FUDaePassState::LIGHT_MODEL_LOCAL_VIEWER_ENABLE:
	case FUDaePassState::LIGHT_MODEL_TWO_SIDE_ENABLE:
	case FUDaePassState::LINE_SMOOTH_ENABLE:
	case FUDaePassState::LINE_STIPPLE_ENABLE:
	case FUDaePassState::LOGIC_OP_ENABLE:
	case FUDaePassState::MULTISAMPLE_ENABLE:
	case FUDaePassState::NORMALIZE_ENABLE:
	case FUDaePassState::POINT_SMOOTH_ENABLE:
	case FUDaePassState::POLYGON_OFFSET_FILL_ENABLE:
	case FUDaePassState::POLYGON_OFFSET_LINE_ENABLE:
	case FUDaePassState::POLYGON_OFFSET_POINT_ENABLE:
	case FUDaePassState::POLYGON_SMOOTH_ENABLE:
	case FUDaePassState::POLYGON_STIPPLE_ENABLE:
	case FUDaePassState::RESCALE_NORMAL_ENABLE:
	case FUDaePassState::SAMPLE_ALPHA_TO_COVERAGE_ENABLE:
	case FUDaePassState::SAMPLE_ALPHA_TO_ONE_ENABLE:
	case FUDaePassState::SAMPLE_COVERAGE_ENABLE:
	case FUDaePassState::SCISSOR_TEST_ENABLE:
	case FUDaePassState::STENCIL_TEST_ENABLE:
	case FUDaePassState::DEPTH_MASK:
		NODE_TYPE(0, stateNode, bool, ToBoolean);
		break;

	default:
		status = false;
		break;
	}

#undef NODE_TYPE
#undef NODE_INDEX
#undef NODE_ENUM
#undef CHILD_NODE_TYPE
#undef CHILD_NODE_ENUM

	return status;
}

xmlNode* FCDEffectPassState::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* stateNode = AddChild(parentNode, FUDaePassState::ToString(type));

#define NODE_TYPE(offset, node, valueType, castType) \
	AddAttribute(node, DAE_VALUE_ATTRIBUTE, FUStringConversion::ToString((castType) *((valueType*)(data + offset))));
#define NODE_INDEX(offset, node) \
	AddAttribute(node, DAE_INDEX_ATTRIBUTE, FUStringConversion::ToString((uint32) *((uint8*)(data + offset))));
#define NODE_ENUM(offset, node, nameSpace, enumName) \
	AddAttribute(node, DAE_VALUE_ATTRIBUTE, nameSpace::ToString((nameSpace::enumName) *((uint32*)(data + offset))));

#define CHILD_NODE_TYPE(offset, elementName, valueType, castType) { \
	xmlNode* node = AddChild(stateNode, elementName); \
	NODE_TYPE(offset, node, valueType, castType); }
#define CHILD_NODE_ENUM(offset, elementName, nameSpace, enumName) { \
	xmlNode* node = AddChild(stateNode, elementName); \
	NODE_ENUM(offset, node, nameSpace, enumName); }

	switch (type)
	{
	case FUDaePassState::ALPHA_FUNC:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FUNC_ELEMENT, FUDaePassStateFunction, Function);
		CHILD_NODE_TYPE(4, DAE_FXSTD_STATE_VALUE_ELEMENT, float, float);
		break;

	case FUDaePassState::BLEND_FUNC:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_SRC_ELEMENT, FUDaePassStateBlendType, Type);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_DEST_ELEMENT, FUDaePassStateBlendType, Type);
		break;

	case FUDaePassState::BLEND_FUNC_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_SRCRGB_ELEMENT, FUDaePassStateBlendType, Type);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_DESTRGB_ELEMENT, FUDaePassStateBlendType, Type);
		CHILD_NODE_ENUM(8, DAE_FXSTD_STATE_SRCALPHA_ELEMENT, FUDaePassStateBlendType, Type);
		CHILD_NODE_ENUM(12, DAE_FXSTD_STATE_DESTALPHA_ELEMENT, FUDaePassStateBlendType, Type);
		break;

	case FUDaePassState::BLEND_EQUATION:
		NODE_ENUM(0, stateNode, FUDaePassStateBlendEquation, Equation);
		break;

	case FUDaePassState::BLEND_EQUATION_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_RGB_ELEMENT, FUDaePassStateBlendEquation, Equation);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_ALPHA_ELEMENT, FUDaePassStateBlendEquation, Equation);
		break;

	case FUDaePassState::COLOR_MATERIAL:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType, Type);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_MODE_ELEMENT, FUDaePassStateMaterialType, Type);
		break;

	case FUDaePassState::CULL_FACE:
		NODE_ENUM(0, stateNode, FUDaePassStateFaceType, Type);
		break;

	case FUDaePassState::DEPTH_FUNC:
		NODE_ENUM(0, stateNode, FUDaePassStateFunction, Function);
		break;

	case FUDaePassState::FOG_MODE:
		NODE_ENUM(0, stateNode, FUDaePassStateFogType, Type);
		break;

	case FUDaePassState::FOG_COORD_SRC:
		NODE_ENUM(0, stateNode, FUDaePassStateFogCoordinateType, Type);
		break;

	case FUDaePassState::FRONT_FACE:
		NODE_ENUM(0, stateNode, FUDaePassStateFrontFaceType, Type);
		break;

	case FUDaePassState::LIGHT_MODEL_COLOR_CONTROL:
		NODE_ENUM(0, stateNode, FUDaePassStateLightModelColorControlType, Type);
		break;

	case FUDaePassState::LOGIC_OP:
		NODE_ENUM(0, stateNode, FUDaePassStateLogicOperation, Operation);
		break;

	case FUDaePassState::POLYGON_MODE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType, Type);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_MODE_ELEMENT, FUDaePassStatePolygonMode, Mode);
		break;

	case FUDaePassState::SHADE_MODEL:
		NODE_ENUM(0, stateNode, FUDaePassStateShadeModel, Model);
		break;

	case FUDaePassState::STENCIL_FUNC:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FUNC_ELEMENT, FUDaePassStateFunction, Function);
		CHILD_NODE_TYPE(4, DAE_FXSTD_STATE_REF_ELEMENT, uint8, uint32);
		CHILD_NODE_TYPE(5, DAE_FXSTD_STATE_MASK_ELEMENT, uint8, uint32);
		break;

	case FUDaePassState::STENCIL_OP:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FAIL_ELEMENT, FUDaePassStateStencilOperation, Operation);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_ZFAIL_ELEMENT, FUDaePassStateStencilOperation, Operation);
		CHILD_NODE_ENUM(8, DAE_FXSTD_STATE_ZPASS_ELEMENT, FUDaePassStateStencilOperation, Operation);
		break;

	case FUDaePassState::STENCIL_FUNC_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FRONT_ELEMENT, FUDaePassStateFunction, Function);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_BACK_ELEMENT, FUDaePassStateFunction, Function);
		CHILD_NODE_TYPE(8, DAE_FXSTD_STATE_REF_ELEMENT, uint8, uint32);
		CHILD_NODE_TYPE(9, DAE_FXSTD_STATE_MASK_ELEMENT, uint8, uint32);
		break;

	case FUDaePassState::STENCIL_OP_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType, Type);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_FAIL_ELEMENT, FUDaePassStateStencilOperation, Operation);
		CHILD_NODE_ENUM(8, DAE_FXSTD_STATE_ZFAIL_ELEMENT, FUDaePassStateStencilOperation, Operation);
		CHILD_NODE_ENUM(12, DAE_FXSTD_STATE_ZPASS_ELEMENT, FUDaePassStateStencilOperation, Operation);
		break;

	case FUDaePassState::STENCIL_MASK_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType, Type);
		CHILD_NODE_TYPE(4, DAE_FXSTD_STATE_MASK_ELEMENT, uint8, uint32);
		break;

	case FUDaePassState::LIGHT_AMBIENT:
	case FUDaePassState::LIGHT_DIFFUSE:
	case FUDaePassState::LIGHT_SPECULAR:
	case FUDaePassState::LIGHT_POSITION:
	case FUDaePassState::TEXTURE_ENV_COLOR:
	case FUDaePassState::CLIP_PLANE:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, FMVector4, FMVector4);
		break;

	case FUDaePassState::LIGHT_CONSTANT_ATTENUATION:
	case FUDaePassState::LIGHT_LINEAR_ATTENUATION:
	case FUDaePassState::LIGHT_QUADRATIC_ATTENUATION:
	case FUDaePassState::LIGHT_SPOT_CUTOFF:
	case FUDaePassState::LIGHT_SPOT_EXPONENT:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, float, float);
		break;

	case FUDaePassState::LIGHT_SPOT_DIRECTION:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, FMVector3, FMVector3);
		break;

	case FUDaePassState::TEXTURE1D:
	case FUDaePassState::TEXTURE2D:
	case FUDaePassState::TEXTURE3D:
	case FUDaePassState::TEXTURECUBE:
	case FUDaePassState::TEXTURERECT:
	case FUDaePassState::TEXTUREDEPTH:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, uint32, uint32);
		break;

	case FUDaePassState::LIGHT_ENABLE:
	case FUDaePassState::TEXTURE1D_ENABLE:
	case FUDaePassState::TEXTURE2D_ENABLE:
	case FUDaePassState::TEXTURE3D_ENABLE:
	case FUDaePassState::TEXTURECUBE_ENABLE:
	case FUDaePassState::TEXTURERECT_ENABLE:
	case FUDaePassState::TEXTUREDEPTH_ENABLE:
	case FUDaePassState::CLIP_PLANE_ENABLE:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, bool, bool);
		break;

	case FUDaePassState::TEXTURE_ENV_MODE: {
		NODE_INDEX(0, stateNode);
		fm::string overrideAvoidance((const char*) (data + 1), 254);
		AddAttribute(stateNode, DAE_VALUE_ATTRIBUTE, overrideAvoidance);
		break; }

	case FUDaePassState::BLEND_COLOR:
	case FUDaePassState::CLEAR_COLOR:
	case FUDaePassState::FOG_COLOR:
	case FUDaePassState::LIGHT_MODEL_AMBIENT:
	case FUDaePassState::MATERIAL_AMBIENT:
	case FUDaePassState::MATERIAL_DIFFUSE:
	case FUDaePassState::MATERIAL_EMISSION:
	case FUDaePassState::MATERIAL_SPECULAR:
	case FUDaePassState::SCISSOR:
		NODE_TYPE(0, stateNode, FMVector4, FMVector4);
		break;

	case FUDaePassState::POINT_DISTANCE_ATTENUATION:
		NODE_TYPE(0, stateNode, FMVector3, FMVector3);
		break;

	case FUDaePassState::DEPTH_BOUNDS:
	case FUDaePassState::DEPTH_RANGE:
	case FUDaePassState::POLYGON_OFFSET:
		NODE_TYPE(0, stateNode, FMVector2, FMVector2);
		break;

	case FUDaePassState::CLEAR_STENCIL:
	case FUDaePassState::STENCIL_MASK:
		NODE_TYPE(0, stateNode, uint32, uint32);
		break;

	case FUDaePassState::CLEAR_DEPTH:
	case FUDaePassState::FOG_DENSITY:
	case FUDaePassState::FOG_START:
	case FUDaePassState::FOG_END:
	case FUDaePassState::LINE_WIDTH:
	case FUDaePassState::MATERIAL_SHININESS:
	case FUDaePassState::POINT_FADE_THRESHOLD_SIZE:
	case FUDaePassState::POINT_SIZE:
	case FUDaePassState::POINT_SIZE_MIN:
	case FUDaePassState::POINT_SIZE_MAX:
		NODE_TYPE(0, stateNode, float, float);
		break;

	case FUDaePassState::COLOR_MASK:
		globalSBuilder.set(*(bool*)(data + 0)); globalSBuilder.append(' ');
		globalSBuilder.append(*(bool*)(data + 1)); globalSBuilder.append(' ');
		globalSBuilder.append(*(bool*)(data + 2)); globalSBuilder.append(' ');
		globalSBuilder.append(*(bool*)(data + 3));
		AddAttribute(stateNode, DAE_VALUE_ATTRIBUTE, globalSBuilder);
		break;

	case FUDaePassState::LINE_STIPPLE:
		globalSBuilder.set((uint32) *(uint16*)(data + 0)); globalSBuilder.append(' ');
		globalSBuilder.append((uint32) *(uint16*)(data + 2));
		AddAttribute(stateNode, DAE_VALUE_ATTRIBUTE, globalSBuilder);
		break;

	case FUDaePassState::MODEL_VIEW_MATRIX:
	case FUDaePassState::PROJECTION_MATRIX:
		NODE_TYPE(0, stateNode, FMMatrix44, FMMatrix44);
		break;

	case FUDaePassState::LIGHTING_ENABLE:
	case FUDaePassState::ALPHA_TEST_ENABLE:
	case FUDaePassState::AUTO_NORMAL_ENABLE:
	case FUDaePassState::BLEND_ENABLE:
	case FUDaePassState::COLOR_LOGIC_OP_ENABLE:
	case FUDaePassState::COLOR_MATERIAL_ENABLE:
	case FUDaePassState::CULL_FACE_ENABLE:
	case FUDaePassState::DEPTH_BOUNDS_ENABLE:
	case FUDaePassState::DEPTH_CLAMP_ENABLE:
	case FUDaePassState::DEPTH_TEST_ENABLE:
	case FUDaePassState::DITHER_ENABLE:
	case FUDaePassState::FOG_ENABLE:
	case FUDaePassState::LIGHT_MODEL_LOCAL_VIEWER_ENABLE:
	case FUDaePassState::LIGHT_MODEL_TWO_SIDE_ENABLE:
	case FUDaePassState::LINE_SMOOTH_ENABLE:
	case FUDaePassState::LINE_STIPPLE_ENABLE:
	case FUDaePassState::LOGIC_OP_ENABLE:
	case FUDaePassState::MULTISAMPLE_ENABLE:
	case FUDaePassState::NORMALIZE_ENABLE:
	case FUDaePassState::POINT_SMOOTH_ENABLE:
	case FUDaePassState::POLYGON_OFFSET_FILL_ENABLE:
	case FUDaePassState::POLYGON_OFFSET_LINE_ENABLE:
	case FUDaePassState::POLYGON_OFFSET_POINT_ENABLE:
	case FUDaePassState::POLYGON_SMOOTH_ENABLE:
	case FUDaePassState::POLYGON_STIPPLE_ENABLE:
	case FUDaePassState::RESCALE_NORMAL_ENABLE:
	case FUDaePassState::SAMPLE_ALPHA_TO_COVERAGE_ENABLE:
	case FUDaePassState::SAMPLE_ALPHA_TO_ONE_ENABLE:
	case FUDaePassState::SAMPLE_COVERAGE_ENABLE:
	case FUDaePassState::SCISSOR_TEST_ENABLE:
	case FUDaePassState::STENCIL_TEST_ENABLE:
	case FUDaePassState::DEPTH_MASK:
		NODE_TYPE(0, stateNode, bool, bool);
		break;
	}

#undef NODE_TYPE
#undef NODE_INDEX
#undef NODE_ENUM
#undef CHILD_NODE_TYPE
#undef CHILD_NODE_ENUM

	return stateNode;
}
