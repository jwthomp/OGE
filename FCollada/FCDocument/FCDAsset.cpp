/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAsset.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUDateTime.h"
#include "FUtils/FUFileManager.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDAsset
//

ImplementObjectType(FCDAsset);

FCDAsset::FCDAsset(FCDocument* document)
:	FCDObject(document)
,	upAxis(FMVector3::YAxis)
,	unitName(FC("meter")), unitConversionFactor(1.0f)
,	hasUnits(false), hasUpAxis(false)
{
	creationDateTime = modifiedDateTime = FUDateTime::GetNow();
}

FCDAsset::~FCDAsset()
{
}

// Clone another asset element.
FCDAsset* FCDAsset::Clone(FCDAsset* clone, bool cloneAllContributors) const
{
	if (clone == NULL) clone = new FCDAsset(const_cast<FCDocument*>(GetDocument()));

	// Clone all the asset-level parameters.
	clone->creationDateTime = creationDateTime;
	clone->modifiedDateTime = FUDateTime::GetNow();
	clone->keywords = keywords;
	clone->revision = revision;
	clone->subject = subject;
	clone->title = title;
	clone->upAxis = upAxis;
	clone->unitName = unitName;
	clone->unitConversionFactor = unitConversionFactor;
	clone->hasUnits = hasUnits;
	clone->hasUpAxis = hasUpAxis;

	if (cloneAllContributors)
	{
		// Clone all the contributors
		for (const FCDAssetContributor** it = contributors.begin(); it != contributors.end(); ++it)
		{
			FCDAssetContributor* clonedContributor = clone->AddContributor();
			(*it)->Clone(clonedContributor);
		}
	}

	return clone;
}

// Read in the <asset> element from a COLLADA XML document
bool FCDAsset::LoadFromXML(xmlNode* assetNode)
{
	bool status = true;
	for (xmlNode* child = assetNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		fm::string content = ReadNodeContentFull(child);
		if (IsEquivalent(child->name, DAE_CONTRIBUTOR_ASSET_ELEMENT))
		{
			FCDAssetContributor* contributor = AddContributor();
			status &= contributor->LoadFromXML(child);
		}
		else if (IsEquivalent(child->name, DAE_CREATED_ASSET_PARAMETER))
		{
			FUStringConversion::ToDateTime(content, creationDateTime);
		}
		else if (IsEquivalent(child->name, DAE_KEYWORDS_ASSET_PARAMETER))
		{
			keywords = TO_FSTRING(content);
		}
		else if (IsEquivalent(child->name, DAE_MODIFIED_ASSET_PARAMETER))
		{
			FUStringConversion::ToDateTime(content, modifiedDateTime); 
		}
		else if (IsEquivalent(child->name, DAE_REVISION_ASSET_PARAMETER))
		{
			revision = TO_FSTRING(content);
		}
		else if (IsEquivalent(child->name, DAE_SUBJECT_ASSET_PARAMETER))
		{
			subject = TO_FSTRING(content);
		}
		else if (IsEquivalent(child->name, DAE_TITLE_ASSET_PARAMETER))
		{
			title = TO_FSTRING(content);
		}
		else if (IsEquivalent(child->name, DAE_UNITS_ASSET_PARAMETER))
		{
			unitName = TO_FSTRING(ReadNodeName(child));
			unitConversionFactor = FUStringConversion::ToFloat(ReadNodeProperty(child, DAE_METERS_ATTRIBUTE));
			if (unitName.empty()) unitName = FC("UNKNOWN");
			if (IsEquivalent(unitConversionFactor, 0.0f) || unitConversionFactor < 0.0f) unitConversionFactor = 1.0f;
			hasUnits = true;
		}
		else if (IsEquivalent(child->name, DAE_UPAXIS_ASSET_PARAMETER))
		{
			if (IsEquivalent(content, DAE_X_UP)) upAxis = FMVector3::XAxis;
			else if (IsEquivalent(content, DAE_Y_UP)) upAxis = FMVector3::YAxis;
			else if (IsEquivalent(content, DAE_Z_UP)) upAxis = FMVector3::ZAxis;
			hasUpAxis = true;
		}
		else
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_CHILD_ELEMENT, child->line);
		}
	}

	SetDirtyFlag();
	return status;
}

// Write out the <asset> element to a COLLADA XML node tree
xmlNode* FCDAsset::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* assetNode = AddChild(parentNode, DAE_ASSET_ELEMENT);

	// Update the 'last modified time'
	FCDAsset* hackedAsset = const_cast<FCDAsset*>(this);
	hackedAsset->modifiedDateTime = FUDateTime::GetNow();

	// Write out the contributors first.
	for (const FCDAssetContributor** itC = contributors.begin(); itC != contributors.end(); ++itC)
	{
		(*itC)->LetWriteToXML(assetNode);
	}

	// Write out the parameters, one by one and in the correct order.
	AddChild(assetNode, DAE_CREATED_ASSET_PARAMETER, FUStringConversion::ToString(creationDateTime));
	if (!keywords.empty()) AddChild(assetNode, DAE_KEYWORDS_ASSET_PARAMETER, keywords);
	AddChild(assetNode, DAE_MODIFIED_ASSET_PARAMETER, FUStringConversion::ToString(modifiedDateTime));
	if (!revision.empty()) AddChild(assetNode, DAE_REVISION_ASSET_PARAMETER, revision);
	if (!subject.empty()) AddChild(assetNode, DAE_SUBJECT_ASSET_PARAMETER, subject);
	if (!title.empty()) AddChild(assetNode, DAE_TITLE_ASSET_PARAMETER, title);

	// Finally: <unit> and <up_axis>
	if (hasUnits)
	{
		xmlNode* unitNode = AddChild(assetNode, DAE_UNITS_ASSET_PARAMETER);
		AddAttribute(unitNode, DAE_METERS_ATTRIBUTE, unitConversionFactor);
		AddAttribute(unitNode, DAE_NAME_ATTRIBUTE, unitName);
	}

	if (hasUpAxis)
	{
		const char* upAxisString = DAE_Y_UP;
		if (IsEquivalent(upAxis, FMVector3::YAxis)) upAxisString = DAE_Y_UP;
		else if (IsEquivalent(upAxis, FMVector3::XAxis)) upAxisString = DAE_X_UP;
		else if (IsEquivalent(upAxis, FMVector3::ZAxis)) upAxisString = DAE_Z_UP;
		AddChild(assetNode, DAE_UPAXIS_ASSET_PARAMETER, upAxisString);
	}
	return assetNode;
}

//
// FCDAssetContributor
//

ImplementObjectType(FCDAssetContributor);

FCDAssetContributor::FCDAssetContributor(FCDocument* document)
:	FCDObject(document)
{
}

FCDAssetContributor::~FCDAssetContributor()
{
}

FCDAssetContributor* FCDAssetContributor::Clone(FCDAssetContributor* clone) const
{
	if (clone == NULL) clone = new FCDAssetContributor(const_cast<FCDocument*>(GetDocument()));

	clone->author = author;
	clone->authoringTool = authoringTool;
	clone->comments = comments;
	clone->copyright = copyright;
	clone->sourceData = sourceData;

	return clone;
}

// Returns whether this contributor element contain any valid data
bool FCDAssetContributor::IsEmpty() const
{
	return author.empty() && authoringTool.empty() && comments.empty() && copyright.empty() && sourceData.empty();
}

// Read in the <asset><contributor> element from a COLLADA XML document
bool FCDAssetContributor::LoadFromXML(xmlNode* contributorNode)
{
	bool status = true;
	for (xmlNode* child = contributorNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		fm::string content = ReadNodeContentFull(child);
		if (IsEquivalent(child->name, DAE_AUTHOR_ASSET_PARAMETER))
		{
			author = TO_FSTRING(content);
		}
		else if (IsEquivalent(child->name, DAE_AUTHORINGTOOL_ASSET_PARAMETER))
		{
			authoringTool = TO_FSTRING(content);
		}
		else if (IsEquivalent(child->name, DAE_COMMENTS_ASSET_PARAMETER))
		{
			comments = TO_FSTRING(content);
		}
		else if (IsEquivalent(child->name, DAE_COPYRIGHT_ASSET_PARAMETER))
		{
			copyright = TO_FSTRING(content);
		}
		else if (IsEquivalent(child->name, DAE_SOURCEDATA_ASSET_PARAMETER))
		{
			sourceData = TO_FSTRING(content);
		}
		else
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_AC_CHILD_ELEMENT, child->line);
		}
	}
	SetDirtyFlag();
	return status;
}

// Write out the <asset><contributor> element to a COLLADA XML node tree
xmlNode* FCDAssetContributor::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* contributorNode = NULL;
	if (!IsEmpty())
	{
		contributorNode = AddChild(parentNode, DAE_CONTRIBUTOR_ASSET_ELEMENT);
		if (!author.empty()) AddChild(contributorNode, DAE_AUTHOR_ASSET_PARAMETER, author);
		if (!authoringTool.empty()) AddChild(contributorNode, DAE_AUTHORINGTOOL_ASSET_PARAMETER, authoringTool);
		if (!comments.empty()) AddChild(contributorNode, DAE_COMMENTS_ASSET_PARAMETER, comments);
		if (!copyright.empty()) AddChild(contributorNode, DAE_COPYRIGHT_ASSET_PARAMETER, copyright);
		if (!sourceData.empty())
		{
			fstring sourceDataUrl = GetDocument()->GetFileManager()->GetFileURL(sourceData, false);
			ConvertFilename(sourceDataUrl);
			AddChild(contributorNode, DAE_SOURCEDATA_ASSET_PARAMETER, sourceDataUrl);
		}
	}
	return contributorNode;
}
