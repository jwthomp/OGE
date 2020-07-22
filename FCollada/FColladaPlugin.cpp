/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FColladaPlugin.h"
#include "FUtils/FUPlugin.h"
#include "FUtils/FUPluginManager.h"

//
// FColladaPlugin
//

ImplementObjectType(FColladaPlugin);

//
// FColladaPluginManager
//

FColladaPluginManager::FColladaPluginManager()
:	loader(NULL)
{
	// Create the plug-in loader and create all the FCollada plug-ins.
	loader = new FUPluginManager(FC("*.fcp|*.dll|*.fvp"));
	loader->LoadPlugins(FColladaPlugin::GetClassType());

	// Retrieve and sort the plug-ins.
	size_t pluginCount = loader->GetLoadedPluginCount();
	for (size_t i = 0; i < pluginCount; ++i)
	{
		FUPlugin* _plugin = loader->GetLoadedPlugin(i);
		FUAssert(_plugin->HasType(FColladaPlugin::GetClassType()), continue);
		FColladaPlugin* plugin = (FColladaPlugin*) _plugin;
		const char* profileName = plugin->GetProfileName();
		if (profileName != NULL && profileName[0] != 0)
		{
			uint32 crc = FUCrc32::CRC32(profileName);
			plugins.insert(crc, plugin);
		}
	}
}

FColladaPluginManager::~FColladaPluginManager()
{
	CLEAR_POINTER_STD_PAIR_CONT(PluginMap, plugins);
	SAFE_DELETE(loader);
}

bool FColladaPluginManager::AddPlugin(FColladaPlugin* plugin)
{
	if(plugin == NULL || plugin->GetProfileName() == NULL || plugin->GetProfileName()[0] == 0)
		return false;

	FUCrc32::crc32 crc = FUCrc32::CRC32(plugin->GetProfileName());
	plugins.insert(crc, plugin);
	return true;
}

FUObject* FColladaPluginManager::ReadProfileFromXML(const char* profileName, xmlNode* techniqueNode, FUObject* parent)
{
	// Verify whether this profile is handled by a plug-in.
	uint32 crc = FUCrc32::CRC32(profileName);
	PluginMap::iterator plugin = plugins.find(crc);
	if (plugin == plugins.end()) return NULL;
	else return plugin->second->ReadFromXML(techniqueNode, parent);
}

xmlNode* FColladaPluginManager::WriteToXML(const char* profileName, xmlNode* techniqueNode, const FUObject* handle) const
{
	uint32 crc = FUCrc32::CRC32(profileName);
	PluginMap::const_iterator plugin = plugins.find(crc);
	if (plugin == plugins.end()) return NULL;
	else return plugin->second->WriteToXML(techniqueNode, handle);
}
