/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectCode.h"
#include "FCDocument/FCDEffectPass.h"
#include "FCDocument/FCDEffectProfileFX.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDEffectParameterList.h"
#include "FCDocument/FCDEffectTechnique.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDImage.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDEffectTechnique);

FCDEffectTechnique::FCDEffectTechnique(FCDEffectProfileFX *_parent)
:	FCDObject(_parent->GetDocument()), parent(_parent)
{
	parameters = new FCDEffectParameterList(GetDocument(), true);;
}

FCDEffectTechnique::~FCDEffectTechnique()
{
	SAFE_RELEASE(parameters);
	parent = NULL;
}

// Adds a new pass to this effect technique.
FCDEffectPass* FCDEffectTechnique::AddPass()
{
	FCDEffectPass* pass = passes.Add(this);
	SetDirtyFlag();
	return pass;
}

// Adds a new code inclusion to this effect profile.
FCDEffectCode* FCDEffectTechnique::AddCode()
{
	FCDEffectCode* code = codes.Add(GetDocument());
	SetDirtyFlag();
	return code;
}

FCDEffectTechnique* FCDEffectTechnique::Clone(FCDEffectTechnique* clone) const
{
	if (clone == NULL) clone = new FCDEffectTechnique(NULL);

	clone->name = name;
	parameters->Clone(clone->parameters);

	// Clone the codes: need to happen before the passes are cloned
	clone->codes.reserve(codes.size());
	for (FCDEffectCodeContainer::const_iterator itC = codes.begin(); itC != codes.end(); ++itC)
	{
		(*itC)->Clone(clone->AddCode());
	}

	// Clone the passes
	clone->passes.reserve(passes.size());
	for (FCDEffectPassContainer::const_iterator itP = passes.begin(); itP != passes.end(); ++itP)
	{
		(*itP)->Clone(clone->AddPass());
	}

	return clone;
}

const fm::string& FCDEffectTechnique::GetDaeId() const
{
	return parent->GetDaeId();
}

void FCDEffectTechnique::AddParameter(FCDEffectParameter* parameter)
{
	parameters->AddParameter(parameter);
	SetDirtyFlag();
}

// Flatten this effect technique: merge the parameter modifiers and generators
void FCDEffectTechnique::Flatten()
{
	for (FCDEffectParameterTrackList::iterator itP = parameters->begin(); itP != parameters->end();)
	{
		FCDEffectParameterList generators(GetDocument());
		if ((*itP)->IsModifier())
		{
			// Overwrite the generators
			FindParametersByReference((*itP)->GetReference(), generators);
			for (FCDEffectParameterTrackList::iterator itQ = generators.begin(); itQ != generators.end(); ++itQ)
			{
				if ((*itQ)->IsGenerator())
				{
					(*itP)->Overwrite(*itQ);
				}
			}
			(*itP)->Release();
		}
		else
		{
			++itP;
		}
	}
	
	SetDirtyFlag();
}

bool FCDEffectTechnique::LoadFromXML(xmlNode* techniqueNode)
{
	bool status = true;
	if (!IsEquivalent(techniqueNode->name, DAE_TECHNIQUE_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_TECHNIQUE_ELEMENT, techniqueNode->line);
		return status;
	}
	
	fm::string techniqueName = ReadNodeProperty(techniqueNode, DAE_SID_ATTRIBUTE);
	name = TO_FSTRING(techniqueName);
	
	// Look for the pass and parameter elements
	SAFE_RELEASE(parameters);
	parameters = new FCDEffectParameterList(GetDocument(), true);
	for (xmlNode* child = techniqueNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_PASS_ELEMENT))
		{
			FCDEffectPass* pass = AddPass();
			status &= (pass->LoadFromXML(child));
		}
		else if (IsEquivalent(child->name, DAE_FXCMN_NEWPARAM_ELEMENT) || IsEquivalent(child->name, DAE_FXCMN_SETPARAM_ELEMENT))
		{
			AddParameter(FCDEffectParameterFactory::LoadFromXML(GetDocument(), child, &status));
		}
		else if (IsEquivalent(child->name, DAE_FXCMN_CODE_ELEMENT) || IsEquivalent(child->name, DAE_FXCMN_INCLUDE_ELEMENT))
		{
			FCDEffectCode* code = new FCDEffectCode(GetDocument());
			codes.push_back(code);
			status &= (code->LoadFromXML(child));
		}
		else if (IsEquivalent(child->name, DAE_IMAGE_ELEMENT))
		{
			FCDImage* image = GetDocument()->GetImageLibrary()->AddEntity();
			status &= (image->LoadFromXML(child));
		}
	}
	
	SetDirtyFlag();
	return status;
}

// Links the effect and its parameters.
void FCDEffectTechnique::Link()
{
	// Make up a list of all the parameters available for linkage.
	FCDEffectParameterList availableParameters;
	if (GetParent() != NULL)
	{
		if (GetParent()->GetParent() != NULL)
		{
			availableParameters.GetParameters().insert(availableParameters.end(), GetParent()->GetParent()->GetParameters()->begin(), GetParent()->GetParent()->GetParameters()->end());
		}
		availableParameters.GetParameters().insert(availableParameters.end(), GetParent()->GetParameters()->begin(), GetParent()->GetParameters()->end());
	}
	availableParameters.GetParameters().insert(availableParameters.end(), parameters->begin(), parameters->end());

	for (FCDEffectParameterTrackList::iterator itP = parameters->begin(); itP != parameters->end(); ++itP)
	{
		(*itP)->Link(availableParameters);
	}
}

// Write out the effect techniques to the COLLADA XML node tree
xmlNode* FCDEffectTechnique::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* techniqueNode = AddChild(parentNode, DAE_TECHNIQUE_ELEMENT);
	fstring& _name = const_cast<fstring&>(name);
	if (_name.empty()) _name = FC("common");
	AddNodeSid(techniqueNode, _name);

	// Write out the code/includes
	for (FCDEffectCodeContainer::const_iterator itC = codes.begin(); itC != codes.end(); ++itC)
	{
		(*itC)->LetWriteToXML(techniqueNode);
	}

	// Write out the effect parameters at this level
	for (FCDEffectParameterTrackList::iterator itP = parameters->begin(); itP != parameters->end(); ++itP)
	{
		(*itP)->LetWriteToXML(techniqueNode);
	}

	// Write out the passes.
	// In COLLADA 1.4: there should always be at least one pass.
	if (!passes.empty())
	{
		for (FCDEffectPassContainer::const_iterator itP = passes.begin(); itP != passes.end(); ++itP)
		{
			(*itP)->LetWriteToXML(techniqueNode);
		}
	}
	else
	{
		UNUSED(xmlNode* dummyPassNode =) AddChild(techniqueNode, DAE_PASS_ELEMENT);
	}

	return techniqueNode;
}

// Look for the parameter with the given reference.
const FCDEffectParameter* FCDEffectTechnique::FindParameter(const char* ref) const
{
	return parameters->FindReference(ref);
}

// Look for the effect parameter with the correct semantic, in order to bind/set its value
FCDEffectParameter* FCDEffectTechnique::FindParameterBySemantic(const fm::string& semantic)
{
	return parameters->FindSemantic(semantic);
}

void FCDEffectTechnique::FindParametersBySemantic(const fm::string& semantic, FCDEffectParameterList& _parameters)
{
	for (FCDEffectParameterTrackList::iterator it = parameters->begin(); it != parameters->end(); ++it)
	{
		if ((*it)->GetSemantic() == semantic) _parameters.AddParameter(*it);
	}
}

void FCDEffectTechnique::FindParametersByReference(const fm::string& reference, FCDEffectParameterList& _parameters)
{
	for (FCDEffectParameterTrackList::iterator it = parameters->begin(); it != parameters->end(); ++it)
	{
		if ((*it)->GetReference() == reference) _parameters.AddParameter(*it);
	}
}

FCDEffectCode* FCDEffectTechnique::FindCode(const fm::string& sid)
{
	for (FCDEffectCodeContainer::iterator itC = codes.begin(); itC != codes.end(); ++itC)
	{
		if ((*itC)->GetSubId() == sid) return (*itC);
	}
	return NULL;
}
const FCDEffectCode* FCDEffectTechnique::FindCode(const fm::string& sid) const
{
	for (FCDEffectCodeContainer::const_iterator itC = codes.begin(); itC != codes.end(); ++itC)
	{
		if ((*itC)->GetSubId() == sid) return (*itC);
	}
	return NULL;
}
