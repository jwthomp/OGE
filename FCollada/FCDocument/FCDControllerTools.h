/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDControllerTools.h
	This file defines the FCDControllerTools namespace.
*/

#ifndef _FCD_CONTROLLER_TOOLS_H_
#define _FCD_CONTROLLER_TOOLS_H_

#include "FCDocument/FCDocument.h"

class FCDSkinController;

/** A translation map between old vertex position indices and the new indices.
	It is generated in the FCDGeometryPolygonsTools::GenerateUniqueIndices function. */
typedef fm::map<uint32, UInt32List> FCDGeometryIndexTranslationMap;

/** Holds commonly-used transformation functions for skin controllers. */
namespace FCDControllerTools
{
	/** Applies a translation map obtained from 
		FCDGeometryPolygonsTools::GenerateUniqueIndices to a given 
		FCDSkinController to synchronize with the updated indices. This is 
		useful for older engines and the applications that only support one 
		index per face-vertex pair.
		@param controller The skin controller to process.
		@param translationMap The map that translates old vertex position 
				indices into new indices. This map is recieved from 
				FCDGeometryPolygonsTools::GenerateUniqueIndices. */
	void FCOLLADA_EXPORT ApplyTranslationMap(FCDSkinController* controller, 
			FCDGeometryIndexTranslationMap& translationMap);

	/** Clones all the geometries tied to morph controllers. This is useful 
		when using GenerateUniqueIndices since all targets and the base meshe
		for morph controllers need to have the same indices.
		@param document The document to apply the cloning to.*/
	void FCOLLADA_EXPORT CloneMorphControllers(FCDocument* document);
}

#endif // _FCD_CONTROLLER_TOOLS_H_
