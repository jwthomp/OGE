/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDEffectTechnique.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEffectPass.h"
#include "FCDocument/FCDEffectPassShader.h"
#include "FCDocument/FCDEffectPassState.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterList.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDEffectPass);

FCDEffectPass::FCDEffectPass(FCDEffectTechnique *_parent)
:	FCDObject(_parent->GetDocument()), parent(_parent)
{
}

FCDEffectPass::~FCDEffectPass()
{
	parent = NULL;
}

// Adds a new shader to the pass.
FCDEffectPassShader* FCDEffectPass::AddShader()
{
	FCDEffectPassShader* shader = new FCDEffectPassShader(this);
	shaders.push_back(shader);
	SetDirtyFlag();
	return shader;
}

// Releases a shader contained within the pass.
void FCDEffectPass::ReleaseShader(FCDEffectPassShader* shader)
{
	SetDirtyFlag();
	shaders.release(shader);
}

// Adds a new vertex shader to the pass.
// If a vertex shader already exists within the pass, it will be released.
FCDEffectPassShader* FCDEffectPass::AddVertexShader()
{
	FCDEffectPassShader* vertexShader;
	for (vertexShader = GetVertexShader(); vertexShader != NULL; vertexShader = GetVertexShader())
	{
		ReleaseShader(vertexShader);
	}

	vertexShader = AddShader();
	vertexShader->AffectsVertices();
	SetDirtyFlag();
	return vertexShader;
}

// Adds a new fragment shader to the pass.
// If a fragment shader already exists within the pass, it will be released.
FCDEffectPassShader* FCDEffectPass::AddFragmentShader()
{
	FCDEffectPassShader* fragmentShader;
	for (fragmentShader = GetFragmentShader(); fragmentShader != NULL; fragmentShader = GetFragmentShader())
	{
		ReleaseShader(fragmentShader);
	}

	fragmentShader = AddShader();
	fragmentShader->AffectsFragments();
	SetDirtyFlag();
	return fragmentShader;
}

FCDEffectPass* FCDEffectPass::Clone(FCDEffectPass* clone) const
{
	if (clone == NULL) clone = new FCDEffectPass(parent);

	clone->name = name;

	// Clone the shaders
	clone->shaders.reserve(shaders.size());
	for (FCDEffectPassShaderContainer::const_iterator itS = shaders.begin(); itS != shaders.end(); ++itS)
	{
		FCDEffectPassShader* clonedShader = clone->AddShader();
		(*itS)->Clone(clonedShader);
	}

	// Clone the states
	clone->states.reserve(states.size());
	for (FCDEffectPassStateContainer::const_iterator itS = states.begin(); itS != states.end(); ++itS)
	{
		FCDEffectPassState* clonedState = clone->AddRenderState((*itS)->GetType());
		(*itS)->Clone(clonedState);
	}

	return clone;
}

const fm::string& FCDEffectPass::GetDaeId() const
{
	return parent->GetDaeId();
}

// Retrieve the type-specific shaders
const FCDEffectPassShader* FCDEffectPass::GetVertexShader() const
{
	for (FCDEffectPassShaderContainer::const_iterator itS = shaders.begin(); itS != shaders.end(); ++itS)
	{
		if ((*itS)->IsVertexShader()) return (*itS);
	}
	return NULL;
}

const FCDEffectPassShader* FCDEffectPass::GetFragmentShader() const
{
	for (FCDEffectPassShaderContainer::const_iterator itS = shaders.begin(); itS != shaders.end(); ++itS)
	{
		if ((*itS)->IsFragmentShader()) return (*itS);
	}
	return NULL;
}

FCDEffectPassState* FCDEffectPass::AddRenderState(FUDaePassState::State type)
{
	FCDEffectPassState* state = new FCDEffectPassState(GetDocument(), type);

	// Ordered-insertion into the states container.
	FCDEffectPassStateContainer::iterator itS;
	for (itS = states.begin(); itS != states.end(); ++itS)
	{
		if ((*itS)->GetType() < type) continue;
	}
	states.insert(itS, state);

	SetDirtyFlag();
	return state;
}

const FCDEffectPassState* FCDEffectPass::FindRenderState(FUDaePassState::State type) const
{
	for (FCDEffectPassStateContainer::const_iterator itS = states.begin(); itS != states.end(); ++itS)
	{
		if ((*itS)->GetType() == type) return (*itS);
	}
	return NULL;
}

bool FCDEffectPass::LoadFromXML(xmlNode* passNode)
{
	bool status = true;
	if (!IsEquivalent(passNode->name, DAE_PASS_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_PASS_ELEMENT, passNode->line);
		return status;
	}
	name = TO_FSTRING(ReadNodeProperty(passNode, DAE_SID_ATTRIBUTE));

	// Iterate over the pass nodes, looking for render states and <shader> elements, in any order.
	for (xmlNode* child = passNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;
		
		// Check for a render state type.
		FUDaePassState::State stateType = FUDaePassState::FromString((const char*) child->name);
		if (stateType != FUDaePassState::INVALID)
		{
			FCDEffectPassState* state = AddRenderState(stateType);
			status &= state->LoadFromXML(child);
		}

		// Look for the <shader> element.
		else if (IsEquivalent(child->name, DAE_SHADER_ELEMENT))
		{
			FCDEffectPassShader* shader = AddShader();
			status &= shader->LoadFromXML(child);
		}
	}

	SetDirtyFlag();
	return status;
}

// Write out the pass to the COLLADA XML node tree
xmlNode* FCDEffectPass::WriteToXML(xmlNode* parentNode) const
{
	// Write out the <pass> element, with the shader's name
	xmlNode* passNode = AddChild(parentNode, DAE_PASS_ELEMENT);
	if (!name.empty())
	{
		fstring& _name = const_cast<fstring&>(name);
		AddNodeSid(passNode, _name);
	}

	// Write out the render states
	for (FCDEffectPassStateContainer::const_iterator itS = states.begin(); itS != states.end(); ++itS)
	{
		(*itS)->LetWriteToXML(passNode);
	}

	// Write out the shaders
	for (FCDEffectPassShaderContainer::const_iterator itS = shaders.begin(); itS != shaders.end(); ++itS)
	{
		(*itS)->LetWriteToXML(passNode);
	}

	return passNode;
}
