/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUtils/FUError.h"

//
// FUError
//
FUError::Level FUError::fatalLevel = FUError::ERROR;
FUError::FUErrorFunctor* FUError::errorCallback = NULL;
FUError::FUErrorFunctor* FUError::warningCallback = NULL;
FUError::FUErrorFunctor* FUError::debugCallback = NULL;

FUError::FUError()
{
}

FUError::~FUError()
{
	SAFE_DELETE(FUError::warningCallback);
	SAFE_DELETE(FUError::errorCallback);
	SAFE_DELETE(FUError::debugCallback);
}

bool FUError::Error(FUError::Level errorLevel, uint32 errorCode, uint32 line)
{
	switch(errorLevel)
	{
	case FUError::WARNING:
		if (warningCallback != NULL)
		{
			(*warningCallback)(errorLevel, errorCode, line);
		}
		break;
	case FUError::ERROR:
		if (errorCallback != NULL)
		{
			(*errorCallback)(errorLevel, errorCode, line);
		}
		break;
	case FUError::D3BUG:
		if (debugCallback != NULL)
		{
			(*debugCallback)(errorLevel, errorCode, line);
		}
		break;
	}

	return errorLevel >= fatalLevel;
}

void FUError::SetErrorCallback(FUError::Level errorLevel, FUError::FUErrorFunctor* callback)
{ 
	switch(errorLevel)
	{
	case FUError::WARNING:
		SAFE_DELETE(warningCallback);
		warningCallback = callback;
		break;

	case FUError::ERROR:
		SAFE_DELETE(errorCallback);
		errorCallback = callback;
		break;

	case FUError::D3BUG:
		SAFE_DELETE(debugCallback);
		debugCallback = callback;
		break;
	}
}

const char* FUError::GetErrorString(FUError::Code errorCode)
{
	char* ptrChar=NULL;

	switch(errorCode)
	{
	case ERROR_DEFAULT_ERROR: ptrChar="Generic Error."; break;
	case ERROR_MALFORMED_XML: ptrChar="Corrupted COLLADA document: malformed XML."; break;
	case ERROR_PARSING_FAILED: ptrChar="Exception caught while parsing a COLLADA document from file."; break;
	case ERROR_INVALID_ELEMENT: ptrChar="Invalid or unexpected XML element."; break;
	case ERROR_MISSING_ELEMENT: ptrChar="Missing, necessary XML element."; break;
	case ERROR_UNKNOWN_ELEMENT: ptrChar="Unknown element: parsing error."; break;
	case ERROR_MISSING_INPUT: ptrChar="Missing necessary COLLADA <input>."; break;
	case ERROR_INVALID_URI: ptrChar="Incomplete or invalid URI fragment."; break;
	case ERROR_WRITE_FILE: ptrChar="Unable to write COLLADA document to file."; break;
	case ERROR_MISSING_PROPERTY: ptrChar="Missing necessary XML property."; break;

	case ERROR_ANIM_CURVE_DRIVER_MISSING: ptrChar="Unable to find animation curve driver."; break;
	case ERROR_SOURCE_SIZE: ptrChar="Expecting sources to be the same size."; break;

	case ERROR_IB_MATRIX_MISSING: ptrChar="No inverted bind matrix input in controller."; break;
	case ERROR_VCOUNT_MISSING: ptrChar="Expecting <vcount> element in combiner for controller."; break;
	case ERROR_V_ELEMENT_MISSING: ptrChar="Expecting <v> element after <vcount> element in combiner for controller."; break;
	case ERROR_JC_BPMC_NOT_EQUAL: ptrChar="Joint count and bind pose matrix count aren't equal for controller."; break;
	case ERROR_INVALID_VCOUNT: ptrChar="The <vcount> element list should contains the number of values determined by the <vertex_weights>'s 'count' attribute."; break;
	case ERROR_PARSING_PROG_ERROR: ptrChar="Parsing programming error in controller."; break;
	case ERROR_UNKNOWN_CHILD: ptrChar="Unknown child in <geometry> with id."; break;
	case ERROR_UNKNOWN_GEO_CH: ptrChar="Unknown geometry for creation of convex hull of."; break;
	case ERROR_UNKNOWN_MESH_ID: ptrChar="Mesh has source with an unknown id."; break;
	case ERROR_INVALID_U_KNOT: ptrChar="Found non-ascending U knot vector"; break;
	case ERROR_INVALID_V_KNOT: ptrChar="Found non-ascending V knot vector"; break;
	case ERROR_NOT_ENOUGHT_U_KNOT: ptrChar="Not enough elements in the U knot vector."; break;
	case ERROR_NOT_ENOUGHT_V_KNOT: ptrChar="Not enough elements in the V knot vector."; break;
	case ERROR_INVALID_CONTROL_VERTICES: ptrChar="Found unexpected number of control vertices."; break;
	case ERROR_NO_CONTROL_VERTICES: ptrChar="No <control_vertices> element in NURBS surface."; break;
	case ERROR_UNKNOWN_POLYGONS: ptrChar="Unknown polygons element in geometry."; break;
	case WARNING_NO_POLYGON: ptrChar="No polygon <p>/<vcount> element found in geometry."; break;
	case ERROR_NO_VERTEX_INPUT: ptrChar="Cannot find 'VERTEX' polygons' input within geometry."; break;
	case ERROR_NO_VCOUNT: ptrChar="No or empty <vcount> element found in geometry." ; break;
	case ERROR_MISPLACED_VCOUNT: ptrChar="<vcount> is only expected with the <polylist> element in geometry."; break;
	case ERROR_UNKNOWN_PH_ELEMENT: ptrChar="Unknown element found in <ph> element for geometry."; break;
	case ERROR_INVALID_FACE_COUNT: ptrChar="Face count for polygons node doesn't match actual number of faces found in <p> element(s) in geometry."; break;
	case ERROR_DUPLICATE_ID: ptrChar="Geometry source has duplicate 'id'."; break;
	case ERROR_INVALID_CVS_WEIGHTS: ptrChar="Numbers of CVs and weights are different in NURB spline."; break;
	case ERROR_INVALID_SPLINE: ptrChar="Invalid spline. Equation \"n = k - d - 1\" is not respected."; break;
	case ERROR_UNKNOWN_EFFECT_CODE: ptrChar="Unknown effect code type."; break;
	case ERROR_BAD_FLOAT_VALUE: ptrChar="Bad value for float parameter in integer parameter."; break;
	case ERROR_BAD_BOOLEAN_VALUE: ptrChar="Bad value for boolean parameter in effect."; break;
	case ERROR_BAD_FLOAT_PARAM: ptrChar="Bad float value for float parameter."; break;
	case ERROR_BAD_FLOAT_PARAM2: ptrChar="Bad value for float2 parameter."; break;
	case ERROR_BAD_FLOAT_PARAM3: ptrChar="Bad value for float3 parameter."; break;
	case ERROR_BAD_FLOAT_PARAM4: ptrChar="Bad value for float4 parameter."; break;
	case ERROR_BAD_MATRIX: ptrChar="Bad value for matrix parameter."; break;
	case ERROR_PROG_NODE_MISSING: ptrChar="Unable to find the program node for standard effect."; break;
	case ERROR_INVALID_TEXTURE_SAMPLER: ptrChar="Unexpected texture sampler on some parameters for material."; break;
	case ERROR_PARAM_NODE_MISSING: ptrChar="Cannot find parameter node referenced by."; break;
	case ERROR_INVALID_IMAGE_FILENAME: ptrChar="Invalid filename for image: "; break;
	case ERROR_UNKNOWN_TEXTURE_SAMPLER: ptrChar="Unknown texture sampler element."; break;
	case ERROR_COMMON_TECHNIQUE_MISSING: ptrChar="Unable to find common technique for physics material."; break;
	case ERROR_TECHNIQUE_NODE_MISSING: ptrChar="Technique node not specified."; break;

	case ERROR_MAX_CANNOT_RESIZE_MUT_LIST: ptrChar="Cannot Resize the ParticleMutationsList"; break;

	case WARNING_MISSING_URI_TARGET: ptrChar="Missing or invalid URI target."; break;
	case WARNING_UNKNOWN_CHILD_ELEMENT: ptrChar="Unknown <asset> child element"; break;
	case WARNING_UNKNOWN_AC_CHILD_ELEMENT: ptrChar="Unknown <asset><contributor> child element."; break;
	case WARNING_BASE_NODE_TYPE: ptrChar="Unknown base node type."; break;
	case WARNING_INST_ENTITY_MISSING: ptrChar="Unable to find instantiated entity."; break;
	case WARNING_INVALID_MATERIAL_BINDING: ptrChar="Invalid material binding in geometry instantiation."; break;
	case WARNING_UNKNOWN_MAT_ID: ptrChar="Unknown material id or semantic."; break;
	case WARNING_RIGID_CONSTRAINT_MISSING: ptrChar="Couldn't find rigid constraint for instantiation."; break;
	case WARNING_INVALID_ANIM_LIB: ptrChar="Animation library contains unknown element."; break;
	case WARNING_UNKNOWN_ANIM_LIB_ELEMENT: ptrChar="Unknown element in animation clip library."; break;
	case WARNING_INVALID_SE_PAIR: ptrChar="Invalid start/end pair for animation clip."; break;
	case WARNING_CURVES_MISSING: ptrChar="No curves instantiated by animation."; break;
	case WARNING_EMPTY_ANIM_CLIP: ptrChar="Empty animation clip."; break;
	case WARNING_UNKNOWN_CAM_ELEMENT: ptrChar="Camera library contains unknown element."; break;
	case WARNING_NO_STD_PROG_TYPE: ptrChar="No standard program type for camera."; break;
	case WARNING_PARAM_ROOT_MISSING: ptrChar="Cannot find parameter root node for camera."; break;
	case WARNING_UNKNOWN_CAM_PROG_TYPE: ptrChar="Unknown program type for camera."; break;
	case WARNING_UNKNOWN_CAM_PARAM: ptrChar="Unknown parameter for camera."; break;
	case WARNING_UNKNOWN_LIGHT_LIB_ELEMENT: ptrChar="Light library contains unknown element."; break;
	case WARNING_UNKNOWN_LIGHT_TYPE_VALUE: ptrChar="Unknown light type value for light."; break;
	case WARNING_UNKNOWN_LT_ELEMENT: ptrChar="Unknown element under <light><technique_common> for light."; break;
	case WARNING_UNKNOWN_LIGHT_PROG_PARAM: ptrChar="Unknown program parameter for light."; break;
	case WARNING_INVALID_CONTROlLER_LIB_NODE: ptrChar="Unexpected node in controller library."; break;
	case WARNING_CONTROLLER_TYPE_CONFLICT: ptrChar="A controller cannot be both a skin and a morpher."; break;
	case WARNING_SM_BASE_MISSING: ptrChar="No base type element, <skin> or <morph>, found for controller."; break;
	case WARNING_UNKNOWN_MC_PROC_METHOD: ptrChar="Unknown processing method from morph controller."; break;
	case WARNING_UNKNOWN_MC_BASE_TARGET_MISSING: ptrChar="Cannot find base target for morph controller."; break;
	case WARNING_UNKNOWN_MORPH_TARGET_TYPE: ptrChar="Unknown morph targets input type in morph controller."; break;
	case WARNING_TARGET_GEOMETRY_MISSING: ptrChar="Unable to find target geometry."; break;
	case WARNING_CONTROLLER_TARGET_MISSING: ptrChar="Target not found for controller."; break;
	case WARNING_UNKNOWN_SC_VERTEX_INPUT: ptrChar="Unknown vertex input in skin controller."; break;
	case WARNING_INVALID_TARGET_GEOMETRY_OP: ptrChar="Unable to clone/find the target geometry for controller."; break;
	case WARNING_INVALID_JOINT_INDEX: ptrChar="Joint index out of bounds in combiner for controller."; break;
	case WARNING_INVALID_WEIGHT_INDEX: ptrChar="Weight index out of bounds in combiner for controller."; break;
	case WARNING_UNKNOWN_JOINT: ptrChar="Unknown joint."; break;
	case WARNING_UNKNOWN_GL_ELEMENT: ptrChar="Geometry library contains unknown element."; break;
	case WARNING_EMPTY_GEOMETRY: ptrChar="No mesh, spline or NURBS surfaces found within geometry."; break;
	case WARNING_MESH_VERTICES_MISSING: ptrChar="No <vertices> element in mesh."; break;
	case WARNING_VP_INPUT_NODE_MISSING: ptrChar="No vertex position input node in mesh."; break;
	case WARNING_GEOMETRY_VERTICES_MISSING: ptrChar="Empty <vertices> element in geometry."; break;
	case WARNING_MESH_TESSELLATION_MISSING: ptrChar="No tessellation found for mesh."; break;
	case WARNING_INVALID_POLYGON_MAT_SYMBOL: ptrChar="Unknown or missing polygonal material symbol in geometry."; break;
	case WARNING_EXTRA_VERTEX_INPUT: ptrChar="There should never be more than one 'VERTEX' input in a mesh: skipping extra 'VERTEX' inputs."; break;
	case WARNING_UNKNOWN_POLYGONS_INPUT: ptrChar="Unknown polygons set input."; break;
	case WARNING_UNKNOWN_POLYGON_CHILD: ptrChar="Unknown polygon child element in geometry."; break;
	case WARNING_INVALID_PRIMITIVE_COUNT: ptrChar="Primitive count for mesh node doesn't match actual number of primitives found in <p> element(s) in geometry."; break;
	case WARNING_INVALID_GEOMETRY_SOURCE_ID: ptrChar="Geometry source with no 'id' is unusable."; break;
	case WARNING_EMPTY_SOURCE: ptrChar="Geometry has source with no data."; break;
	case WARNING_SPLINE_CONTROL_INPUT_MISSING: ptrChar="No control vertice input in spline."; break;
	case WARNING_CONTROL_VERTICES_MISSING: ptrChar="No <control_vertices> element in spline."; break;
	case WARNING_VARYING_SPLINE_TYPE: ptrChar="Geometry contains different kinds of splines."; break;
	case WARNING_UNKNOWN_EFFECT_ELEMENT: ptrChar="Unknown element in effect library."; break;
	case WARNING_UNSUPPORTED_PROFILE: ptrChar="Unsupported profile or unknown element in effect."; break;
	case WARNING_SID_MISSING: ptrChar="<code>/<include> nodes must have an 'sid' attribute to identify them."; break;
	case WARNING_INVALID_ANNO_TYPE: ptrChar="Annotation has none-supported type."; break;
	case WARNING_GEN_REF_ATTRIBUTE_MISSING: ptrChar="No reference attribute on generator parameter."; break;
	case WARNING_MOD_REF_ATTRIBUTE_MISSING: ptrChar="No reference attribute on modifier parameter."; break;
	case WARNING_SAMPLER_NODE_MISSING: ptrChar="Unable to find sampler node for sampler parameter."; break;
	case WARNING_EMPTY_SURFACE_SOURCE: ptrChar="Empty surface source value for sampler parameter."; break;
	case WARNING_EMPTY_INIT_FROM: ptrChar="<init_from> element is empty in surface parameter."; break;
	case WARNING_EMPTY_IMAGE_NAME: ptrChar="Empty image name for surface parameter."; break;
	case WARNING_UNKNOWN_PASS_ELEMENT: ptrChar="Pass contains unknown element."; break;
	case WARNING_UNKNOWN_PASS_SHADER_ELEMENT: ptrChar="Pass shader contains unknown element."; break;
	case WARNING_UNAMED_EFFECT_PASS_SHADER: ptrChar="Unnamed effect pass shader found."; break;
	case WARNING_UNKNOWN_EPS_STAGE: ptrChar="Unknown stage for effect pass shader."; break;
	case WARNING_INVALID_PROFILE_INPUT_NODE: ptrChar="Invalid profile input node for effect"; break;
	case WARNING_UNKNOWN_STD_MAT_BASE_ELEMENT: ptrChar="Unknown element as standard material base."; break;
	case WARNING_TECHNIQUE_MISSING: ptrChar="Expecting <technique> within the <profile_COMMON> element for effect."; break;
	case WARNING_UNKNOWN_MAT_INPUT_SEMANTIC: ptrChar="Unknown input semantic in material."; break;
	case WARNING_UNKNOWN_INPUT_TEXTURE: ptrChar="Unknown input texture."; break;
	case WARNING_UNSUPPORTED_SHADER_TYPE: ptrChar="Unsupported shader program type."; break;
	case WARNING_UNKNOWN_MAT_PARAM_NAME: ptrChar="Unknown parameter name for material."; break;
	case WARNING_UNKNOWN_TECHNIQUE_ELEMENT: ptrChar="Technique contains unknown element."; break;
	case WARNING_UNKNOWN_IMAGE_LIB_ELEMENT: ptrChar="Image library contains unknown element."; break;
	case WARNING_UNKNOWN_TEX_LIB_ELEMENT: ptrChar="Texture library contains unknown element."; break;
	case WARNING_UNKNOWN_CHANNEL_USAGE: ptrChar="Unknown channel usage for texture."; break;
	case WARNING_UNKNOWN_INPUTE_SEMANTIC: ptrChar="Unknown input semantic for texture."; break;
	case WARNING_UNKNOWN_IMAGE_SOURCE: ptrChar="Unknown or external image source for texture."; break;
	case WARNING_UNKNOWN_MAT_LIB_ELEMENT: ptrChar="Unknown element in material library."; break;
	case WARNING_UNSUPPORTED_REF_EFFECTS: ptrChar="Externally referenced effects are not supported. Material."; break;
	case WARNING_EMPTY_INSTANCE_EFFECT: ptrChar="Empty material's <instance_effect> definition. Should instantiate an effect from the effect's library. Material."; break;
	case WARNING_EFFECT_MISSING: ptrChar="Unable to find effect for material."; break;
	case WARNING_UNKNOWN_FORCE_FIELD_ELEMENT: ptrChar="Force field library contains unknown element."; break;
	case WARNING_UNKNOWN_ELEMENT: ptrChar="Unknown element."; break;
	case WARNING_INVALID_BOX_TYPE: ptrChar="Box is not of the right type."; break;
	case WARNING_INVALID_PLANE_TYPE: ptrChar="Plane is not of the right type."; break;
	case WARNING_INVALID_SPHERE_TYPE: ptrChar="Sphere is not of the right type."; break;
	case WARNING_INVALID_CAPSULE_TYPE: ptrChar="Capsule is not of the right type."; break;
	case WARNING_INVALID_TCAPSULE_TYPE: ptrChar="Tapered Capsule is not of the right type."; break;
	case WARNING_INVALID_TCYLINDER_TYPE: ptrChar="Tapered cylinder is not of the right type."; break;
	case WARNING_UNKNOWN_PHYS_MAT_LIB_ELEMENT: ptrChar="Unknown element in physics material library."; break;

	case WARNING_UNKNOWN_PHYS_LIB_ELEMENT: ptrChar="PhysicsModel library contains unknown element."; break;
	case WARNING_CORRUPTED_INSTANCE: ptrChar="Unable to retrieve instance for scene node."; break;
	case WARNING_UNKNOWN_PRB_LIB_ELEMENT: ptrChar="PhysicsRigidBody library contains unknown element."; break;
	case WARNING_PHYS_MAT_INST_MISSING: ptrChar="Error: Instantiated physics material in rigid body was not found."; break;
	case WARNING_PHYS_MAT_DEF_MISSING: ptrChar="No physics material defined in rigid body."; break;
	case WARNING_UNKNOWN_RGC_LIB_ELEMENT: ptrChar="PhysicsRigidConstraint library contains unknown element."; break;
	case WARNING_INVALID_NODE_TRANSFORM: ptrChar="Invalid node transform."; break;
	case WARNING_RF_NODE_MISSING: ptrChar="Reference-frame rigid body/scene node not defined in rigid constraint."; break;
	case WARNING_RF_REF_NODE_MISSING: ptrChar="Reference-frame rigid body/scene node specified in rigid_constraint not found in physics model."; break;
	case WARNING_TARGET_BS_NODE_MISSING: ptrChar="Target rigid body/scene node not defined in rigid constraint."; break;
	case WARNING_TARGE_BS_REF_NODE_MISSING: ptrChar="Target rigid body/scene node specified in rigid_constraint not found in physics model."; break;
	case WARNING_UNKNOW_PS_LIB_ELEMENT: ptrChar="PhysicsShape library contains unknown element."; break;
	case WARNING_FCDGEOMETRY_INST_MISSING: ptrChar="Unable to retrieve FCDGeometry instance for scene node. "; break;
	case WARNING_INVALID_SHAPE_NODE: ptrChar="Invalid shape."; break;
	case WARNING_UNKNOW_NODE_ELEMENT_TYPE: ptrChar="Unknown node type for scene's <node> element."; break;
	case WARNING_CYCLE_DETECTED: ptrChar="A cycle was found in the visual scene at node."; break;
	case WARNING_INVALID_NODE_INST: ptrChar="Unable to retrieve node instance for scene node."; break;
	case WARNING_INVALID_WEAK_NODE_INST: ptrChar="Unable to retrieve weakly-typed instance for scene node."; break;
	case WARNING_UNSUPPORTED_EXTERN_REF: ptrChar="FCollada does not support external references for this element/entity."; break;
	case WARNING_UNEXPECTED_ASSET: ptrChar="Found more than one asset present in scene node."; break;
	case WARNING_INVALID_TRANSFORM: ptrChar="Unknown element or bad transform in scene node."; break;
	case WARNING_TARGET_SCENE_NODE_MISSING: ptrChar="Unable to find target scene node for object."; break;
	case WARNING_XREF_UNASSIGNED: ptrChar="XRef imported but not instanciated."; break;
	case WARNING_UNSUPPORTED_EXTERN_REF_NODE: ptrChar="Unsupported external reference node."; break;
	
	case DEBUG_LOAD_SUCCESSFUL: ptrChar="COLLADA document loaded successfully."; break;
	case DEBUG_WRITE_SUCCESSFUL: ptrChar="COLLADA document written successfully."; break;
	}

	return ptrChar;
}

//
// FUErrorSimpleHandler
//

FUErrorSimpleHandler::FUErrorSimpleHandler(FUError::Level fatalLevel)
:	fails(false)
{
	FUError::SetFatalityLevel(fatalLevel);
	FUError::SetErrorCallback(FUError::D3BUG, NewFUFunctor3(this, &FUErrorSimpleHandler::OnError));
	FUError::SetErrorCallback(FUError::WARNING, NewFUFunctor3(this, &FUErrorSimpleHandler::OnError));
	FUError::SetErrorCallback(FUError::ERROR, NewFUFunctor3(this, &FUErrorSimpleHandler::OnError));
}

FUErrorSimpleHandler::~FUErrorSimpleHandler()
{
	FUError::SetErrorCallback(FUError::D3BUG, NULL);
	FUError::SetErrorCallback(FUError::WARNING, NULL);
	FUError::SetErrorCallback(FUError::ERROR, NULL);
}

void FUErrorSimpleHandler::OnError(FUError::Level errorLevel, uint32 errorCode, uint32 lineNumber)
{
	FUSStringBuilder newLine(256);
	newLine.append('['); newLine.append(lineNumber); newLine.append("] ");
	if (errorLevel == FUError::WARNING) newLine.append("Warning: ");
	else if (errorLevel == FUError::ERROR) newLine.append("ERROR: ");
	const char* errorString = FUError::GetErrorString((FUError::Code) errorCode);
	if (errorString != NULL) newLine.append(errorString);
	else
	{
		newLine.append("Unknown error code: ");
		newLine.append(errorCode);
	}

	if (message.length() > 0) message.append('\n');
	message.append(newLine);

	fails |= errorLevel >= FUError::GetFatalityLevel(); 
}
