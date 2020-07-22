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
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDMorphController.h"
#include "FUtils/FUDaeParser.h"
using namespace FUDaeParser;

ImplementObjectType(FCDController);

FCDController::FCDController(FCDocument* document) : FCDEntity(document, "Controller")
{
}

FCDController::~FCDController()
{
}


// Sets the type of this controller to a skin controller.
FCDSkinController* FCDController::CreateSkinController()
{
	morphController = NULL;
	skinController = new FCDSkinController(GetDocument(), this);
	SetDirtyFlag();
	return skinController;
}

// Sets the type of this controller to a morph controller.
FCDMorphController* FCDController::CreateMorphController()
{
	skinController = NULL;
	morphController = new FCDMorphController(GetDocument(), this);
	SetDirtyFlag();
	return morphController;
}

FCDEntity* FCDController::GetBaseTarget()
{
	if (skinController != NULL) return skinController->GetTarget();
	else if (morphController != NULL) return morphController->GetBaseTarget();
	else return NULL;
}
const FCDEntity* FCDController::GetBaseTarget() const
{
	if (skinController != NULL) return skinController->GetTarget();
	else if (morphController != NULL) return morphController->GetBaseTarget();
	else return NULL;
}

// Retrieves the base target geometry for this controller.
const FCDGeometry* FCDController::GetBaseGeometry() const
{
	const FCDEntity* base = GetBaseTarget();
	while (base != NULL && base->GetType() == FCDEntity::CONTROLLER)
	{
		base = ((const FCDController*) base)->GetBaseTarget();
	}

	if (base != NULL && base->GetType() == FCDEntity::GEOMETRY)
	{
		return (const FCDGeometry*) base;
	}
	return NULL;
}

// Returns the first FCDController directly above the base of this controller
const FCDController* FCDController::GetBaseGeometryController() const
{
	const FCDEntity* parentBase = this;
	const FCDEntity* base = GetBaseTarget();
	while (base != NULL && base->GetType() == FCDEntity::CONTROLLER)
	{
		parentBase = base;
		base = ((const FCDController*) base)->GetBaseTarget();
	}

	if (base != NULL && base->GetType() == FCDEntity::GEOMETRY)
	{
		return (const FCDController*) parentBase;
	}

	return NULL;
}

// Load this controller from a COLLADA <controller> node
bool FCDController::LoadFromXML(xmlNode* controllerNode)
{
	bool status = FCDEntity::LoadFromXML(controllerNode);
	if (!status) return status;
	if (!IsEquivalent(controllerNode->name, DAE_CONTROLLER_ELEMENT))
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_INVALID_CONTROlLER_LIB_NODE, controllerNode->line);
		return status;
	}

	// Find the <skin> or <morph> element and process it
	xmlNode* skinNode = FindChildByType(controllerNode, DAE_CONTROLLER_SKIN_ELEMENT);
	xmlNode* morphNode = FindChildByType(controllerNode, DAE_CONTROLLER_MORPH_ELEMENT);
	if (skinNode != NULL && morphNode != NULL)
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_CONTROLLER_TYPE_CONFLICT, controllerNode->line);
	}
	if (skinNode != NULL)
	{
		// Create and parse in the skin controller
		FCDSkinController* controller = CreateSkinController();
		status &= (controller->LoadFromXML(skinNode));
	}
	else if (morphNode != NULL)
	{
		// Create and parse in the morph controller
		FCDMorphController* controller = CreateMorphController();
		status &= (controller->LoadFromXML(morphNode));
	}
	else
	{
		FUError::Error(FUError::WARNING, FUError::WARNING_SM_BASE_MISSING, controllerNode->line);
	}
	return status;
}

// Write out this controller to a COLLADA XML node tree
xmlNode* FCDController::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* controllerNode = FCDEntity::WriteToEntityXML(parentNode, DAE_CONTROLLER_ELEMENT);
	if (skinController != NULL) skinController->LetWriteToXML(controllerNode);
	else if (morphController != NULL) morphController->LetWriteToXML(controllerNode);
	FCDEntity::WriteToExtraXML(controllerNode);
	return controllerNode;
}


bool FCDController::LinkImport()
{
	bool ret = true;
	if(GetBaseTarget() == NULL)
	{
		if(IsSkin())
		{
			ret &= GetSkinController()->LinkImport();
		}
		else if(IsMorph())
		{
			ret &= GetMorphController()->LinkImport();
		}
		else return false;

		// If our target is a controller, link it too.
		FCDEntity* entity = GetBaseTarget();
		if(entity != NULL && entity->GetType() == FCDEntity::CONTROLLER)
		{
			ret &= ((FCDController*)entity)->LinkImport();
		}
	}
	return ret;
}
