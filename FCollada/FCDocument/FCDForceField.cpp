/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDForceDeflector.h"
#include "FCDocument/FCDForceDrag.h"
#include "FCDocument/FCDForceGravity.h"
#include "FCDocument/FCDForcePBomb.h"
#include "FCDocument/FCDForceWind.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDForceField
//

ImplementObjectType(FCDForceField);

FCDForceField::FCDForceField(FCDocument* document)
:	FCDEntity(document, "ForceField")
{
	information = new FCDExtra(GetDocument(), this);
}

FCDForceField::~FCDForceField()
{
}


FCDEntity* FCDForceField::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDForceField* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDForceField(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDForceField::GetClassType())) clone = (FCDForceField*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{

		// Clone the extra information.
		information->Clone(clone->information);
	}
	return _clone;
}

bool FCDForceField::LoadFromXML(xmlNode* node)
{
	bool status = Parent::LoadFromXML(node);
	if (!status) return status;
	if (!IsEquivalent(node->name, DAE_FORCE_FIELD_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_FORCE_FIELD_ELEMENT, node->line);
		return status;
	}


	if (information != NULL)
	{
		information = new FCDExtra(GetDocument(), this);
	}
	status &= (information->LoadFromXML(node));
	
	SetDirtyFlag();
	return status;
}

xmlNode* FCDForceField::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* forceFieldNode = Parent::WriteToEntityXML(parentNode, DAE_FORCE_FIELD_ELEMENT);


	if (information != NULL)
	{
		information->WriteTechniquesToXML(forceFieldNode);
	}

	Parent::WriteToExtraXML(forceFieldNode);
	return forceFieldNode;
}
