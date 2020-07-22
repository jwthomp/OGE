/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEmitter.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDEmitterParticle.h"
/*#include "FCDocument/FCDParticleSpray.h"
#include "FCDocument/FCDParticleSnow.h"
#include "FCDocument/FCDParticleBlizzard.h"
#include "FCDocument/FCDParticleSuperSpray.h"
#include "FCDocument/FCDParticlePArray.h"
#include "FCDocument/FCDParticlePCloud.h" */
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"

using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDEmitter
//

ImplementObjectType(FCDEmitter);

FCDEmitter::FCDEmitter(FCDocument* document)
:	FCDEntity(document, "Emitter")
{
}

FCDEmitter::~FCDEmitter()
{
}

FCDEntity* FCDEmitter::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDEmitter* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDEmitter(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDEmitter::GetClassType())) clone = (FCDEmitter*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
	}
	return _clone;
}


bool FCDEmitter::LoadFromXML(xmlNode* node)
{
	bool status = Parent::LoadFromXML(node);
	if (!status) return status;
	if (!IsEquivalent(node->name, DAE_EMITTER_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_UNKNOWN_ELEMENT, node->line);
		return status;
	}


	SetDirtyFlag();
	return status;
}

xmlNode* FCDEmitter::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* emitterNode = WriteToEntityXML(parentNode, DAE_EMITTER_ELEMENT);


	Parent::WriteToExtraXML(emitterNode);
	return emitterNode;
}

bool FCDEmitter::Link()
{

	 return true;
}
