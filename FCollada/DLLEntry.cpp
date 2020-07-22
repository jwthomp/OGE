/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"

#ifdef FCOLLADA_DLL
#ifdef WIN32
HINSTANCE hInstance = NULL;

BOOL WINAPI DllMain(HINSTANCE _hInstance, ULONG UNUSED(fdwReason), LPVOID UNUSED(lpvReserved))
{
	hInstance = _hInstance;
	return TRUE;
}
#elif defined(MAC_TIGER) || defined(LINUX)
void __attribute((constructor)) DllEntry(void)
{
}

void __attribute((destructor)) DllTerminate(void)
{
}
#endif // WIN32

#include "FMath/FMColor.h"
#include "FMath/FMRandom.h"
#include "FUtils/FUDebug.h"
#include "FUtils/FULogFile.h"
#include "FUtils/FUBoundingBox.h"
#include "FUtils/FUBoundingSphere.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimationClipTools.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSceneNodeTools.h"
#include "FCDocument/FCDSceneNodeIterator.h"

extern void TrickLinker3();

// Trick the linker so that it adds the functionalities of the classes that are not used internally.
FCOLLADA_EXPORT void TrickLinker()
{
	// FMColor
	FMColor* color = NULL;
	float* f = NULL;
	color->ToFloats(f, 4);

	// FULogFile
	FULogFile* logFile = NULL;
	logFile->WriteLine("Test");

	// FUBoundingBox and FUBoundingSphere
	FUBoundingBox bb;
	FUBoundingSphere ss;
	if (!bb.Overlaps(ss))
	{
		// FUDebug
		DEBUG_OUT("Tricking Linker...");
	}

	// FCDAnimationClipTools
	FUObjectRef<FCDocument> d = FCollada::NewTopDocument();
	FCDAnimationClipTools::ResetAnimationClipTimes(d, 0.0f);

	// FCDSceneNodeTools
	FCDSceneNode* n = d->GetVisualSceneLibrary()->AddEntity();
	FCDSceneNodeIterator it(n);
	it.GetNode();
	FCDSceneNodeTools::GenerateSampledAnimation(n);
	FMMatrix44List mat44s = FCDSceneNodeTools::GetSampledAnimationMatrices();
	FCDSceneNodeTools::ClearSampledAnimation();

	TrickLinker3(); // FCDSceneNodeIterator.
}

#endif // FCOLLADA_DLL
