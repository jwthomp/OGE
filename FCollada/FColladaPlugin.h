/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FColladaPlugin.h
	This file contains the FColladaPlugin class
	and the internal FColladaPluginManager class.
*/

#ifndef _FCOLLADA_PLUGIN_H_
#define _FCOLLADA_PLUGIN_H_

#ifndef _FU_PLUGIN_H_
#include "FUtils/FUPlugin.h"
#endif // _FU_PLUGIN_H_

class FUPluginManager;

/**
	A FCollada plug-in.
	We use plug-ins in FCollada to parse technique-specific informations.
	There are three techniques that FCollada handles directly but can be overriden
	using one or more plug-ins: "MAYA", "MAX3D" and "FCOLLADA".

	@see FUPlugin
	@ingroup FCollada
*/
class FCOLLADA_EXPORT FColladaPlugin : public FUPlugin
{
private:
	DeclareObjectType(FUPlugin);

public:
	/** Destructor. */
	virtual ~FColladaPlugin() {}

	/** Retrieves the name of the technique profile that this plug-in handles.
		@return The name of the technique profile. */
	virtual const char* GetProfileName() = 0;

	/** Reads in the XML tree node and generates the profile-specific object for it.
		@param techniqueNode The XML tree node for the profile-specific technique.
		@param parent The object that is creating this profile-specific object.
		@return An handle to the profile-specific object. When this handle is NULL,
			the FCollada extra tree will be generated. When the extra tree that contains
			this handle is released, the handle will also be released. If you need this
			handle elsewhere, make sure you correctly use the FUObjectPtr, FUObjectRef,
			FUObjectList or FUObjectContainer templates. */
	virtual FUObject* ReadFromXML(xmlNode* techniqueNode, FUObject* parent) = 0;

	/** Writes the XML tree node for the given handle.
		Note that the \<technique profile="X"\> will be automatically created.
		@param techniqueNode The XML tree node to contain the profile-specific object.
			This node is in the form: \<technique profile="X"\>
		@param handle An object created by the ReadFromXML function.
		@return The created XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* techniqueNode, const FUObject* handle) const = 0;
};

/**
	[INTERNAL] The COLLADA profile plug-ins manager.
*/
class FCOLLADA_EXPORT FColladaPluginManager
{
private:
	typedef fm::map<uint32, FColladaPlugin*> PluginMap;
	PluginMap plugins; // The key is a crc32 value for the profile string.
	FUPluginManager* loader;

public:
	/** [INTERNAL] Constructor. */
	FColladaPluginManager();

	/** [INTERNAL] Destructor. */
	virtual ~FColladaPluginManager();

	/** [INTERNAL] Adds a plugin to the PluginMap. Please use the FCollada::RegisterPlugin
		method.
		@param plugin The plugin to manually add to the plugin map.
		@return ?. */
	bool AddPlugin(FColladaPlugin* plugin);

	/** [INTERNAL] Attempts to create an object from the profile-specific technique.
		If this function returns NULL, the extra tree will do the parsing.
		@param profileName The profile string.
		@param techniqueNode The XML tree node with the profile string.
		@param parent The parent object under which the plug-in object should be created.
		@return The object created by a plug-in that handles this profile. When this
			pointer is NULL, the extra tree should do the parsing. */
	FUObject* ReadProfileFromXML(const char* profileName, xmlNode* techniqueNode, FUObject* parent);

	/** [INTERNAL] Calls the plug-in that handles this profile to write out the given object.
		@param profileName The profile string.
		@param techniqueNode The XML tree node to fill in with the object information.
		@param handle The plug-in object.
		@return An XML tree node created by the plug-in. */
	xmlNode* WriteToXML(const char* profileName, xmlNode* techniqueNode, const FUObject* handle) const;
};

#endif // _FCOLLADA_PLUGIN_H_

