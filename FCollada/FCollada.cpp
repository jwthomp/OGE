/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDPlaceHolder.h"
#include "FUtils/FUTestBed.h"
#include "FColladaPlugin.h"

namespace FCollada
{
	static size_t libraryInitializationCount = 0;
	static FUObjectContainer<FCDocument> topDocuments;
	static bool dereferenceFlag = true;
	FColladaPluginManager* pluginManager = NULL; // Externed in FCDExtra.cpp.

	FCOLLADA_EXPORT unsigned long GetVersion() { return FCOLLADA_VERSION; }

	FCOLLADA_EXPORT void Initialize()
	{
		if (pluginManager == NULL)
		{
			pluginManager = new FColladaPluginManager();
		}
		++libraryInitializationCount;
	}

	FCOLLADA_EXPORT size_t Release()
	{
		FUAssert(libraryInitializationCount > 0, return 0);

		if (--libraryInitializationCount == 0)
		{
			// Release all the top-level documents, in case some were missing.
			topDocuments.clear();

			// Detach all the plug-ins.
			SAFE_DELETE(pluginManager);
		}
		return libraryInitializationCount;
	}

	FCOLLADA_EXPORT FCDocument* NewTopDocument()
	{
		// Just add the top documents to the above tracker: this will add one global tracker and the
		// document will not be released automatically by the document placeholders.
		return topDocuments.Add();
	}

	FCOLLADA_EXPORT size_t GetTopDocumentCount() { return topDocuments.size(); }
	FCOLLADA_EXPORT FCDocument* GetTopDocument(size_t index) { FUAssert(index < topDocuments.size(), return NULL); return topDocuments.at(index); }
	FCOLLADA_EXPORT bool IsTopDocument(FCDocument* document) { return topDocuments.contains(document); }

	FCOLLADA_EXPORT void GetAllDocuments(FCDocumentList& documents)
	{
		documents.clear();
		documents.insert(documents.end(), topDocuments.begin(), topDocuments.end());
		for (size_t index = 0; index < topDocuments.size(); ++index)
		{
			FCDocument* document = documents[index];
			FCDExternalReferenceManager* xrefManager = document->GetExternalReferenceManager();
			size_t placeHolderCount = xrefManager->GetPlaceHolderCount();
			for (size_t p = 0; p < placeHolderCount; ++p)
			{
				FCDPlaceHolder* placeHolder = xrefManager->GetPlaceHolder(p);
				FCDocument* target = placeHolder->GetTarget(false);
				if (target != NULL && !documents.contains(target)) documents.push_back(target);
			}
		}
	}

	FCOLLADA_EXPORT bool GetDereferenceFlag() { return dereferenceFlag; }
	FCOLLADA_EXPORT void SetDereferenceFlag(bool flag) { dereferenceFlag = flag; }

	FCOLLADA_EXPORT bool RegisterPlugin(FColladaPlugin* plugin)
	{
		if(pluginManager == NULL)
			return false;

		return pluginManager->AddPlugin(plugin);
	}

#ifndef RETAIL
};

extern FUTestSuite* _testFMArray;
extern FUTestSuite* _testFMTree;
extern FUTestSuite* _testFUObject;
extern FUTestSuite* _testFUCrc32;
extern FUTestSuite* _testFUFunctor;
extern FUTestSuite* _testFUEvent;
extern FUTestSuite* _testFUString;
extern FUTestSuite* _testFUFileManager;
extern FUTestSuite* _testFUBoundingTest;

namespace FCollada
{
	FCOLLADA_EXPORT void RunTests(FUTestBed& testBed)
	{
		// FMath tests
		testBed.RunTestSuite(::_testFMArray);
		testBed.RunTestSuite(::_testFMTree);

		// FUtils tests
		testBed.RunTestSuite(::_testFUObject);
		testBed.RunTestSuite(::_testFUCrc32);
		testBed.RunTestSuite(::_testFUFunctor);
		testBed.RunTestSuite(::_testFUEvent);
		testBed.RunTestSuite(::_testFUString);
		testBed.RunTestSuite(::_testFUFileManager);
		testBed.RunTestSuite(::_testFUBoundingTest);
	}
#endif // RETAIL
};
