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
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDTexture.h"
#include "FUtils/FUDaeEnum.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUStringConversion.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDEffectStandard);

FCDEffectStandard::FCDEffectStandard(FCDEffect* _parent)
:	FCDEffectProfile(_parent)
,	type(CONSTANT), textureBuckets(NULL)
,	emissionColor(0.0f, 0.0f, 0.0f, 1.0f), emissionFactor(1.0f), isEmissionFactor(false)
,	isReflective(true), reflectivityColor(FMVector4::One), reflectivityFactor(1.0f), indexOfRefraction(1.0f)
,	translucencyColor(0.0f, 0.0f, 0.0f, 1.0f), translucencyFactor(1.0f)
,	diffuseColor(0.0f, 0.0f, 0.0f, 1.0f), ambientColor(0.0f, 0.0f, 0.0f, 1.0f)
,	specularColor(0.0f, 0.0f, 0.0f, 1.0f), specularFactor(1.0f), shininess(20.0f)
{
	textureBuckets = new FCDTextureContainer[FUDaeTextureChannel::COUNT];

	// Note: the 1.4.1 spec calls for A_ONE as default, which breaks backwards compatibility
	// with 1.3 by having a transparency factor of 1 for opaque instead of 0 in 1.3.
	// If we're lower than 1.4.1, we default to RGB_ZERO and a transparency (or translucency)
	// factor of 0.0.
	transparencyMode = GetDocument()->GetVersion().IsLowerThan("1.4.1") ? RGB_ZERO : A_ONE;
}

FCDEffectStandard::~FCDEffectStandard()
{
	SAFE_DELETE_ARRAY(textureBuckets);
}

// Retrieve one of the buckets
FCDTextureContainer& FCDEffectStandard::GetTextureBucket(uint32 bucket)
{ return const_cast<FCDTextureContainer&>(const_cast<const FCDEffectStandard*>(this)->GetTextureBucket(bucket)); }
const FCDTextureContainer& FCDEffectStandard::GetTextureBucket(uint32 bucket) const
{
	if (bucket < FUDaeTextureChannel::COUNT) return textureBuckets[bucket];
	else return textureBuckets[FUDaeTextureChannel::FILTER]; // Because I think this one will almost always be empty. ;)
}

// Adds a texture to a specific channel.
FCDTexture* FCDEffectStandard::AddTexture(uint32 bucket)
{
	FUAssert(bucket < FUDaeTextureChannel::COUNT, return NULL);
	FCDTexture* texture = new FCDTexture(GetDocument(), this);
	textureBuckets[bucket].push_back(texture);
	SetDirtyFlag();
	return texture;
}

// Releases a texture contained within this effect profile.
void FCDEffectStandard::ReleaseTexture(FCDTexture* texture)
{
	for (uint32 i = 0; i < FUDaeTextureChannel::COUNT; ++i)
	{
		if (textureBuckets[i].release(texture))
		{
			SetDirtyFlag();
			break;
		}
	}
}

// Calculate the opacity for this material
float FCDEffectStandard::GetOpacity() const
{
	if (transparencyMode == RGB_ZERO)
		return 1.0f - (translucencyColor.x + translucencyColor.y + translucencyColor.z) / 3.0f * translucencyFactor;
	else
		return translucencyColor.w * translucencyFactor;
}

// Calculate the overall reflectivity for this material
float FCDEffectStandard::GetReflectivity() const
{
	return (reflectivityColor.x + reflectivityColor.y + reflectivityColor.z) / 3.0f * reflectivityFactor;
}

// Look for the effect parameter with the correct semantic, in order to bind/set its value
FCDEffectParameter* FCDEffectStandard::FindParameterBySemantic(const fm::string& semantic)
{
	FCDEffectParameter* p = FCDEffectProfile::FindParameterBySemantic(semantic);
	for (uint32 i = 0; i < FUDaeTextureChannel::COUNT && p == NULL; ++i)
	{
		for (FCDTextureContainer::iterator itT = textureBuckets[i].begin(); itT != textureBuckets[i].end() && p == NULL; ++itT)
		{
			p = (*itT)->FindParameterBySemantic(semantic);
		}
	}

	return p;
}

void FCDEffectStandard::FindParametersBySemantic(const fm::string& semantic, FCDEffectParameterList& parameters)
{
	FCDEffectProfile::FindParametersBySemantic(semantic, parameters);
	for (uint32 i = 0; i < FUDaeTextureChannel::COUNT; ++i)
	{
		for (FCDTextureContainer::iterator itT = textureBuckets[i].begin(); itT != textureBuckets[i].end(); ++itT)
		{
			(*itT)->FindParametersBySemantic(semantic, parameters);
		}
	}
}

void FCDEffectStandard::FindParametersByReference(const fm::string& reference, FCDEffectParameterList& parameters)
{
	FCDEffectProfile::FindParametersByReference(reference, parameters);
	for (uint32 i = 0; i < FUDaeTextureChannel::COUNT; ++i)
	{
		for (FCDTextureContainer::iterator itT = textureBuckets[i].begin(); itT != textureBuckets[i].end(); ++itT)
		{
			(*itT)->FindParametersByReference(reference, parameters);
		}
	}
}

// Clone the standard effect
FCDEffectProfile* FCDEffectStandard::Clone(FCDEffectProfile* _clone) const
{
	FCDEffectStandard* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDEffectStandard(const_cast<FCDEffect*>(GetParent()));
	else if (_clone->GetObjectType() == FCDEffectStandard::GetClassType()) clone = (FCDEffectStandard*) _clone;

	if (_clone != NULL) FCDEffectProfile::Clone(_clone);
	if (clone != NULL)
	{
		clone->type = type;
		for (uint32 i = 0; i < FUDaeTextureChannel::COUNT; ++i)
		{
			for (FCDTextureContainer::iterator itT = textureBuckets[i].begin(); itT != textureBuckets[i].end(); ++itT)
			{
				FCDTexture* clonedTexture = clone->textureBuckets[i].Add(clone->GetDocument(), clone);
				(*itT)->Clone(clonedTexture);
			}
		}
		clone->transparencyMode = transparencyMode;

#define CLONE_ANIMATED_F(flt) clone->flt = flt; FCDAnimatedFloat::Clone(const_cast<FCDocument*>(GetDocument()), &flt, &clone->flt);
#define CLONE_ANIMATED_C(flt) clone->flt = flt; FCDAnimatedColor::Clone(const_cast<FCDocument*>(GetDocument()), &flt, &clone->flt);

		CLONE_ANIMATED_C(emissionColor); CLONE_ANIMATED_F(emissionFactor); clone->isEmissionFactor = isEmissionFactor;
		CLONE_ANIMATED_C(translucencyColor); CLONE_ANIMATED_F(translucencyFactor);
		CLONE_ANIMATED_C(diffuseColor); CLONE_ANIMATED_C(ambientColor);
		CLONE_ANIMATED_C(specularColor); CLONE_ANIMATED_F(specularFactor); CLONE_ANIMATED_F(shininess);
		CLONE_ANIMATED_C(reflectivityColor); CLONE_ANIMATED_F(reflectivityFactor); CLONE_ANIMATED_F(indexOfRefraction);

#undef CLONE_ANIMATED_F
#undef CLONE_ANIMATED_C
	}

	return _clone;
}

/** [INTERNAL] Links the effect profile and its parameters. This is done after
	the whole COLLADA document has been processed once. */
void FCDEffectStandard::Link()
{
	FCDEffectProfile::Link();

	// Link the textures with the sampler parameters
	for (uint32 i = 0; i < FUDaeTextureChannel::COUNT; ++i)
	{
		for (FCDTextureContainer::iterator itT = textureBuckets[i].begin(); itT != textureBuckets[i].end(); ++itT)
		{
			(*itT)->Link(*GetParameters());
		}
	}
}

void FCDEffectStandard::AddExtraAttribute(const char* profile, const char* key, const fchar* value)
{
	FUAssert(GetParent() != NULL, return);
	FCDETechnique* extraTech = GetParent()->GetExtra()->GetDefaultType()->FindTechnique(profile);
	if (extraTech == NULL) extraTech = GetParent()->GetExtra()->GetDefaultType()->AddTechnique(profile);
	FCDENode *enode= extraTech->AddParameter(key, value);
	enode->SetName(key);
	enode->SetContent(value);
	SetDirtyFlag();
}

const fchar* FCDEffectStandard::GetExtraAttribute(const char* profile, const char* key) const
{
	FUAssert(GetParent() != NULL, return NULL);
	const FCDETechnique * extraTech = GetParent()->GetExtra()->GetDefaultType()->FindTechnique(profile);
	if (extraTech == NULL) return NULL;
	const FCDENode * enode = extraTech->FindParameter(key);
	if (enode == NULL) return NULL;
	return enode->GetContent();
}

// Read in a <material> node from the COLLADA document
bool FCDEffectStandard::LoadFromXML(xmlNode* baseNode)
{
	bool status = FCDEffectProfile::LoadFromXML(baseNode);
	if (!status) return status;

	// Find the node with the Max/Maya/FC-specific parameters
	xmlNode* maxParameterNode = NULL;
	xmlNode* mayaParameterNode = NULL;
	xmlNode* fcParameterNode = NULL;

	// Bump the base node up the first <technique> element
	xmlNode* techniqueNode = FindChildByType(baseNode, DAE_TECHNIQUE_ELEMENT);
	if (techniqueNode == NULL)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_TECHNIQUE_MISSING, baseNode->line);
		return status;
	}
	baseNode = techniqueNode;

	// Look for an <extra><technique> node for Max-specific parameter
	xmlNode* extraNode = FindChildByType(baseNode, DAE_EXTRA_ELEMENT);
	maxParameterNode = FindTechnique(extraNode, DAEMAX_MAX_PROFILE);
	mayaParameterNode = FindTechnique(extraNode, DAEMAYA_MAYA_PROFILE);
	fcParameterNode = FindTechnique(extraNode, DAE_FCOLLADA_PROFILE);

	// Parse the material's program node and figure out the correct shader type
	// Either <phong>, <lambert> or <constant> are expected
	xmlNode* commonParameterNode = NULL;
	for (commonParameterNode = baseNode->children; commonParameterNode != NULL; commonParameterNode = commonParameterNode->next)
	{
		if (commonParameterNode->type != XML_ELEMENT_NODE) continue;
		if (IsEquivalent(commonParameterNode->name, DAE_FXSTD_CONSTANT_ELEMENT)) { type = CONSTANT; break; }
		else if (IsEquivalent(commonParameterNode->name, DAE_FXSTD_LAMBERT_ELEMENT)) { type = LAMBERT; break; }
		else if (IsEquivalent(commonParameterNode->name, DAE_FXSTD_PHONG_ELEMENT)) { type = PHONG; break; }
		else if (IsEquivalent(commonParameterNode->name, DAE_FXSTD_BLINN_ELEMENT)) { type = BLINN; break; }
	}
	if (commonParameterNode == NULL)
	{
		//return status.Fail(FS("Unable to find the program node for standard effect: ") + TO_FSTRING(GetDaeId()), baseNode->line);
		FUError::Error(FUError::ERROR, FUError::ERROR_PROG_NODE_MISSING, baseNode->line);
	}

	bool hasTranslucency = false, hasReflectivity = false;
	FCDTextureContainer emptyBucket;

	// Read in the parameters for the common program types and apply them to the shader
	StringList parameterNames;
	xmlNodeList parameterNodes;
	FindParameters(commonParameterNode, parameterNames, parameterNodes);
	FindParameters(maxParameterNode, parameterNames, parameterNodes);
	FindParameters(mayaParameterNode, parameterNames, parameterNodes);
	FindParameters(fcParameterNode, parameterNames, parameterNodes);
	size_t parameterCount = parameterNodes.size();
	for (size_t i = 0; i < parameterCount; ++i)
	{
		xmlNode* parameterNode = parameterNodes[i];
		const fm::string& parameterName = parameterNames[i];
		const char* parameterContent = ReadNodeContentDirect(parameterNode);
		if (parameterName == DAE_EMISSION_MATERIAL_PARAMETER)
		{
			status &= (ParseColorTextureParameter(parameterNode, emissionColor, textureBuckets[FUDaeTextureChannel::EMISSION]));
		}
		else if (parameterName == DAE_DIFFUSE_MATERIAL_PARAMETER)
		{
			status &= (ParseColorTextureParameter(parameterNode, diffuseColor, textureBuckets[FUDaeTextureChannel::DIFFUSE]));
		}
		else if (parameterName == DAE_AMBIENT_MATERIAL_PARAMETER)
		{
			status &= (ParseColorTextureParameter(parameterNode, ambientColor, textureBuckets[FUDaeTextureChannel::AMBIENT]));
		}
		else if (parameterName == DAE_TRANSPARENT_MATERIAL_PARAMETER)
		{
			fm::string opaque = ReadNodeProperty(parameterNode, DAE_OPAQUE_MATERIAL_ATTRIBUTE);
			if (IsEquivalentI(opaque, DAE_RGB_ZERO_ELEMENT))
				transparencyMode = RGB_ZERO;
			else if (IsEquivalentI(opaque, DAE_A_ONE_ELEMENT))
				transparencyMode = A_ONE;

			status &= (ParseColorTextureParameter(parameterNode, translucencyColor, textureBuckets[FUDaeTextureChannel::TRANSPARENT]));
			hasTranslucency = true;
		}
		else if (parameterName == DAE_TRANSPARENCY_MATERIAL_PARAMETER)
		{
			status &= (ParseFloatTextureParameter(parameterNode, translucencyFactor, textureBuckets[FUDaeTextureChannel::OPACITY]));
			hasTranslucency = true;
		}
		else if (parameterName == DAE_SPECULAR_MATERIAL_PARAMETER)
		{
			status &= (ParseColorTextureParameter(parameterNode, specularColor, textureBuckets[FUDaeTextureChannel::SPECULAR]));
		}
		else if (parameterName == DAE_SHININESS_MATERIAL_PARAMETER)
		{
			status &= (ParseFloatTextureParameter(parameterNode, shininess, textureBuckets[FUDaeTextureChannel::SHININESS]));
		}
		else if (parameterName == DAE_REFLECTIVE_MATERIAL_PARAMETER)
		{
			status &= (ParseColorTextureParameter(parameterNode, reflectivityColor, textureBuckets[FUDaeTextureChannel::REFLECTION]));
			hasReflectivity = true;
		}
		else if (parameterName == DAE_REFLECTIVITY_MATERIAL_PARAMETER)
		{
			status &= (ParseFloatTextureParameter(parameterNode, reflectivityFactor, emptyBucket));
			hasReflectivity = true;
		}
		else if (parameterName == DAE_INDEXOFREFRACTION_MATERIAL_PARAMETER)
		{
			status &= (ParseFloatTextureParameter(parameterNode, indexOfRefraction, emptyBucket));
		}
		else if (parameterName == DAE_BUMP_MATERIAL_PARAMETER)
		{
			status &= (ParseSimpleTextureParameter(parameterNode, textureBuckets[FUDaeTextureChannel::BUMP]));
		}
		else if (parameterName == DAEMAX_SPECLEVEL_MATERIAL_PARAMETER)
		{
			status &= (ParseFloatTextureParameter(parameterNode, specularFactor, textureBuckets[FUDaeTextureChannel::SPECULAR_LEVEL]));
		}
		else if (parameterName == DAEMAX_EMISSIONLEVEL_MATERIAL_PARAMETER)
		{
			status &= (ParseFloatTextureParameter(parameterNode, emissionFactor, textureBuckets[FUDaeTextureChannel::EMISSION]));
			isEmissionFactor = true;
		}
		else if (parameterName == DAEMAX_FACETED_MATERIAL_PARAMETER)
		{
			AddExtraAttribute(DAEMAX_MAX_PROFILE, DAEMAX_FACETED_MATERIAL_PARAMETER, TO_FSTRING(parameterContent).c_str());
		}
		else if (parameterName == DAESHD_DOUBLESIDED_PARAMETER)
		{
			AddExtraAttribute(DAEMAX_MAX_PROFILE, DAESHD_DOUBLESIDED_PARAMETER, TO_FSTRING(parameterContent).c_str());
		}
		else if (parameterName == DAEMAX_WIREFRAME_MATERIAL_PARAMETER)
		{
			AddExtraAttribute(DAEMAX_MAX_PROFILE, DAEMAX_WIREFRAME_MATERIAL_PARAMETER, TO_FSTRING(parameterContent).c_str());
		}
		else if (parameterName == DAEMAX_FACEMAP_MATERIAL_PARAMETER)
		{
			AddExtraAttribute(DAEMAX_MAX_PROFILE, DAEMAX_FACEMAP_MATERIAL_PARAMETER, TO_FSTRING(parameterContent).c_str());
		}
		else if (parameterName == DAEMAX_INDEXOFREFRACTION_MATERIAL_PARAMETER)
		{
			status &= (ParseSimpleTextureParameter(parameterNode, textureBuckets[FUDaeTextureChannel::REFRACTION]));
		}
		else if (parameterName == DAEMAX_DISPLACEMENT_MATERIAL_PARAMETER)
		{
			status &= (ParseSimpleTextureParameter(parameterNode, textureBuckets[FUDaeTextureChannel::DISPLACEMENT]));
		}
		else if (parameterName == DAEMAX_FILTERCOLOR_MATERIAL_PARAMETER)
		{
			status &= (ParseSimpleTextureParameter(parameterNode, textureBuckets[FUDaeTextureChannel::FILTER]));
		}
		else
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_MAT_PARAM_NAME, parameterNode->line);
		}
	}

	bool isEmptyBucketEmpty = emptyBucket.empty();
	if (!isEmptyBucketEmpty)
	{
		//return status.Fail(FS("Unexpected texture sampler on some parameters for material ") + TO_FSTRING(GetDaeId()), baseNode->line);
		FUError::Error(FUError::ERROR, FUError::ERROR_INVALID_TEXTURE_SAMPLER, baseNode->line);
	}

	// Although the default COLLADA materials gives, wrongly, a transparent material,
	// when neither the TRANSPARENT or TRANSPARENCY parameters are set, assume an opaque material.
	// Similarly for reflectivity
	if (!hasTranslucency)
	{
		translucencyColor = (transparencyMode == RGB_ZERO) ? FMVector4::Zero : FMVector4::One;
		translucencyFactor = (transparencyMode == RGB_ZERO) ? 0.0f : 1.0f;
	}
	if (!hasReflectivity)
	{
		reflectivityColor = FMVector4::Zero;
		reflectivityFactor = 0.0f;
	}

	// Convert some of the values that may appear in different formats
	if (!isEmissionFactor)
	{
		emissionFactor = (emissionColor.x + emissionColor.y + emissionColor.z) / 3.0f;
	}

	SetDirtyFlag();
	return status;
}

xmlNode* FCDEffectStandard::WriteToXML(xmlNode* parentNode) const
{
	// Call the parent to create the profile node and to export the parameters.
	xmlNode* profileNode = FCDEffectProfile::WriteToXML(parentNode);
	xmlNode* techniqueCommonNode = AddChild(profileNode, DAE_TECHNIQUE_ELEMENT);
	AddNodeSid(techniqueCommonNode, "common");

	const char* materialName;
	switch (type)
	{
	case CONSTANT: materialName = DAE_FXSTD_CONSTANT_ELEMENT; break;
	case LAMBERT: materialName = DAE_FXSTD_LAMBERT_ELEMENT; break;
	case PHONG: materialName = DAE_FXSTD_PHONG_ELEMENT; break;
	case BLINN: materialName = DAE_FXSTD_BLINN_ELEMENT; break;
	case UNKNOWN:
	default: materialName = DAEERR_UNKNOWN_ELEMENT; break;
	}
	xmlNode* materialNode = AddChild(techniqueCommonNode, materialName);
	xmlNode* techniqueNode = AddExtraTechniqueChild(techniqueCommonNode, DAE_FCOLLADA_PROFILE);

	// Export the color/float parameters
	FCDTextureTrackList emptyBucket; float emptyValue = 0.0f; FMVector4 emptyColor;
	if (!isEmissionFactor)
	{
		WriteColorTextureParameterToXML(materialNode, DAE_EMISSION_MATERIAL_PARAMETER, emissionColor, textureBuckets[FUDaeTextureChannel::EMISSION]);
	}
	if (type != CONSTANT)
	{
		WriteColorTextureParameterToXML(materialNode, DAE_AMBIENT_MATERIAL_PARAMETER, ambientColor, textureBuckets[FUDaeTextureChannel::AMBIENT]);
		WriteColorTextureParameterToXML(materialNode, DAE_DIFFUSE_MATERIAL_PARAMETER, diffuseColor, textureBuckets[FUDaeTextureChannel::DIFFUSE]);
		if (type != LAMBERT)
		{
			WriteColorTextureParameterToXML(materialNode, DAE_SPECULAR_MATERIAL_PARAMETER, specularColor, textureBuckets[FUDaeTextureChannel::SPECULAR]);
			WriteFloatTextureParameterToXML(materialNode, DAE_SHININESS_MATERIAL_PARAMETER, shininess, emptyBucket);
			if (!textureBuckets[FUDaeTextureChannel::SHININESS].empty())
			{
				WriteFloatTextureParameterToXML(techniqueNode, DAE_SHININESS_MATERIAL_PARAMETER, shininess, textureBuckets[FUDaeTextureChannel::SHININESS]);
			}
			if (specularFactor != 1.0f)
			{
				WriteFloatTextureParameterToXML(techniqueNode, DAEMAX_SPECLEVEL_MATERIAL_PARAMETER, specularFactor, textureBuckets[FUDaeTextureChannel::SPECULAR_LEVEL]);
			}
		}
	}

	// Export the reflectivity parameters, which belong to the common material.
	if (isReflective)
	{
		WriteColorTextureParameterToXML(materialNode, DAE_REFLECTIVE_MATERIAL_PARAMETER, reflectivityColor, textureBuckets[FUDaeTextureChannel::REFLECTION]);
		WriteFloatTextureParameterToXML(materialNode, DAE_REFLECTIVITY_MATERIAL_PARAMETER, reflectivityFactor, emptyBucket);
	}
	
	// Translucency includes both transparent and opacity textures
	FCDTextureTrackList translucencyBucket;
	translucencyBucket.insert(translucencyBucket.end(), textureBuckets[FUDaeTextureChannel::TRANSPARENT].begin(), textureBuckets[FUDaeTextureChannel::TRANSPARENT].end());
	translucencyBucket.insert(translucencyBucket.end(), textureBuckets[FUDaeTextureChannel::OPACITY].begin(), textureBuckets[FUDaeTextureChannel::OPACITY].end());
	xmlNode* transparentNode = WriteColorTextureParameterToXML(materialNode, DAE_TRANSPARENT_MATERIAL_PARAMETER, translucencyColor, translucencyBucket);
	AddAttribute(transparentNode, DAE_OPAQUE_MATERIAL_ATTRIBUTE, transparencyMode == RGB_ZERO ? DAE_RGB_ZERO_ELEMENT : DAE_A_ONE_ELEMENT);
	WriteFloatTextureParameterToXML(materialNode, DAE_TRANSPARENCY_MATERIAL_PARAMETER, translucencyFactor, emptyBucket);

	// The index of refraction parameter belongs to the common material.
	WriteFloatTextureParameterToXML(materialNode, DAE_INDEXOFREFRACTION_MATERIAL_PARAMETER, indexOfRefraction, emptyBucket);

	// Non-COLLADA parameters
	if (!textureBuckets[FUDaeTextureChannel::BUMP].empty())
	{
		WriteFloatTextureParameterToXML(techniqueNode, DAE_BUMP_MATERIAL_PARAMETER, emptyValue, textureBuckets[FUDaeTextureChannel::BUMP]);
	}
	if (isEmissionFactor)
	{
		WriteFloatTextureParameterToXML(techniqueNode, DAEMAX_EMISSIONLEVEL_MATERIAL_PARAMETER, emissionFactor, emptyBucket);
	}
	if (!textureBuckets[FUDaeTextureChannel::DISPLACEMENT].empty())
	{
		WriteFloatTextureParameterToXML(techniqueNode, DAEMAX_DISPLACEMENT_MATERIAL_PARAMETER, emptyValue, textureBuckets[FUDaeTextureChannel::DISPLACEMENT]);
	}
	if (!textureBuckets[FUDaeTextureChannel::FILTER].empty())
	{
		WriteColorTextureParameterToXML(techniqueNode, DAEMAX_FILTERCOLOR_MATERIAL_PARAMETER, emptyColor, textureBuckets[FUDaeTextureChannel::FILTER]);
	}
	if (!textureBuckets[FUDaeTextureChannel::REFRACTION].empty())
	{
		WriteColorTextureParameterToXML(techniqueNode, DAEMAX_INDEXOFREFRACTION_MATERIAL_PARAMETER, emptyColor, textureBuckets[FUDaeTextureChannel::REFRACTION]);
	}

	GetExtra()->LetWriteToXML(profileNode);
	return profileNode;
}

xmlNode* FCDEffectStandard::WriteColorTextureParameterToXML(xmlNode* parentNode, const char* parameterNodeName, const FMVector4& value, const FCDTextureTrackList& textureBucket) const
{
	xmlNode* parameterNode = AddChild(parentNode, parameterNodeName);
	if (WriteTextureParameterToXML(parameterNode, textureBucket) == NULL)
	{
		fm::string colorValue = FUStringConversion::ToString(value);
		xmlNode* valueNode = AddChild(parameterNode, DAE_FXSTD_COLOR_ELEMENT, colorValue);
		GetDocument()->WriteAnimatedValueToXML(&value.x, valueNode, parameterNodeName);
	}
	return parameterNode;
}


xmlNode* FCDEffectStandard::WriteFloatTextureParameterToXML(xmlNode* parentNode, const char* parameterNodeName, const float& value, const FCDTextureTrackList& textureBucket) const
{
	xmlNode* parameterNode = AddChild(parentNode, parameterNodeName);
	if (WriteTextureParameterToXML(parameterNode, textureBucket) == NULL)
	{
		xmlNode* valueNode = AddChild(parameterNode, DAE_FXSTD_FLOAT_ELEMENT, value);
		GetDocument()->WriteAnimatedValueToXML(&value, valueNode, parameterNodeName);
	}
	return parameterNode;
}

xmlNode* FCDEffectStandard::WriteTextureParameterToXML(xmlNode* parentNode, const FCDTextureTrackList& textureBucket) const
{
	xmlNode* textureNode = NULL;
	for (FCDTextureTrackList::const_iterator itT = textureBucket.begin(); itT != textureBucket.end(); ++itT)
	{
		xmlNode* newTextureNode = (*itT)->LetWriteToXML(parentNode);
		if (newTextureNode != NULL && textureNode == NULL) textureNode = newTextureNode;
	}
	return textureNode;
}

// Parse in the different standard effect parameters, bucketing the textures
bool FCDEffectStandard::ParseColorTextureParameter(xmlNode* parameterNode, FMVector4& value, FCDTextureContainer& textureBucket)
{
	bool status = true;

	// Look for <texture> elements, they pre-empt everything else
	size_t originalSize = textureBucket.size();
	ParseSimpleTextureParameter(parameterNode, textureBucket);
	if (originalSize < textureBucket.size()) { value = FMVector4(1.0f, 1.0f, 1.0f, 1.0f); return status; }

	// Try to find a <param> element
	xmlNode* paramNode = FindChildByType(parameterNode, DAE_PARAMETER_ELEMENT);
	if (paramNode != NULL)
	{
		fm::string name = ReadNodeProperty(paramNode, DAE_REF_ATTRIBUTE);
		xmlNode* effectNode = NULL, *tempNode = parameterNode;
		//look for the parameter up to the effect level
		while (effectNode == NULL && tempNode != NULL)
		{
			tempNode = tempNode->parent;
			if (IsEquivalent(tempNode->name, DAE_EFFECT_ELEMENT))
				effectNode = tempNode;
		}
		xmlNode* paramRefNode = NULL;
		if (effectNode)
			paramRefNode = FindHierarchyChildBySid(effectNode, name.c_str());


		if (paramRefNode == NULL)
		{
			FUError::Error(FUError::ERROR, FUError::ERROR_PARAM_NODE_MISSING, parameterNode->line);
		}
		else
		{
			// Parse the color value and allow for an animation of it
			xmlNode* valueNode = FindChildByType(paramRefNode, DAE_FXCMN_FLOAT4_ELEMENT);
			if (valueNode)
			{
				const char* content = ReadNodeContentDirect(valueNode);
				value = FUStringConversion::ToVector4(content);
			}
			else
			{
				valueNode = FindChildByType(paramRefNode, DAE_FXCMN_FLOAT3_ELEMENT);
				if (valueNode)
				{
					const char* content = ReadNodeContentDirect(valueNode);
					value = FUStringConversion::ToVector3(content);
					value.w = 1.0f;
				}
			}
			if (HasNodeProperty(paramRefNode, DAE_ID_ATTRIBUTE) || HasNodeProperty(paramRefNode, DAE_SID_ATTRIBUTE))
				FCDAnimatedColor::Create(GetDocument(), paramRefNode, &value);
			else
				FCDAnimatedColor::Create(GetDocument(), paramRefNode, &value);
		}
	}
	else
	{
		// Look for a <color> element
		xmlNode* colorNode = FindChildByType(parameterNode, DAE_FXSTD_COLOR_ELEMENT);
		const char* content = ReadNodeContentDirect(colorNode);

		// Parse the color value and allow for an animation of it
		value = FUStringConversion::ToVector4(content);
		if (HasNodeProperty(colorNode, DAE_ID_ATTRIBUTE) || HasNodeProperty(colorNode, DAE_SID_ATTRIBUTE))
			FCDAnimatedColor::Create(GetDocument(), colorNode, &value);
		else
			FCDAnimatedColor::Create(GetDocument(), parameterNode, &value);
	}

	return status;
}

bool FCDEffectStandard::ParseFloatTextureParameter(xmlNode* parameterNode, float& value, FCDTextureContainer& textureBucket)
{
	bool status = true;

	// Look for <texture> elements, they pre-empt everything else
	size_t originalSize = textureBucket.size();
	ParseSimpleTextureParameter(parameterNode, textureBucket);
	if (originalSize < textureBucket.size()) { value = 1.0f; return status; }

	// Next, look for a <float> element
	xmlNode* floatNode = FindChildByType(parameterNode, DAE_FXSTD_FLOAT_ELEMENT);
	const char* content = ReadNodeContentDirect(floatNode);

	// Parse the value and register it for an animation.
	value = FUStringConversion::ToFloat(content);
	if (HasNodeProperty(floatNode, DAE_ID_ATTRIBUTE) || HasNodeProperty(floatNode, DAE_SID_ATTRIBUTE))
		FCDAnimatedFloat::Create(GetDocument(), floatNode, &value);
	else
		FCDAnimatedFloat::Create(GetDocument(), parameterNode, &value);

	return status;
}

bool FCDEffectStandard::ParseSimpleTextureParameter(xmlNode* parameterNode, FCDTextureContainer& textureBucket)
{
	bool status = true; 

	// Parse in all the <texture> elements as standard effect samplers
	xmlNodeList samplerNodes;
	FindChildrenByType(parameterNode, DAE_FXSTD_TEXTURE_ELEMENT, samplerNodes);
	if (!samplerNodes.empty())
	{
		for (xmlNodeList::iterator itS = samplerNodes.begin(); itS != samplerNodes.end(); ++itS)
		{
			// Parse in the texture element and bucket them
			FCDTexture* texture = textureBucket.Add(GetDocument(), this);
			status &= (texture->LoadFromXML(*itS));
			if (!status) { SAFE_RELEASE(texture); }
		}
	}
	return status;
}
