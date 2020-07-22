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
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDEffectParameterList.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDMaterial.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUUniqueStringMap.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDMaterial);

FCDMaterial::FCDMaterial(FCDocument* document)
:	FCDEntity(document, "VisualMaterial")
{
	parameters = new FCDEffectParameterList(GetDocument(), true);
	ownsEffect = false;
}

FCDMaterial::~FCDMaterial()
{
	if (ownsEffect)
	{
		SAFE_RELEASE(effect);
	}
	SAFE_RELEASE(parameters);
	techniqueHints.clear();
}

// Cloning
FCDEntity* FCDMaterial::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDMaterial* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDMaterial(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDMaterial::GetClassType())) clone = (FCDMaterial*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		// Clone the effect and the local list of parameters
		if (effect != NULL)
		{
			if (cloneChildren)
			{
				clone->ownsEffect = true;
				FCDEffect* clonedEffect = clone->GetDocument()->GetEffectLibrary()->AddEntity();
				effect->Clone(clonedEffect, cloneChildren);
			}
			else
			{
				clone->SetEffect(const_cast<FCDEffect*>(&*effect));
			}
		}
		parameters->Clone(clone->parameters);
	}

	return _clone;
}

// Flatten the material: remove all the modifier parameters from the parameter list, permanently modifying their base parameter
void FCDMaterial::Flatten()
{
	for (FCDEffectParameterTrackList::iterator itP = parameters->begin(); itP != parameters->end(); ++itP)
	{
		FCDEffectParameterList generators;
		if ((*itP)->IsModifier())
		{
			// Overwrite the generators
			FindParametersByReference((*itP)->GetReference(), generators);
			for (FCDEffectParameterTrackList::iterator itQ = generators.begin(); itQ != generators.end(); ++itQ)
			{
				if ((*itP) != (*itQ))
				{
					(*itP)->Overwrite(*itQ);
				}
			}
		}
		else
		{
			// Add this parameter to hierarchy below
			if (effect != NULL) effect->AddParameter((*itP)->Clone());
		}
	}
	while (parameters->GetParameterCount() > 0)
	{
		parameters->GetParameters().back()->Release();
	}

	if (effect != NULL) effect->Flatten();
	SetDirtyFlag(); 
}

void FCDMaterial::AddParameter(FCDEffectParameter* parameter)
{
	if (parameter != NULL)
	{
		parameters->AddParameter(parameter);
		SetDirtyFlag(); 
	}
}

// Look for the effect parameter with the correct semantic, in order to bind/set its value
FCDEffectParameter* FCDMaterial::FindParameterBySemantic(const fm::string& semantic)
{
	return (effect != NULL) ? effect->FindParameterBySemantic(semantic) : NULL;
}

void FCDMaterial::FindParametersBySemantic(const fm::string& semantic, FCDEffectParameterList& _parameters)
{
	parameters->FindSemantic(semantic, _parameters);
	if (effect != NULL) effect->FindParametersBySemantic(semantic, _parameters);
}

void FCDMaterial::FindParametersByReference(const fm::string& reference, FCDEffectParameterList& _parameters)
{
	parameters->FindReference(reference, _parameters);
	if (effect != NULL) effect->FindParametersByReference(reference, _parameters);
}

// Parse COLLADA document's <material> element
bool FCDMaterial::LoadFromXML(xmlNode* materialNode)
{
	while (parameters->GetParameterCount() > 0)
	{
		parameters->GetParameters().back()->Release();
	}

	bool status = FCDEntity::LoadFromXML(materialNode);
	if (!status) return status;
	if (!IsEquivalent(materialNode->name, DAE_MATERIAL_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_MAT_LIB_ELEMENT, materialNode->line);
		return status;
	}

	// Read in the effect pointer node
	xmlNode* effectNode = FindChildByType(materialNode, DAE_INSTANCE_EFFECT_ELEMENT);
	if (effectNode == NULL)
	{
		FUError::Error(FUError::WARNING, FUError::ERROR_MISSING_ELEMENT, materialNode->line);
	}

	FUUri url = ReadNodeUrl(effectNode);
	if (!url.prefix.empty())
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNSUPPORTED_REF_EFFECTS, effectNode->line);
		return status;
	}
	else if (url.suffix.empty())
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_EMPTY_INSTANCE_EFFECT, effectNode->line);
		return status;
	}
	effect = GetDocument()->FindEffect(url.suffix);

	// Read in the parameter modifications
	for (xmlNode* child = effectNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;
		
		if (IsEquivalent(child->name, DAE_FXCMN_SETPARAM_ELEMENT))
		{
			AddParameter(FCDEffectParameterFactory::LoadFromXML(GetDocument(), child, &status));
		}
		else if (IsEquivalent(child->name, DAE_FXCMN_HINT_ELEMENT))
		{
			FCDMaterialTechniqueHint& hint = *(techniqueHints.insert(techniqueHints.end(), FCDMaterialTechniqueHint()));
			hint.platform = TO_FSTRING(ReadNodeProperty(child, DAE_PLATFORM_ATTRIBUTE));
			hint.technique = ReadNodeProperty(child, DAE_REF_ATTRIBUTE);
		}
	}

	if (effect == NULL)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_EFFECT_MISSING, materialNode->line);
		return status;
	}
	
	SetDirtyFlag(); 
	return status;
}

// Links the material and its parameters.
void FCDMaterial::Link()
{
	for (FCDEffectParameterTrackList::iterator itP = parameters->begin(); itP != parameters->end(); ++itP)
	{
		(*itP)->Link(*parameters);
	}
}

// Write out the <material> element to the COLLADA XML tree
xmlNode* FCDMaterial::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* materialNode = WriteToEntityXML(parentNode, DAE_MATERIAL_ELEMENT);

	// The <instance_effect> element is required in COLLADA 1.4
	xmlNode* instanceEffectNode = AddChild(materialNode, DAE_INSTANCE_EFFECT_ELEMENT);
	if (effect != NULL)
	{
		AddAttribute(instanceEffectNode, DAE_URL_ATTRIBUTE, fm::string("#") + effect->GetDaeId());

		// Write out the technique hints
		for (FCDMaterialTechniqueHintList::const_iterator itH = techniqueHints.begin(); itH != techniqueHints.end(); ++itH)
		{
			xmlNode* hintNode = AddChild(instanceEffectNode, DAE_FXCMN_HINT_ELEMENT);
			AddAttribute(hintNode, DAE_PLATFORM_ATTRIBUTE, (*itH).platform);
			AddAttribute(hintNode, DAE_REF_ATTRIBUTE, (*itH).technique);
		}

		// Write out the parameters
		for (FCDEffectParameterTrackList::iterator itP = parameters->begin(); itP != parameters->end(); ++itP)
		{
			(*itP)->LetWriteToXML(instanceEffectNode);
		}
	}
	else
	{
		AddAttribute(instanceEffectNode, DAE_URL_ATTRIBUTE, fm::string("#"));
	}

	FCDEntity::WriteToExtraXML(materialNode);
	return materialNode;
}

