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
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FColladaPlugin.h"
namespace FCollada { extern FColladaPluginManager* pluginManager; }
using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDExtra
//

ImplementObjectType(FCDExtra);

FCDExtra::FCDExtra(FCDocument* document, FUObject* parent)
:	FCDObject(document), parent(parent)
{
	// Create the default extra type.
	types.push_back(new FCDEType(document, this, ""));
}

FCDExtra::~FCDExtra()
{
}

// Adds a type of the given name (or return the existing type with this name).
FCDEType* FCDExtra::AddType(const char* name)
{
	FCDEType* type = FindType(name);
	if (type == NULL)
	{
		type = types.Add(GetDocument(), this, name);
		SetDirtyFlag();
	}
	return type;
}

// Search for a profile-specific type
const FCDEType* FCDExtra::FindType(const char* name) const
{
	for (FCDETypeContainer::const_iterator itT = types.begin(); itT != types.end(); ++itT)
	{
		if (IsEquivalent((*itT)->GetName(), name)) return *itT;
	}
	return NULL;
}

FCDExtra* FCDExtra::Clone(FCDExtra* clone) const
{
	if (clone == NULL) clone = new FCDExtra(const_cast<FCDocument*>(GetDocument()), NULL);

	// Create all the types
	clone->types.reserve(types.size());
	for (FCDETypeContainer::const_iterator itT = types.begin(); itT != types.end(); ++itT)
	{
		FCDEType* cloneT = clone->AddType((*itT)->GetName());
		(*itT)->Clone(cloneT);
	}
	return clone;
}

// Read in/Write to a COLLADA XML document
bool FCDExtra::LoadFromXML(xmlNode* extraNode)
{
	bool status = true;

	// Do NOT assume that we have an <extra> element: we may be parsing a type switch instead.
	FCDEType* parsingType = NULL;
	if (IsEquivalent(extraNode->name, DAE_EXTRA_ELEMENT))
	{
		parsingType = AddType(ReadNodeProperty(extraNode, DAE_TYPE_ATTRIBUTE));
	}
	if (parsingType == NULL) parsingType = GetDefaultType();
	parsingType->LoadFromXML(extraNode);

	SetDirtyFlag();
	return status;
}

xmlNode* FCDExtra::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* extraNode = NULL;
	for (FCDETypeContainer::const_iterator it = types.begin(); it != types.end(); ++it)
	{
		// Add the <extra> element and its types
		extraNode = (*it)->LetWriteToXML(parentNode);
	}
	return extraNode;
}

void FCDExtra::WriteTechniquesToXML(xmlNode* parentNode) const
{
	// Write the types within the given parent XML tree node.
	for (FCDETypeContainer::const_iterator itT = types.begin(); itT != types.end(); ++itT)
	{
		(*itT)->WriteTechniquesToXML(parentNode);
	}
}

//
// FCDEType
//

ImplementObjectType(FCDEType);

FCDEType::FCDEType(FCDocument* document, FCDExtra* _parent, const char* _name)
:	FCDObject(document)
,	parent(_parent), name(_name)
{
}

FCDEType::~FCDEType()
{
}

// Adds a technique of the given profile (or return the existing technique with this profile).
FCDETechnique* FCDEType::AddTechnique(const char* profile)
{
	FCDETechnique* technique = FindTechnique(profile);
	if (technique == NULL)
	{
		technique = techniques.Add(GetDocument(), this, profile);
		SetDirtyFlag();
	}
	return technique;
}

// Search for a profile-specific technique
const FCDETechnique* FCDEType::FindTechnique(const char* profile) const
{
	for (FCDETechniqueContainer::const_iterator itT = techniques.begin(); itT != techniques.end(); ++itT)
	{
		if (IsEquivalent((*itT)->GetProfile(), profile)) return *itT;
	}
	return NULL;
}

// Search for a root node with a specific element name
const FCDENode* FCDEType::FindRootNode(const char* name) const
{
	const FCDENode* rootNode = NULL;
	for (FCDETechniqueContainer::const_iterator itT = techniques.begin(); itT != techniques.end(); ++itT)
	{
		rootNode = (*itT)->FindChildNode(name);
		if (rootNode != NULL) break;
	}
	return rootNode;
}

FCDEType* FCDEType::Clone(FCDEType* clone) const
{
	// If no clone is given: create one
	if (clone == NULL)
	{
		clone = new FCDEType(const_cast<FCDocument*>(GetDocument()), NULL, name.c_str());
	}

	clone->techniques.reserve(techniques.size());
	for (FCDETechniqueContainer::const_iterator itT = techniques.begin(); itT != techniques.end(); ++itT)
	{
		FCDETechnique* cloneT = clone->AddTechnique((*itT)->GetProfile());
		(*itT)->Clone(cloneT);
	}
	return clone;
}

// Read in a COLLADA XML document
bool FCDEType::LoadFromXML(xmlNode* extraNode)
{
	bool status = true;

	// Do NOT verify that we have an <extra> element: we may be parsing a technique switch instead.

	// Read in the techniques
	xmlNodeList techniqueNodes;
	FindChildrenByType(extraNode, DAE_TECHNIQUE_ELEMENT, techniqueNodes);
	for (xmlNodeList::iterator itN = techniqueNodes.begin(); itN != techniqueNodes.end(); ++itN)
	{
		xmlNode* techniqueNode = (*itN);
		fm::string profile = ReadNodeProperty(techniqueNode, DAE_PROFILE_ATTRIBUTE);
		FCDETechnique* technique = AddTechnique(profile);
		status &= (technique->LoadFromXML(techniqueNode));
	}

	SetDirtyFlag();
	return status;
}

// Write to a COLLADA XML document
xmlNode* FCDEType::WriteToXML(xmlNode* parentNode) const
{
	if (name.empty() && techniques.empty()) return NULL;

	// Add the <extra> element and its techniques
	xmlNode* extraNode = AddChild(parentNode, DAE_EXTRA_ELEMENT);
	if (!name.empty()) AddAttribute(extraNode, DAE_TYPE_ATTRIBUTE, name);
	WriteTechniquesToXML(extraNode);
	return extraNode;
}

void FCDEType::WriteTechniquesToXML(xmlNode* parentNode) const
{
	// Write the techniques within the given parent XML tree node.
	for (FCDETechniqueContainer::const_iterator itT = techniques.begin(); itT != techniques.end(); ++itT)
	{
		(*itT)->LetWriteToXML(parentNode);
	}
}

//
// FCDENode
//

ImplementObjectType(FCDENode);

FCDENode::FCDENode(FCDocument* document, FCDENode* _parent)
:	FCDObject(document), parent(_parent)
{
	animated = FCDAnimatedCustom::Create(GetDocument());
}

FCDENode::~FCDENode()
{
	parent = NULL;
}

void FCDENode::SetContent(const fchar* _content)
{
	// As COLLADA doesn't allow for mixed content, release all the children.
	while (!children.empty())
	{
		children.back()->Release();
	}

	content = _content;
	SetDirtyFlag();
}

// Search for a children with a specific name
const FCDENode* FCDENode::FindChildNode(const char* name) const
{
	for (FCDENodeContainer::const_iterator itN = children.begin(); itN != children.end(); ++itN)
	{
		if (IsEquivalent((*itN)->GetName(), name)) return (*itN);
	}
	return NULL;
}

void FCDENode::FindChildrenNodes(const char* name, FCDENodeList& nodes) const
{
	for (FCDENodeContainer::const_iterator itN = children.begin(); itN != children.end(); ++itN)
	{
		if (IsEquivalent((*itN)->GetName(), name)) nodes.push_back(const_cast<FCDENode*>(*itN));
	}
}

const FCDENode* FCDENode::FindParameter(const char* name) const
{
	for (FCDENodeContainer::const_iterator itN = children.begin(); itN != children.end(); ++itN)
	{
		const FCDENode* node = (*itN);
		if (IsEquivalent(node->GetName(), name)) return node;
	}
	return NULL;
}

void FCDENode::FindParameters(FCDENodeList& nodes, StringList& names)
{
	for (FCDENodeContainer::iterator itN = children.begin(); itN != children.end(); ++itN)
	{
		FCDENode* node = (*itN);
		if (node->GetChildNodeCount() == 0)
		{
			nodes.push_back(node);
			names.push_back(node->GetName());
		}
	}
}

// Adds a new attribute to this extra tree node.
FCDEAttribute* FCDENode::AddAttribute(const char* _name, const fchar* _value)
{
	FCDEAttribute* attribute = FindAttribute(_name);
	if (attribute == NULL)
	{
		attribute = attributes.Add(_name, _value);
	}

	attribute->value = _value;
	SetDirtyFlag();
	return attribute;
}

// Search for an attribute with a specific name
const FCDEAttribute* FCDENode::FindAttribute(const char* name) const
{
	for (FCDEAttributeContainer::const_iterator itA = attributes.begin(); itA != attributes.end(); ++itA)
	{
		if ((*itA)->name == name) return (*itA);
	}
	return NULL;
}

const fstring& FCDENode::ReadAttribute(const char* name) const
{
	const FCDEAttribute* attribute = FindAttribute(name);
	return (attribute != NULL) ? attribute->value : emptyFString;
}

FCDENode* FCDENode::AddParameter(const char* name, const fchar* value)
{
	FCDENode* parameter = AddChildNode();
	parameter->SetName(name);
	parameter->SetContent(value);
	SetDirtyFlag();
	return parameter;
}

FCDENode* FCDENode::Clone(FCDENode* clone) const
{
	if (clone == NULL) return NULL;

	clone->name = name;
	clone->content = content;

	clone->attributes.reserve(attributes.size());
	for (FCDEAttributeContainer::const_iterator itA = attributes.begin(); itA != attributes.end(); ++itA)
	{
		clone->AddAttribute((*itA)->name, (*itA)->value);
	}

	clone->children.reserve(children.size());
	for (FCDENodeContainer::const_iterator itC = children.begin(); itC != children.end(); ++itC)
	{
		FCDENode* clonedChild = clone->AddChildNode();
		(*itC)->Clone(clonedChild);
	}

	// TODO: Clone the animated custom..

	return clone;
}

// Read in this extra node from a COLLADA XML document
bool FCDENode::LoadFromXML(xmlNode* customNode)
{
	bool status = true;

	// Read in the node's name and children
	name = (const char*) customNode->name;
	ReadChildrenFromXML(customNode);
	
	// If there are no child nodes, we have a tree leaf: parse in the content and its animation
	content = (children.empty()) ? TO_FSTRING(ReadNodeContentFull(customNode)) : FS("");
	SAFE_RELEASE(animated);
	animated = FCDAnimatedCustom::Create(GetDocument(), customNode);

	// Read in the node's attributes
	for (xmlAttr* a = customNode->properties; a != NULL; a = a->next)
	{
		AddAttribute((const char*) a->name, (a->children != NULL) ? TO_FSTRING((const char*) (a->children->content)) : FS(""));
	}

	SetDirtyFlag();
	return status;
}

// Write out this extra to a COLLADA XML document
xmlNode* FCDENode::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* customNode = AddChild(parentNode, name.c_str(), content);
	
	// Write out the attributes
	for (FCDEAttributeContainer::const_iterator itA = attributes.begin(); itA != attributes.end(); ++itA)
	{
		const FCDEAttribute* attribute = (*itA);
		FUXmlWriter::AddAttribute(customNode, attribute->name.c_str(), attribute->value);
	}

	// Write out the animated element
	if (animated != NULL && animated->HasCurve())
	{
		GetDocument()->WriteAnimatedValueToXML(animated, customNode, name.c_str());
	}

	// Write out the children
	WriteChildrenToXML(customNode);
	return customNode;
}

// Read in the child nodes from the XML tree node
bool FCDENode::ReadChildrenFromXML(xmlNode* customNode)
{
	bool status = true;

	// Read in the node's children
	for (xmlNode* k = customNode->children; k != NULL; k = k->next)
	{
		if (k->type != XML_ELEMENT_NODE) continue;

		FCDENode* node = AddChildNode();
		status &= (node->LoadFromXML(k));
	}

	SetDirtyFlag();
	return status;
}

// Write out the child nodes to the XML tree node
void FCDENode::WriteChildrenToXML(xmlNode* customNode) const
{
	for (FCDENodeContainer::const_iterator itN = children.begin(); itN != children.end(); ++itN)
	{
		(*itN)->LetWriteToXML(customNode);
	}
}

//
// FCDETechnique
//

ImplementObjectType(FCDETechnique);

FCDETechnique::FCDETechnique(FCDocument* document, FCDEType* _parent, const char* _profile)
:	FCDENode(document, NULL)
,	parent(_parent), pluginOverride(NULL)
{
	if (_profile != NULL) profile = _profile;
}

FCDETechnique::~FCDETechnique() {}

FCDENode* FCDETechnique::Clone(FCDENode* clone) const
{
	if (clone == NULL)
	{
		clone = new FCDETechnique(const_cast<FCDocument*>(GetDocument()), NULL, profile.c_str());
	}
	else if (clone->GetObjectType().Includes(FCDETechnique::GetClassType()))
	{
		((FCDETechnique*) clone)->profile = profile;
	}

	FCDENode::Clone(clone);
	return clone;
}

// Read in/Write to a COLLADA XML document
bool FCDETechnique::LoadFromXML(xmlNode* techniqueNode)
{
	bool status = true;

	// Check for a plug-in that handles this profile.
	if (FCollada::pluginManager != NULL && parent != NULL)
	{
		pluginOverride = FCollada::pluginManager->ReadProfileFromXML(profile.c_str(), techniqueNode, parent->GetParent()->GetParent());
	}
	if (pluginOverride == NULL)
	{
		// Read in only the child elements: none of the attributes
		status &= (ReadChildrenFromXML(techniqueNode));
	}
	return status;
}

xmlNode* FCDETechnique::WriteToXML(xmlNode* parentNode) const
{
	// Create the technique for this profile and write out the children
	xmlNode* techniqueNode = AddTechniqueChild(parentNode, profile.c_str());
	if (pluginOverride == NULL)
	{
		WriteChildrenToXML(techniqueNode);
	}
	else
	{
		FCollada::pluginManager->WriteToXML(profile.c_str(), techniqueNode, pluginOverride);
	}
	return techniqueNode;
}

//
// FCDEAttribute
//

ImplementObjectType(FCDEAttribute);

FCDEAttribute::FCDEAttribute(const char* _name, const fchar* _value)
{
	name = _name;
	value = _value;
}
