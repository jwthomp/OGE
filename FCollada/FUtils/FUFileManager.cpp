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
#include "FUtils/FUFile.h"
#include "FUtils/FUFileManager.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUXmlParser.h"

#if defined(WIN32)
	#include <direct.h>
#elif defined(MAC_TIGER)
	#include <mach-o/dyld.h>
	typedef int (*NSGetExecutablePathProcPtr)(char *buf, size_t *bufsize);
#elif defined(LINUX)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

//
// Macros and extra definitions
//

#ifdef WIN32
#define FOLDER_CHAR '\\'
#define UNWANTED_FOLDER_CHAR '/'
#define FOLDER_STR FC("\\")
#else
#define FOLDER_CHAR '/'
#define UNWANTED_FOLDER_CHAR '\\'
#define FOLDER_STR FC("/")
#endif

inline bool IsSomeFolderChar(fchar c) { return c == FOLDER_CHAR || c == UNWANTED_FOLDER_CHAR; }

//
// FUFileManager
//

FUFileManager::FUFileManager()
{
#if __PPU__
	// Push on the stack the default home path
	pathStack.push_back(FC("/app_home"));
#else
	// Push on the stack the original root path
	char fullPath[MAX_PATH];
	_getcwd(fullPath, MAX_PATH);
	pathStack.push_back(TO_FSTRING((const char*) fullPath));
#endif // __PPU__

	forceAbsolute = false;
}

FUFileManager::~FUFileManager()
{
}

// Set a new root path
void FUFileManager::PushRootPath(const fstring& path)
{
	// Ensure that the new root path is an absolute root path.
	fstring absolutePath = MakeFilePathAbsolute(path);
#ifdef WIN32
	// Ensure that the new root path has a drive letter.
	if (absolutePath.size() > 1 && absolutePath[0] == FOLDER_CHAR && absolutePath[1] != FOLDER_CHAR)
	{
		fchar driveLetter = pathStack.front().at(0);
		absolutePath = fstring(1, driveLetter) + FS(":") + absolutePath;
	}
#endif
	pathStack.push_back(absolutePath);
}

// Go back to the previous root path
void FUFileManager::PopRootPath()
{
	if (pathStack.size() > 1)
	{
		pathStack.pop_back();
	}
}

// Set the current path root, using a known filename
void FUFileManager::PushRootFile(const fstring& filename)
{
	fstring f = filename;

	// Strip the filename of the actual file's name
	f = StripFileFromPath(f);
	PushRootPath(f);
}

void FUFileManager::PopRootFile()
{
	PopRootPath();
}

// Open a file to read
FUFile* FUFileManager::OpenFile(const fstring& filename, bool write)
{
	fstring absoluteFilename = MakeFilePathAbsolute(filename);
	return new FUFile(absoluteFilename.c_str(), write ? FUFile::WRITE : FUFile::READ);
}

// Massage a given filename to be absolute
const fstring& FUFileManager::GetFilePath(const fstring& fileURL) const
{
    // Strip any prefix
	fstring filename = fileURL;
	filename.replace('\\', '/');
	if (filename.substr(0, 7) == FC("file://"))
	{
		filename = FUXmlParser::XmlToString(filename.c_str());
		if (filename.size() > 10 && filename[7] == '/' && (filename[9] == ':' || filename[9] == '|'))
		{
			// mapped drive (e.g. C:\work)
			filename = filename.c_str() + 8;
			if (filename[1] == '|') filename[1] = ':';
		}
		else
		{
			// networked drive (e.g. \\shared)
			// do nothing
		}
	}
	else
	{
		// Process the filename
		filename.replace('|', ':');
	}

	return MakeFilePathAbsolute(filename);
}

// Transform a file path into a file URL
const fstring& FUFileManager::GetFileURL(const fstring& filepath, bool relative) const
{
	static fstring url;
	if (!forceAbsolute && relative)
	{
		if (filepath.size() > 0 && filepath[0] != '.')
		{
			url = filepath;
			url = MakeFilePathRelative(url);
			if (url.size() > 0 && url[0] != '.')
			{
				// Unable to make the path relative, so return an absolute path
				relative = false;
			}
		}
		else
		{
			// Already a valid relative Url
			url = filepath;
		}
	
		url.replace('\\', '/');
	}

	if (forceAbsolute || !relative)
	{
		url = MakeFilePathAbsolute(filepath);

		// Look for a network path hostname
		fstring hostname = ExtractNetworkHostname(url);
		url = GenerateURI(FC("file"), hostname, url);
	}

	return url;
}

// Strip a full filename of its filename, returning the path
const fchar* FUFileManager::StripFileFromPath(const fstring& filename)
{
	static fchar fullPath[MAX_PATH + 1];
	fstrncpy(fullPath, filename.c_str(), MAX_PATH);
	fullPath[MAX_PATH] = 0;
	fchar* lastSlash = fstrrchr(fullPath, FC('/'));
	fchar* lastBackslash = fstrrchr(fullPath, FC('\\'));
	lastSlash = max(lastSlash, lastBackslash);
	if (lastSlash != NULL) *(lastSlash + 1) = 0;
	return fullPath;
}

// Extract the file extension out of a filename
const fchar* FUFileManager::GetFileExtension(const fstring& _filename)
{
	static fchar filename[MAX_PATH];
	fstrncpy(filename, _filename.c_str(), MAX_PATH);
	filename[MAX_PATH - 1] = 0;

	fchar* lastPeriod = fstrrchr(filename, '.');
	if (lastPeriod == NULL) return emptyFString.c_str();

	fchar* lastSlash = fstrrchr(filename, '/');
	fchar* lastBackslash = fstrrchr(filename, '\\');
	lastSlash = max(lastSlash, lastBackslash);
	if (lastSlash > lastPeriod) return emptyFString.c_str();

	fstrlower(lastPeriod + 1);	// [claforte] Untested on __PPU__, refer to definition of fstrlower.
	return lastPeriod + 1;
}

const fstring& FUFileManager::ExtractNetworkHostname(fstring& filename)
{
	static fstring hostname;
	hostname.clear();
#ifdef WIN32
	// UNC network paths are only supported on WIN32, right now.
	if (filename.size() > 2 && (filename[0] == '/' || filename[0] == '\\') && filename[1] == filename[0])
	{
		size_t nextSlash = min(filename.find('/', 2), filename.find('\\', 2));
		FUAssert(nextSlash != fstring::npos, return hostname); // The UNC patch should always have at least one network path
		hostname = filename.substr(2, nextSlash - 2);
		filename.erase(0, nextSlash); // Keep the slash to indicate absolute path.
	}
#endif
	return hostname;
}

// For a relative path, extract the list of the individual paths that must be traversed to get to the file.
void FUFileManager::ExtractPathStack(const fstring& name, fstring& hostname, fchar& driveLetter, FStringList& list, bool includeFilename)
{
	hostname.clear();
	list.clear();
	list.reserve(6);
	driveLetter = 0;

	fstring split = name;
	split.replace('\\', '/');

	// Extract the hostname
	hostname = ExtractNetworkHostname(split);

	// Extract the drive letter
#ifdef WIN32
	if (split.size() > 1 && (split[1] == ':' || split[1] == '|'))
	{
		driveLetter = split[0];
		if (driveLetter >= 'a' && driveLetter <= 'z') driveLetter += (fchar) ('A' - 'a');
		split.erase(0, 3);
	}
#endif

	while (!split.empty())
	{
		// Extract out the next path
		size_t slashIndex = split.find('/');
		if (slashIndex != fstring::npos && slashIndex != 0)
		{
			list.push_back(split.substr(0, slashIndex));
			split.erase(0, slashIndex + 1);
		}
		else if (slashIndex != 0)
		{
			if (includeFilename) list.push_back(split);
			split.clear();
		}
		else
		{
			split.erase(0, 1);
		}
	}
}

// Make a file path relative/absolute
const fstring& FUFileManager::MakeFilePathAbsolute(const fstring& _filePath) const
{
	static fstring filePath;
	fstring scheme, hostname;
	SplitURI(_filePath, scheme, hostname, filePath);

#ifdef WIN32
	filePath.replace('/', '\\');

	// Check for network paths
	if (!hostname.empty())
	{
		FUStringBuilder uncPath;
		uncPath.set(FC("\\\\")); uncPath.append(hostname);
		if (filePath.size() > 0 && filePath[0] != '\\') uncPath.append(FC('\\'));
		uncPath.append(filePath);
		filePath = uncPath.ToString();
		return filePath;
	}

	if (filePath.size() > 1 && filePath[1] == '|') filePath[1] = ':';
#endif

	if ((filePath.size() > 0 && IsSomeFolderChar(filePath[0]))
		|| (filePath.size() > 1 && filePath[1] == ':'))
	{
		// Already an absolute filepath
	}
	else
	{
		// Relative file path.
		fstring documentHost, localHost;
		fchar documentDrive, localDrive;
		FStringList documentPaths, localPaths;
		ExtractPathStack(pathStack.back(), documentHost, documentDrive, documentPaths, true);
		ExtractPathStack(filePath, localHost, localDrive, localPaths, true);
		for (FStringList::iterator it = localPaths.begin(); it != localPaths.end(); ++it)
		{
			// Look for special relative path tokens: '.' and '..'
			if ((*it) == FC(".")) {} // do nothing
			else if ((*it) == FC(".."))
			{
				// pop one path out
				if (!documentPaths.empty()) documentPaths.pop_back();
			} 
			else // traverse this path
			{
				documentPaths.push_back(*it);
			}
		}

		// Recreate the absolute filename
		FUStringBuilder outPath;
#ifdef WIN32
		if (!documentHost.empty())
		{
			outPath.set(FC("\\\\")); outPath.append(documentHost);
		}
		else if (documentDrive != 0)
		{
			outPath.set(documentDrive); outPath.append((fchar) ':');
		}
#endif // WIN32
		for (FStringList::iterator it = documentPaths.begin(); it != documentPaths.end(); ++it)
		{
#ifdef WIN32
			if (!outPath.empty()) 
#endif //WIN32
				outPath.append(FOLDER_STR);
			outPath.append(*it);
		}
#ifndef WIN32
		filePath = FOLDER_STR + outPath.ToString();
#else // WIN32
		filePath = outPath.ToString();
#endif // WIN32
	}

	return filePath;
}

const fstring& FUFileManager::MakeFilePathRelative(const fstring& _filePath) const
{
	static fstring filePath;
	filePath = _filePath;

	if (!filePath.empty() && filePath[0] != '.')
	{
		// First, ensure we have an absolute file path
		filePath = MakeFilePathAbsolute(_filePath);

		// Relative file path.
		fstring documentHostname, localHostname;
		fchar documentDrive, localDrive;
		FStringList documentPaths, localPaths;
		ExtractPathStack(pathStack.back(), documentHostname, documentDrive, documentPaths, true);
		ExtractPathStack(filePath, localHostname, localDrive, localPaths, true);
		if (fstricmp(documentHostname.c_str(), localHostname.c_str()) == 0
			&& (localDrive == 0 || localDrive == documentDrive))
		{
			// Extract the filename from the path stack
			fstring filename = localPaths.back();
			localPaths.pop_back();

			// Look for commonality in the path stacks
			size_t documentPathCount = documentPaths.size();
			size_t filePathCount = localPaths.size();
			size_t matchIndex = 0;
			for (; matchIndex < filePathCount && matchIndex < documentPathCount; ++matchIndex)
			{
				if (fstricmp(documentPaths[matchIndex].c_str(), localPaths[matchIndex].c_str()) != 0) break;
			}

			if (matchIndex > 0)
			{
				// There is some similar part, so generate the relative filename
				fstring relativePath;

				if (documentPathCount > matchIndex)
				{
					// Backtrack the document's path
					for (size_t i = matchIndex; i < documentPathCount; ++i)
					{
						relativePath += fstring(FC("../"));
					}
				}
				else
				{
					// Start at the document's root folder
					relativePath = FC("./");
				}

				// Add the file's relative path
				for (size_t i = matchIndex; i < filePathCount; ++i)
				{
					relativePath += localPaths[i] + FS("/");
				}
				filePath = relativePath + filename;
			}
		}
		else
		{
			// The file hosts/drivers are different, so use the absolute path
		}
	}

	filePath.replace(UNWANTED_FOLDER_CHAR, FOLDER_CHAR);

	return filePath;
}

const fstring& FUFileManager::GenerateURI(const fstring& scheme, const fstring& hostname, const fstring& _filename)
{
	fstring filename = _filename;
	filename.replace('\\', '/');
	filename.replace('|', ':');

	// Build the URI from its components
	FUStringBuilder uri;
	if (scheme.empty() && hostname.empty()) uri.append(filename);
	else
	{
		if (scheme.empty()) uri.append(FC("file://"));
		else
		{
			uri.append(scheme);
			uri.append(FC("://")); 
		}
		if (!hostname.empty())
		{
			uri.append(hostname);
		}
		if (filename.size() > 0) uri.append((fchar) '/');
		const fchar* f = filename.c_str();
		while (filename.length() > 0 && f[0] == '/') ++f;
		uri.append(f);
	}

	static fstring _uri;
	_uri = uri.ToString();
	return _uri;
}

void FUFileManager::SplitURI(const fstring& uri, fstring& scheme, fstring& hostname, fstring& filename)
{
	scheme.clear();
	hostname.clear();
	filename.clear();

	// Find the scheme through its ':' delimiter
	size_t schemeDelimiter = uri.find(':');
	if (schemeDelimiter != fstring::npos && schemeDelimiter > 1)
	{
		scheme = uri.substr(0, schemeDelimiter);
		schemeDelimiter += 3; // Skip the full delimiter '://'

		// Find the hostname through its '/' delimiter
		// The absence of a scheme implies the absence of hostname
		size_t hostDelimiter = uri.find('/', schemeDelimiter);
		if (hostDelimiter == fstring::npos)
		{
			// This should not happen, so assume that we have a full filename.
			scheme.clear();
			filename = uri;
		}
		else
		{
			hostname = uri.substr(schemeDelimiter, hostDelimiter - schemeDelimiter);

			// there was 3 or more slashes
			// 3 slashes means drive, more means networked
			if (hostname.empty() && (uri.size() > hostDelimiter + 1) &&
					(uri[hostDelimiter + 1] == '/'))
			{
				// public bug #44 says need file:///// for networked paths
				hostDelimiter ++;
				while ((uri.size() > hostDelimiter) && 
						(uri[hostDelimiter] == '/'))
				{
					hostDelimiter++;
				}
				size_t realHostDelimiter = uri.find('/', hostDelimiter);
				if (realHostDelimiter == fstring::npos)
				{
					// This should not happen, so assume that we have a full 
					// filename.
					scheme.clear();
					filename = uri;
					return;
				}
				hostname = uri.substr(hostDelimiter, 
						realHostDelimiter - hostDelimiter);
				hostDelimiter = realHostDelimiter;
			}

			// Check for bad URIs that don't include enough slahes.
			if (hostname.size() > 1 && (hostname[1] == ':' || hostname[1] == '|'))
			{
				hostname.clear();
				hostDelimiter = schemeDelimiter;
			}

			// The rest of the URI is the filename
			// If we have an absolute file path, we want to keep the slash.
			if (uri.size() > hostDelimiter + 1 && uri[hostDelimiter + 1] == '.')
			{
				hostDelimiter++;
			}
#ifdef WIN32
			// Don't keep the slash when we have a drive letter ahead.
			else if (uri.size() > hostDelimiter + 2 && (uri[hostDelimiter + 2] == ':' || uri[hostDelimiter + 2] == '|'))
			{
				hostDelimiter++;
			}
#endif
			filename = uri.substr(hostDelimiter);

			// Don't bother with the 'localhost' hostname.
			if (fstricmp(FC("localhost"), hostname.c_str()) == 0)
			{
				hostname.clear();
			}
		}
	}
	else
	{
		filename = uri;
	}
}

const fstring& FUFileManager::GetApplicationFolderName()
{
	static fstring _uri;

#ifdef WIN32
	fchar buffer[1024];
	GetModuleFileName(NULL, buffer, 1024);
	buffer[1023] = 0;
	_uri = buffer;
#elif defined(LINUX)
	char path[1024]; 
	char path2[1024];
	struct stat stat_buf;
	strncpy(path2, "/proc/self/exe", 1023);
	while (1) {
		ssize_t size = readlink(path2, path, 1023);
		if (size == -1) {
			strncpy(path, '\0', 1);
			break;
		}
		else {
			path[1024] = '\0';
			int i = stat (path, &stat_buf);
			if (i == -1) {
				break; 
			}
			else {
				if (!S_ISLNK(stat_buf.st_mode))
				break;
			}
			strncpy(path, path2, 1023);
		}
	}
	//"path" should have the application folder path in it.
	const char * exeName = &path[0];
	_uri = TO_FSTRING(exeName);
#elif defined(__APPLE__)
	char path[1024];
	size_t pathLength = 1023;
	static NSGetExecutablePathProcPtr NSGetExecutablePath = NULL;
	if (NSGetExecutablePath == NULL)
	{
		NSGetExecutablePath = (NSGetExecutablePathProcPtr) NSAddressOfSymbol(NSLookupAndBindSymbol("__NSGetExecutablePath"));
	}
	if (NSGetExecutablePath != NULL)
	{
		(*NSGetExecutablePath)(path, &pathLength);
		path[1023] = 0;
	}
	_uri = TO_FSTRING((const char*) path);
#endif // WIN32

	fstring out = StripFileFromPath(_uri);
	if (out.length() > 0 && (out.back() == UNWANTED_FOLDER_CHAR || out.back() == FOLDER_CHAR))
	{
		out.pop_back();
	}
	return _uri = out;
}

