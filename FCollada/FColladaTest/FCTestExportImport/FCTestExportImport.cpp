/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDCamera.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTransform.h"

#include "FCTestExportImport.h"
using namespace FCTestExportImport;

static fm::string sceneNode1Id, sceneNode2Id, sceneNode3Id;

// Test import of a code-generated scene, with library entities.
// Does the export, re-import and validates that the information is intact.
TESTSUITE_START(FCDExportReimport)

TESTSUITE_TEST(0, Export)
	// Write out a simple document with three visual scenes
	FCDocument* doc = FCollada::NewTopDocument();
	FCDSceneNode* sceneNode1 = doc->AddVisualScene();
	sceneNode1->SetName(FC("Scene1"));
	FCDSceneNode* sceneNode2 = doc->AddVisualScene();
	sceneNode2->SetName(FC("Scene2"));
	FCDSceneNode* sceneNode3 = doc->AddVisualScene();
	sceneNode3->SetName(FC("Scene3"));

	// Fill in the other libraries
	PassIf(FillLayers(fileOut, doc));
	PassIf(FillImageLibrary(fileOut, doc->GetImageLibrary()));
	PassIf(FillCameraLibrary(fileOut, doc->GetCameraLibrary()));
	PassIf(FillLightLibrary(fileOut, doc->GetLightLibrary()));
	PassIf(FillForceFieldLibrary(fileOut, doc->GetForceFieldLibrary()));
	PassIf(FillEmitterLibrary(fileOut, doc->GetEmitterLibrary())); // must occur after FillForceFieldLibrary;
	PassIf(FillGeometryLibrary(fileOut, doc->GetGeometryLibrary()));
	PassIf(FillControllerLibrary(fileOut, doc->GetControllerLibrary())); // must occur after FillGeometryLibrary;
	PassIf(FillMaterialLibrary(fileOut, doc->GetMaterialLibrary())); // must occur after FillImageLibrary;
	PassIf(FillVisualScene(fileOut, sceneNode2)); // must occur after lights, cameras, geometries and controllers;
	PassIf(FillPhysics(fileOut, doc)); // must occur after the visual scene is filled in;
	PassIf(FillAnimationLibrary(fileOut, doc->GetAnimationLibrary())); // must occur last;
	doc->WriteToFile(FC("TestOut.dae"));

	// Check the dae id that were automatically generated: they should be unique
	FailIf(sceneNode1->GetDaeId() == sceneNode2->GetDaeId());
	FailIf(sceneNode1->GetDaeId() == sceneNode3->GetDaeId());
	FailIf(sceneNode2->GetDaeId() == sceneNode3->GetDaeId());
	sceneNode1Id = sceneNode1->GetDaeId();
	sceneNode2Id = sceneNode2->GetDaeId();
	sceneNode3Id = sceneNode3->GetDaeId();
	SAFE_RELEASE(doc);

TESTSUITE_TEST(1, Reimport)
	// Import back this document
	FCDocument* idoc = FCollada::NewTopDocument();
	FUErrorSimpleHandler errorHandler;
	idoc->LoadFromFile(FC("TestOut.dae"));
#ifdef _WIN32
	OutputDebugStringA(errorHandler.GetErrorString());
#endif
	PassIf(errorHandler.IsSuccessful());

	// Verify that all the data we pushed is still available
	// Note that visual scenes may be added by other tests: such as for joints.
	FCDVisualSceneNodeLibrary* vsl = idoc->GetVisualSceneLibrary();
	PassIf(vsl->GetEntityCount() >= 3);

	// Verify that the visual scene ids are unique.
	for (size_t i = 0; i < vsl->GetEntityCount(); ++i)
	{
		FCDSceneNode* inode = vsl->GetEntity(i);
		for (size_t j = 0; j < i; ++j)
		{
			FCDSceneNode* jnode = vsl->GetEntity(j);
			FailIf(inode->GetDaeId() == jnode->GetDaeId());
		}
	}

	// Verify that the three wanted visual scene ids exist and find the one we fill in.
	bool found1 = false, found3 = false;
	FCDSceneNode* found2 = NULL;
	for (size_t i = 0; i < vsl->GetEntityCount(); ++i)
	{
		FCDSceneNode* inode = vsl->GetEntity(i);
		if (inode->GetDaeId() == sceneNode1Id)
		{
			FailIf(found1);
			PassIf(inode->GetName() == FC("Scene1"));
			found1 = true;
		}
		else if (inode->GetDaeId() == sceneNode2Id)
		{
			FailIf(found2 != NULL);
			PassIf(inode->GetName() == FC("Scene2")); 
			found2 = inode;
		}
		else if (inode->GetDaeId() == sceneNode3Id)
		{
			FailIf(found3);
			PassIf(inode->GetName() == FC("Scene3"));
			found3 = true;
		}
	}
	PassIf(found2 != NULL);

	// Compare all these re-imported library contents
	PassIf(CheckLayers(fileOut, idoc));
	PassIf(CheckVisualScene(fileOut, found2));
	PassIf(CheckImageLibrary(fileOut, idoc->GetImageLibrary()));
	PassIf(CheckCameraLibrary(fileOut, idoc->GetCameraLibrary()));
	PassIf(CheckEmitterLibrary(fileOut, idoc->GetEmitterLibrary()));
	PassIf(CheckForceFieldLibrary(fileOut, idoc->GetForceFieldLibrary()));
	PassIf(CheckLightLibrary(fileOut, idoc->GetLightLibrary()));
	PassIf(CheckGeometryLibrary(fileOut, idoc->GetGeometryLibrary()));
	PassIf(CheckControllerLibrary(fileOut, idoc->GetControllerLibrary()));
	PassIf(CheckMaterialLibrary(fileOut, idoc->GetMaterialLibrary()));
	PassIf(CheckAnimationLibrary(fileOut, idoc->GetAnimationLibrary()));
	PassIf(CheckPhysics(fileOut, idoc));
	
	SAFE_RELEASE(idoc);

TESTSUITE_END
