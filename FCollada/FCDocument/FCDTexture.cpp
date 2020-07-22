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
#include "FCDocument/FCDEffectParameterList.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDEffectParameterSurface.h"
#include "FCDocument/FCDEffectParameterSampler.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDImage.h"
#include "FCDocument/FCDTexture.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDTexture
//

ImplementObjectType(FCDTexture);

FCDTexture::FCDTexture(FCDocument* document, FCDEffectStandard* _parent)
:	FCDObject(document), parent(_parent)
,	sampler(NULL)
{
	set = new FCDEffectParameterInt(document);
	extra = new FCDExtra(document, this);
	set->SetValue(-1);
}

FCDTexture::~FCDTexture()
{
	parent = NULL;
}

// Retrieves the sampler parameter: creates one if none are attached.
FCDEffectParameterSampler* FCDTexture::GetSampler()
{
	if (parent == NULL && sampler == NULL) return NULL;
	if (sampler == NULL)
	{
		sampler = new FCDEffectParameterSampler(GetDocument());
		parent->AddParameter(sampler);
	}
	return sampler;
}

// Retrieves the image information for this texture.
const FCDImage* FCDTexture::GetImage() const
{
	if (sampler == NULL) return NULL;
	if (sampler->GetSurface() == NULL) return NULL;
	const FCDEffectParameterSurface* surface = sampler->GetSurface();
	return surface->GetImage();
}

// Set the image information for this texture.
void FCDTexture::SetImage(FCDImage* image)
{
	// TODO: No parameter re-use for now.
	SAFE_RELEASE(sampler);
	if (image != NULL && parent != NULL)
	{
		// Look for a surface with the expected sid.
		fm::string surfaceSid = image->GetDaeId() + "-surface";
        FCDEffectParameter* _surface = const_cast<FCDEffectParameter*>(parent->FindParameterByReference(surfaceSid.c_str()));
		FCDEffectParameterSurface* surface = NULL;
		if (_surface == NULL)
		{
			// Create the surface parameter
			surface = new FCDEffectParameterSurface(GetDocument());
			surface->SetInitMethod(new FCDEffectParameterSurfaceInitFrom());
			surface->AddImage(image);
			surface->SetGenerator();
			surface->SetReference(surfaceSid);
			parent->AddParameter(surface);
		}
		else if (_surface->GetObjectType().Includes(FCDEffectParameterSurface::GetClassType()))
		{
			surface = (FCDEffectParameterSurface*) _surface;
		}
		else return;

		// Look for a sampler with the expected sid.
		fm::string samplerSid = image->GetDaeId() + "-sampler";
        const FCDEffectParameter* _sampler = parent->FindParameterByReference(samplerSid.c_str());
		if (_sampler == NULL)
		{
			sampler = new FCDEffectParameterSampler(GetDocument());
			sampler->SetSurface(surface);
			sampler->SetGenerator();
			sampler->SetReference(samplerSid);
			parent->AddParameter(sampler);
		}
		else if (_sampler->GetObjectType().Includes(FCDEffectParameterSampler::GetClassType()))
		{
			sampler = (FCDEffectParameterSampler*) const_cast<FCDEffectParameter*>(_sampler);
		}
	}

	SetDirtyFlag(); 
}

// Look for the effect parameter with the correct semantic, in order to bind/set its value
FCDEffectParameter* FCDTexture::FindParameterBySemantic(const fm::string& semantic)
{
	if (set->GetSemantic() == semantic) return set;
	else return NULL;
}

void FCDTexture::FindParametersBySemantic(const fm::string& semantic, FCDEffectParameterList& parameters)
{
	if (set->GetSemantic() == semantic) parameters.AddParameter(set);
}

void FCDTexture::FindParametersByReference(const fm::string& reference, FCDEffectParameterList& parameters)
{
	if (set->GetReference() == reference) parameters.AddParameter(set);
}

// Returns a copy of the texture/sampler, with all the animations attached
FCDTexture* FCDTexture::Clone(FCDTexture* clone) const
{
	if (clone == NULL) clone = new FCDTexture(const_cast<FCDTexture*>(this)->GetDocument(), parent);

	set->Clone(clone->set);
	extra->Clone(clone->extra);

	if (sampler != NULL)
	{
		sampler->Clone(clone->GetSampler());
	}
	clone->samplerSid = samplerSid; // We do clone these at import-time, before the sampler gets assigned.

	return clone;
}

// Read in the texture element for a standard effect COLLADA texture
bool FCDTexture::LoadFromXML(xmlNode* textureNode)
{
	bool status = true;

	// Verify that this is a sampler node
	if (!IsEquivalent(textureNode->name, DAE_FXSTD_TEXTURE_ELEMENT))
	{
		//return status.Fail(FS("Unknown texture sampler element."), textureNode->line);
		FUError::Error(FUError::ERROR, FUError::ERROR_UNKNOWN_TEXTURE_SAMPLER, textureNode->line);
	}
	
	// Read in the 'texture' attribute: points to an image(early 1.4.0) or a sampler(late 1.4.0)
	// Will be resolved at link-time.
	samplerSid = ReadNodeProperty(textureNode, DAE_FXSTD_TEXTURE_ATTRIBUTE);

	// Read in the 'texcoord' attribute: a texture coordinate set identifier
	fm::string semantic = ReadNodeProperty(textureNode, DAE_FXSTD_TEXTURESET_ATTRIBUTE);
	if (!semantic.empty())
	{
		set->SetSemantic(semantic);

		// [GLaforte 06-01-2006] Also attempt to convert the value to a signed integer
		// since that was done quite a bit in COLLADA 1.4 preview exporters.
		set->SetValue(FUStringConversion::ToInt32(semantic));
	}

	// Parse in the extra trees
	xmlNodeList extraNodes;
	FindChildrenByType(textureNode, DAE_EXTRA_ELEMENT, extraNodes);
	for (xmlNodeList::iterator itX = extraNodes.begin(); itX != extraNodes.end(); ++itX)
	{
		status &= (extra->LoadFromXML(*itX));
	}
	SetDirtyFlag(); 
	return status;
}

void FCDTexture::Link(FCDEffectParameterList& parameters)
{
	if (!samplerSid.empty())
	{
		// Check for the sampler parameter in the parent profile and the effect.
		if (parent != NULL)
		{
			FCDEffectParameter* p = parameters.FindReference(samplerSid);
			if (p != NULL && p->GetObjectType() == FCDEffectParameterSampler::GetClassType())
			{
				sampler = (FCDEffectParameterSampler*) p;
			}
		}

		if (sampler == NULL)
		{
			// Early COLLADA 1.4.0 backward compatibility: Also look for an image with this id.
            FCDImage* image = GetDocument()->FindImage(samplerSid);
			SetImage(image);
			SetDirtyFlag();
		}
		samplerSid.clear();
	}
}

// Write out the texture to the COLLADA XML node tree
xmlNode* FCDTexture::WriteToXML(xmlNode* parentNode) const
{
	// Create the <texture> element
	xmlNode* textureNode = AddChild(parentNode, DAE_TEXTURE_ELEMENT);
	AddAttribute(textureNode, DAE_FXSTD_TEXTURE_ATTRIBUTE, (sampler != NULL) ? sampler->GetReference() : "");
	AddAttribute(textureNode, DAE_FXSTD_TEXTURESET_ATTRIBUTE, (set != NULL) ? set->GetSemantic() : "");
	extra->LetWriteToXML(textureNode);
	return textureNode;
}
