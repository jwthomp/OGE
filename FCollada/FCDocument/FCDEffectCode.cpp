/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEffectCode.h"
#include "FUtils/FUFileManager.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

ImplementObjectType(FCDEffectCode);

FCDEffectCode::FCDEffectCode(FCDocument* document) : FCDObject(document)
{
	type = INCLUDE;
}

FCDEffectCode::~FCDEffectCode()
{
}

// Do not inline this function.  No memory-creating functions should be inline
void FCDEffectCode::SetSubId(const fm::string& _sid)
{ 
	sid = _sid; 
	SetDirtyFlag(); 
}

// Clone
FCDEffectCode* FCDEffectCode::Clone(FCDEffectCode* clone) const
{
	if (clone == NULL) clone = new FCDEffectCode(const_cast<FCDocument*>(GetDocument()));
	clone->type = type;
	clone->sid = sid;
	clone->filename = filename;
	clone->code = code;
	return clone;
}

// Read in the code/include from the XML node tree
bool FCDEffectCode::LoadFromXML(xmlNode* codeNode)
{
	bool status = true;
	if (IsEquivalent(codeNode->name, DAE_FXCMN_INCLUDE_ELEMENT)) type = INCLUDE;
	else if (IsEquivalent(codeNode->name, DAE_FXCMN_CODE_ELEMENT)) type = CODE;
	else
	{
		//return status.Fail(FS("Unknown effect code type."), codeNode->line);
		FUError::Error(FUError::ERROR, FUError::ERROR_UNKNOWN_EFFECT_CODE, codeNode->line); 
		return status;
	}

	// Read in the code identifier and the actual code or filename
	sid = ReadNodeProperty(codeNode, DAE_SID_ATTRIBUTE);
	if (type == INCLUDE && sid.empty())
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_SID_MISSING, codeNode->line);
	}
	if (type == INCLUDE) 
	{
		filename = ReadNodeUrl(codeNode).prefix;
		filename = GetDocument()->GetFileManager()->GetFilePath(filename);
	}
	else
	{
		code = TO_FSTRING(ReadNodeContentFull(codeNode)); 
	}

	SetDirtyFlag();
	return status;
}

// Write out the code/include to the COLLADA XML node tree
xmlNode* FCDEffectCode::WriteToXML(xmlNode* parentNode) const
{
	// Place the new element at the correct position in the XML.
	// This is necessary for FX profiles.
	xmlNode* includeAtNode = NULL;
	for (xmlNode* n = parentNode->children; n != NULL; n = n->next)
	{
		if (n->type != XML_ELEMENT_NODE) continue;
		else if (IsEquivalent(n->name, DAE_ASSET_ELEMENT)) continue;
		else if (IsEquivalent(n->name, DAE_FXCMN_CODE_ELEMENT)) continue;
		else if (IsEquivalent(n->name, DAE_FXCMN_INCLUDE_ELEMENT)) continue;
		else { includeAtNode = n; break; }
	}

	// In COLLADA 1.4, the 'sid' and 'url' attributes are required.
	// In the case of the sub-id, save it for later use.
	xmlNode* codeNode;
	fm::string& _sid = const_cast<fm::string&>(sid);
	switch (type)
	{
	case CODE:
		if (includeAtNode == NULL) codeNode = AddChild(parentNode, DAE_FXCMN_CODE_ELEMENT);
		else codeNode = InsertChild(parentNode, includeAtNode, DAE_FXCMN_CODE_ELEMENT);
		AddContent(codeNode, code);
		if (_sid.empty()) _sid = "code";
		AddNodeSid(codeNode, _sid);
		break;

	case INCLUDE: {
		if (includeAtNode == NULL) codeNode = AddChild(parentNode, DAE_FXCMN_INCLUDE_ELEMENT);
		else codeNode = InsertChild(parentNode, includeAtNode, DAE_FXCMN_INCLUDE_ELEMENT);
		if (_sid.empty()) _sid = "include";
		AddNodeSid(codeNode, _sid);
		fstring fileURL = GetDocument()->GetFileManager()->GetFileURL(filename, true);
		FUXmlWriter::ConvertFilename(fileURL);
		AddAttribute(codeNode, DAE_URL_ATTRIBUTE, fileURL);
		break; }

	default:
		codeNode = NULL;
	}
	return codeNode;
}
