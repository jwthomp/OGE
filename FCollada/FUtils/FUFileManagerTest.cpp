/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUtils/FUTestBed.h"
#include "FUtils/FUFileManager.h"
#if defined(WIN32)
	#include <direct.h>
#endif // WIN32

inline bool PlatformEquivalent(fstring& f, const fchar* check)
{
#ifdef WIN32
	return IsEquivalent(f, check);
#else
	fstring p = check;
	p.replace('\\', '/');
	return IsEquivalent(f, p);
#endif // WIN32
}

TESTSUITE_START(FUFileManager)

TESTSUITE_TEST(0, Substrings)
	const size_t testFilenameCount = 6;
	const fchar* testFilenames[testFilenameCount] = {
		FC("C:\\TestFolder\\TestFile.dae"),
		FC("/UnixFolder/AbsoluteFile.dae"),
		FC("\\\\WindowsUNCPath/NetworkFolder/SomeFile.dae"),
		FC("file:///strangeURI"),
		FC("/UnixFolder/UnixFolder.AnnoyingExtension/SomeFileWithoutExtension"),
		FC("file://www.someweb.com/index")
	};

	for (size_t i = 0; i < testFilenameCount; ++i)
	{
		// Check the ExtractNetworkHostname function
		fstring filename = testFilenames[i];
		fstring hostname = FUFileManager::ExtractNetworkHostname(filename);

		static const fchar* checkFilenameReferences[testFilenameCount] = {
			FC("C:\\TestFolder\\TestFile.dae"),
			FC("/UnixFolder/AbsoluteFile.dae"),
#ifdef WIN32
			FC("/NetworkFolder/SomeFile.dae"),
#else // WIN32
			FC("\\\\WindowsUNCPath/NetworkFolder/SomeFile.dae"),
#endif // WIN32
			FC("file:///strangeURI"),
			FC("/UnixFolder/UnixFolder.AnnoyingExtension/SomeFileWithoutExtension"),
			FC("file://www.someweb.com/index")
		};

#ifdef WIN32
		static const fchar* checkHostnames[testFilenameCount] =
			{ FC(""), FC(""), FC("WindowsUNCPath"), FC(""), FC(""), FC("") };
#else // WIN32
		static const fchar* checkHostnames[testFilenameCount] =
			{ FC(""), FC(""), FC(""), FC(""), FC(""), FC("") };
#endif // WIN32

		PassIf(IsEquivalent(filename, checkFilenameReferences[i]));
		PassIf(IsEquivalent(hostname, checkHostnames[i]));

		// Check the GetFileExtension function
		fstring extension = FUFileManager::GetFileExtension(testFilenames[i]);

		static const fchar* checkExtensions[testFilenameCount] =
			{ FC("dae"), FC("dae"), FC("dae"), FC(""), FC(""), FC("") };
		PassIf(IsEquivalent(extension, checkExtensions[i]));

		// Check the StripFileFromPath function
		filename = FUFileManager::StripFileFromPath(testFilenames[i]);
		static const fchar* checkStrippedFile[testFilenameCount] =
			{ FC("C:\\TestFolder\\"), FC("/UnixFolder/"), FC("\\\\WindowsUNCPath/NetworkFolder/"),
			FC("file:///"), FC("/UnixFolder/UnixFolder.AnnoyingExtension/"), FC("file://www.someweb.com/") };
		PassIf(IsEquivalent(filename, checkStrippedFile[i]));
	}


TESTSUITE_TEST(1, BasicFileManagement)
	FUFileManager manager;

	// Verify that the 'localhost' hostname is dropped.
	fstring f = FC("file://localhost/rootFolder/testFile.dae");
	f = manager.MakeFilePathAbsolute(f);
	PassIf(PlatformEquivalent(f, FC("\\rootFolder\\testFile.dae")));

TESTSUITE_TEST(2, RelativePaths)
	// Verify the relative file path generation for local files
	FUFileManager manager;
	manager.PushRootFile(FC("C:\\AFolder\\AFolder.D\\AFile"));
	fstring f = manager.MakeFilePathRelative(FC("D:\\BPath\\BFile"));
	PassIf(PlatformEquivalent(f, FC("D:\\BPath\\BFile")));

	f = manager.MakeFilePathRelative(FC("\\BPath\\BFile"));
	PassIf(PlatformEquivalent(f, FC("\\BPath\\BFile")));

#ifdef WIN32
	// Verify drive-aware relative path generation.
	f = manager.MakeFilePathRelative(FC("\\AFolder\\AFolder.D\\BFile"));
	PassIf(PlatformEquivalent(f, FC(".\\BFile")));

	f = manager.MakeFilePathRelative(FC("\\AFolder\\BFile"));
	PassIf(PlatformEquivalent(f, FC("..\\BFile")));

	f = manager.MakeFilePathRelative(FC("..\\AFolder.D\\BFile"));
	// [glaforte 21-07-2006] might be optimization possible, although the current output is not wrong.
	PassIf(PlatformEquivalent(f, FC("..\\AFolder.D\\BFile")));
#endif

	f = manager.MakeFilePathRelative(FC("\\\\AFolder\\AFile"));
	PassIf(PlatformEquivalent(f, FC("\\\\AFolder\\AFile")));

#ifndef WIN32
	// Verify the relative file path generation with a Unix file as the root
	manager.PopRootFile();
	manager.PushRootFile(FC("/ARoot/AFolder/AFolder.D/AFile"));
	f = manager.MakeFilePathRelative(FC("/ARoot/AFolder/AFolder.D/FileG"));
	PassIf(PlatformEquivalent(f, FC("./FileG")));

	f = manager.MakeFilePathRelative(FC("/ARoot/AFolder/TestOut.pdg"));
	PassIf(PlatformEquivalent(f, FC("../TestOut.pdg")));
#endif // WIN32

	// Verify the relative file path generation with a network file as the root
	manager.PopRootFile();
	manager.PushRootFile(FC("\\\\ANetwork\\AFolder\\AFolder.D\\AFile"));
	f = manager.MakeFilePathRelative(FC("C:\\BLocal\\BFolder\\AFile"));
	PassIf(PlatformEquivalent(f, FC("C:\\BLocal\\BFolder\\AFile")));

	f = manager.MakeFilePathRelative(FC("\\\\ANetwork\\AFolder\\BFile"));
	PassIf(PlatformEquivalent(f, FC("..\\BFile")));
	
	// Verify the relative file path generation when the root has no drive letter
	manager.PushRootFile(FC("\\ARootFolder\\BFile"));
	f = manager.MakeFilePathRelative(FC("A:\\ARootFolder\\CFile"));
	PassIf(PlatformEquivalent(f, FC("A:\\ARootFolder\\CFile")));

	f = manager.MakeFilePathRelative(FC("BPureRelative\\CFile"));
	PassIf(PlatformEquivalent(f, FC(".\\BPureRelative\\CFile")));

TESTSUITE_TEST(3, AbsolutePaths)
#ifdef WIN32
	char currentPath[1024];
	_getcwd(currentPath, 1023);
	currentPath[1023] = 0;
	fchar driveLetter = (fchar) toupper(currentPath[0]);
#endif // WIN32

	// Verify the relative file path generation for local files
	FUFileManager manager;
	manager.PushRootFile(FC("C:\\AFolder\\AFolder.D\\AFile"));
	fstring f = manager.MakeFilePathAbsolute(FC(".\\BFile"));
#ifdef WIN32
	PassIf(PlatformEquivalent(f, FC("C:\\AFolder\\AFolder.D\\BFile")));
#else
	PassIf(PlatformEquivalent(f, FC("\\C:\\AFolder\\AFolder.D\\BFile")));
#endif // WIN32

	manager.PopRootFile();
	manager.PushRootFile(FC("\\AFolder\\AFolder.D\\AFile"));
	f = manager.MakeFilePathAbsolute(FC("..\\BFile.S"));
#ifdef WIN32
	fstring check = FC("C:\\AFolder\\BFile.S");
	check[0] = driveLetter;
	PassIf(PlatformEquivalent(f, check.c_str()));
#else
	PassIf(PlatformEquivalent(f, FC("\\AFolder\\BFile.S")));
#endif // WIN32

TESTSUITE_TEST(4, URI_Generation)
	// Verify the URI generation with respect to a local file
	FUFileManager manager;
	manager.PushRootFile(FC("C:\\AFolder\\AFolder.D\\AFile"));
	fstring f = manager.GetFileURL(FC("C:\\AFolder\\AFolder.D\\BFile"), true);
	PassIf(f == FC("./BFile"));
	f = manager.GetFileURL(FC("C:\\AFolder\\AFolder.D\\BFile"), false);
	PassIf(f == FC("file:///C:/AFolder/AFolder.D/BFile"));

#ifdef WIN32
	f = manager.GetFileURL(FC("\\\\BNetwork\\BFolder\\BFile"), true);
	PassIf(f == FC("file://BNetwork/BFolder/BFile"));
	f = manager.GetFileURL(FC("\\\\BNetwork\\BFolder\\BFile"), false);
	PassIf(f == FC("file://BNetwork/BFolder/BFile"));
#endif // WIN32

	f = manager.GetFileURL(FC("file:///C:/AFolder/BFolder/BFile"), true);
	PassIf(f == FC("../BFolder/BFile"));
	f = manager.GetFileURL(FC("file:///C:/AFolder/BFolder/BFile"), false);
	PassIf(f == FC("file:///C:/AFolder/BFolder/BFile"));

#ifdef WIN32
	// Verify the URI generation with respect to a networked file
	manager.PushRootFile(FC("\\\\BNetwork\\BFolder\\BSubfolder\\BFile"));
	f = manager.GetFileURL(FC("C:\\BFolder\\BSubfolder\\CFile"), true);
	PassIf(f == FC("file:///C:/BFolder/BSubfolder/CFile"));
	f = manager.GetFileURL(FC("C:\\BFolder\\BSubfolder\\CFile"), false);
	PassIf(f == FC("file:///C:/BFolder/BSubfolder/CFile"));

	f = manager.GetFileURL(FC("\\\\BNetwork\\BFolder\\CFile"), true);
	PassIf(f == FC("../CFile"));
	f = manager.GetFileURL(FC("\\\\BNetwork\\BFolder\\CFile"), false);
	PassIf(f == FC("file://BNetwork/BFolder/CFile"));

	f = manager.GetFileURL(FC("file://BNetwork/BFolder/BSubfolder/CFile"), true);
	PassIf(f == FC("./CFile"));
	f = manager.GetFileURL(FC("file://BNetwork/BFolder/BSubfolder/CFile"), false);
	PassIf(f == FC("file://BNetwork/BFolder/BSubfolder/CFile"));
#endif // WIN32

TESTSUITE_TEST(5, BackwardCompatibility)

	// Verify the handling of the file paths that we used to export.
#ifdef WIN32
	FUFileManager manager;
	manager.PushRootFile(FC("C:\\AFolder\\AFolder.D\\AFile"));
	fstring f = manager.GetFilePath(FC("file://C|/AFolder/BFolder/BFile"));
	PassIf(PlatformEquivalent(f, FC("C:\\AFolder\\BFolder\\BFile")));

	f = manager.MakeFilePathRelative(FC("file://C|/AFolder/BFolder/BFile"));
	PassIf(PlatformEquivalent(f, FC("..\\BFolder\\BFile")));
#endif // WIN32

TESTSUITE_TEST(6, ApplicationFolderName)
	fstring applicationFolderName = FUFileManager::GetApplicationFolderName();
	FailIf(applicationFolderName.empty()); // really not much else I can test for..

TESTSUITE_END
