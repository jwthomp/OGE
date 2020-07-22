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
#include "FCDocument/FCDEffectProfileFX.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDEffectParameterList.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDImage.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDEffect);

FCDEffect::FCDEffect(FCDocument* document) : FCDEntity(document, "Effect")
{
	parameters = new FCDEffectParameterList(GetDocument(), true);
}

FCDEffect::~FCDEffect()
{
	SAFE_RELEASE(parameters);
}

void FCDEffect::AddParameter(FCDEffectParameter* parameter)
{
	if (parameter != NULL)
	{
		parameters->AddParameter(parameter);
		SetDirtyFlag(); 
	}
}

// Flatten this effect: trickling down all the parameters to the technique level
void FCDEffect::Flatten()
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
			// Add this parameter to hierarchies below
			for (FCDEffectProfileContainer::iterator itR = profiles.begin(); itR != profiles.end(); ++itR)
			{
				if ((*itR)->GetType() != FUDaeProfileType::COMMON)
				{
					((FCDEffectProfileFX*) (*itR))->AddParameter((*itP)->Clone());
				}
			}
		}
	}
	while (parameters->GetParameterCount() > 0)
	{
		parameters->GetParameters().back()->Release();
	}

	for (FCDEffectProfileContainer::iterator itR = profiles.begin(); itR != profiles.end(); ++itR)
	{
		(*itR)->Flatten();
	}

	SetDirtyFlag(); 
}

// Search for a profile of the given type
const FCDEffectProfile* FCDEffect::FindProfile(FUDaeProfileType::Type type) const
{
	for (FCDEffectProfileContainer::const_iterator itR = profiles.begin(); itR != profiles.end(); ++itR)
	{
		if ((*itR)->GetType() == type) return (*itR);
	}
	return NULL;
}

// Search for a profile of a given type and platform
FCDEffectProfile* FCDEffect::FindProfileByTypeAndPlatform(FUDaeProfileType::Type type, const fm::string& platform)
{
	for (FCDEffectProfileContainer::iterator itR = profiles.begin(); itR != profiles.end(); ++itR)
	{
		if ((*itR)->GetType() == type) 
		{
			if (((FCDEffectProfileFX*)(*itR))->GetPlatform() == TO_FSTRING(platform)) return (*itR);
		}
	}
	return NULL;
}

const FCDEffectProfile* FCDEffect::FindProfileByTypeAndPlatform(FUDaeProfileType::Type type, const fm::string& platform) const
{
	for (FCDEffectProfileContainer::const_iterator itR = profiles.begin(); itR != profiles.end(); ++itR)
	{
		if ((*itR)->GetType() == type) 
		{
			if (((FCDEffectProfileFX*)(*itR))->GetPlatform() == TO_FSTRING(platform)) return (*itR);
		}
	}
	return NULL;
}

// Create a new effect profile.
FCDEffectProfile* FCDEffect::AddProfile(FUDaeProfileType::Type type)
{
	FCDEffectProfile* profile = NULL;

	// Create the correct profile for this type.
	if (type == FUDaeProfileType::COMMON) profile = new FCDEffectStandard(this);
	else profile = new FCDEffectProfileFX(this, type);

	profiles.push_back(profile);
	SetDirtyFlag(); 
	return profile;
}

// Look for the effect parameter with the correct semantic, in order to bind/set its value
FCDEffectParameter* FCDEffect::FindParameterBySemantic(const fm::string& semantic)
{
	FCDEffectParameter* p = parameters->FindSemantic(semantic);
	for (FCDEffectProfileContainer::iterator itR = profiles.begin(); itR != profiles.end() && p == NULL; ++itR)
	{
		p = (*itR)->FindParameterBySemantic(semantic);
	}
	return p;
}

void FCDEffect::FindParametersBySemantic(const fm::string& semantic, FCDEffectParameterList& _parameters)
{
	parameters->FindSemantic(semantic, _parameters);
	for (FCDEffectProfileContainer::iterator itR = profiles.begin(); itR != profiles.end(); ++itR)
	{
		(*itR)->FindParametersBySemantic(semantic, _parameters);
	}
}

void FCDEffect::FindParametersByReference(const fm::string& reference, FCDEffectParameterList& _parameters)
{
	parameters->FindReference(reference, _parameters);
	for (FCDEffectProfileContainer::iterator itR = profiles.begin(); itR != profiles.end(); ++itR)
	{
		(*itR)->FindParametersBySemantic(reference, _parameters);
	}
}

// Parse COLLADA document's <effect> element
bool FCDEffect::LoadFromXML(xmlNode* effectNode)
{
	bool status = FCDEntity::LoadFromXML(effectNode);
	if (!status) return status;

	while (parameters->GetParameterCount() > 0)
	{
		parameters->GetParameters().back()->Release();
	}

	// Accept solely <effect> elements at this point.
	if (!IsEquivalent(effectNode->name, DAE_EFFECT_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_EFFECT_ELEMENT, effectNode->line);
	}

	for (xmlNode* child = effectNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_IMAGE_ELEMENT))
		{
			FCDImage* image = GetDocument()->GetImageLibrary()->AddEntity();
			status &= (image->LoadFromXML(child));
		}
		else if (IsEquivalent(child->name, DAE_FXCMN_SETPARAM_ELEMENT) || IsEquivalent(child->name, DAE_FXCMN_NEWPARAM_ELEMENT))
		{
			AddParameter(FCDEffectParameterFactory::LoadFromXML(GetDocument(), child, &status));
		}
		else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT)) {} // processed by FCDEntity.
		else
		{
			// Check for a valid profile element.
			FUDaeProfileType::Type type = FUDaeProfileType::FromString((const char*) child->name);
			if (type != FUDaeProfileType::UNKNOWN)
			{
				FCDEffectProfile* profile = AddProfile(type);
				status &= (profile->LoadFromXML(child));
			}
			else
			{
				FUError::Error(FUError::WARNING, FUError::WARNING_UNSUPPORTED_PROFILE, child->line);
			}
		}
	}

	SetDirtyFlag(); 
	return status;
}

// Links the effect and its parameters.
void FCDEffect::Link()
{
	// Link up the parameters
	for (FCDEffectParameterTrackList::iterator itP = parameters->begin(); itP != parameters->end(); ++itP)
	{
		(*itP)->Link(*parameters);
	}

	// Link up the profiles and their parameters/textures/images
	for (FCDEffectProfileContainer::iterator itR = profiles.begin(); itR != profiles.end(); ++itR)
	{
		(*itR)->Link();
	}
}

// Returns a copy of the effect, with all the animations/textures attached
FCDEntity* FCDEffect::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDEffect* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDEffect(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDEffect::GetClassType())) clone = (FCDEffect*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		// Clone the effect profiles
		for (FCDEffectProfileContainer::const_iterator itR = profiles.begin(); itR != profiles.end(); ++itR)
		{
			FCDEffectProfile* clonedProfile = clone->AddProfile((*itR)->GetType());
			(*itR)->Clone(clonedProfile);
		}

		// Clone the effect parameters
		parameters->Clone(clone->parameters);
	}
	return _clone;
}

// Write out the <material> element to the COLLADA XML tree
xmlNode* FCDEffect::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* effectNode = WriteToEntityXML(parentNode, DAE_EFFECT_ELEMENT);

	// Write out the parameters
	for (FCDEffectParameterTrackList::iterator itP = parameters->begin(); itP != parameters->end(); ++itP)
	{
		(*itP)->LetWriteToXML(effectNode);
	}

	// Write out the profiles
	for (FCDEffectProfileContainer::const_iterator itR = profiles.begin(); itR != profiles.end(); ++itR)
	{
		(*itR)->LetWriteToXML(effectNode);
	}

	FCDEffect::WriteToExtraXML(effectNode);
	return effectNode;
}
