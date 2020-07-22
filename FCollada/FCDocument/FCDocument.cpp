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
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationClip.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDCamera.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEmitter.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDImage.h"
#include "FCDocument/FCDLight.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDLibrary.hpp"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDPhysicsMaterial.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsScene.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTexture.h"
#include "FUtils/FUDaeParser.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUFileManager.h"
#include "FUtils/FUUniqueStringMap.h"
#include "FUtils/FUXmlDocument.h"
using namespace FUDaeParser;
using namespace FUDaeWriter;

//
// FCDocument
//
ImplementObjectType(FCDocument);

FCDocument::FCDocument()
{
	fileManager = new FUFileManager();
	asset = new FCDAsset(this);
	uniqueNameMap = new FUSUniqueStringMap();
	externalReferenceManager = new FCDExternalReferenceManager(this);

	animationLibrary = new FCDAnimationLibrary(this);
	animationClipLibrary = new FCDAnimationClipLibrary(this);
	cameraLibrary = new FCDCameraLibrary(this);
	controllerLibrary = new FCDControllerLibrary(this);
	emitterLibrary = new FCDEmitterLibrary(this);
	forceFieldLibrary = new FCDForceFieldLibrary(this);
	effectLibrary = new FCDEffectLibrary(this);
	geometryLibrary = new FCDGeometryLibrary(this);
	imageLibrary = new FCDImageLibrary(this);
	lightLibrary = new FCDLightLibrary(this);
	materialLibrary = new FCDMaterialLibrary(this);
	visualSceneLibrary = new FCDVisualSceneNodeLibrary(this);
	physicsMaterialLibrary = new FCDPhysicsMaterialLibrary(this);
	physicsModelLibrary = new FCDPhysicsModelLibrary(this);
	physicsSceneLibrary = new FCDPhysicsSceneLibrary(this);

	visualSceneRoot = NULL;
	physicsSceneRoot = NULL;

	// Document global parameters
	hasStartTime = hasEndTime = false;
	startTime = endTime = 0.0f;

	// Set the current COLLADA document version.
	version.ParseVersionNumbers(DAE_SCHEMA_VERSION);
}

FCDocument::~FCDocument()
{
	// Release the external references to and from this document
	// before all clearing the entities.
	FUObject::Detach();
	
	externalReferenceManager = NULL;

	// Release the libraries and the asset
	animationLibrary = NULL;
	animationClipLibrary = NULL;
	cameraLibrary = NULL;
	controllerLibrary = NULL;
	effectLibrary = NULL;
	emitterLibrary = NULL;
	forceFieldLibrary = NULL;
	geometryLibrary = NULL;
	imageLibrary = NULL;
	lightLibrary = NULL;
	materialLibrary = NULL;
	visualSceneLibrary = NULL;
	physicsMaterialLibrary = NULL;
	physicsModelLibrary = NULL;
	physicsSceneLibrary = NULL;
	asset = NULL;

	// Must be released last
	CLEAR_POINTER_VECTOR(layers);
	while (!animatedValues.empty()) { (*animatedValues.begin()).first->Release(); }
	animatedValueMap.clear();

	SAFE_DELETE(fileManager);
	SAFE_DELETE(uniqueNameMap);
}

// Adds an entity layer to the document.
FCDLayer* FCDocument::AddLayer()
{
	FCDLayer* layer = new FCDLayer();
	layers.push_back(layer);
	return layer;
}

// Releases an entity layer from the document.
void FCDocument::ReleaseLayer(FCDLayer* layer)
{
	layers.release(layer);
}

// Search for a driven curve that needs this animated value as a driver
bool FCDocument::LinkDriver(FCDAnimated* animated)
{
	if (animated->GetTargetPointer().empty()) return false;

	bool driven = false;
	size_t animationCount = animationLibrary->GetEntityCount();
	for (size_t i = 0; i < animationCount; ++i)
	{
		FCDAnimation* animation = animationLibrary->GetEntity(i);
		driven |= animation->LinkDriver(animated);
	}
	return driven;
}

// Search for an animation channel targeting the given pointer
void FCDocument::FindAnimationChannels(const fm::string& pointer, FCDAnimationChannelList& channels)
{
	if (pointer.empty()) return;

	size_t animationCount = (uint32) animationLibrary->GetEntityCount();
	for (size_t i = 0; i < animationCount; ++i)
	{
		FCDAnimation* animation = animationLibrary->GetEntity(i);
		animation->FindAnimationChannels(pointer, channels);
	}
}

// Gather a list of the indices of animated array element belonging to the node
void FCDocument::FindAnimationChannelsArrayIndices(xmlNode* targetArray, Int32List& animatedIndices)
{
	// Calculte the node's pointer
	fm::string pointer;
	CalculateNodeTargetPointer(targetArray, pointer);
	if (pointer.empty()) return;

	// Retrieve the channels for this pointer and extract their matrix indices.
	FCDAnimationChannelList channels;
	FindAnimationChannels(pointer, channels);
	for (FCDAnimationChannelList::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		fm::string qualifier = (*it)->GetTargetQualifier();
		int32 animatedIndex = ReadTargetMatrixElement(qualifier);
		if (animatedIndex != -1) animatedIndices.push_back(animatedIndex);
	}
}

// Search for a specific COLLADA library items with a given COLLADA id.
FCDAnimation* FCDocument::FindAnimation(const fm::string& daeId) { return animationLibrary->FindDaeId(daeId); }
FCDAnimationClip* FCDocument::FindAnimationClip(const fm::string& daeId) { return animationClipLibrary->FindDaeId(daeId); }
FCDCamera* FCDocument::FindCamera(const fm::string& daeId) { return cameraLibrary->FindDaeId(daeId); }
FCDController* FCDocument::FindController(const fm::string& daeId) { return controllerLibrary->FindDaeId(daeId); }
FCDEffect* FCDocument::FindEffect(const fm::string& daeId) { return effectLibrary->FindDaeId(daeId); }
FCDEmitter* FCDocument::FindEmitter(const fm::string& daeId) { return emitterLibrary->FindDaeId(daeId); }
FCDForceField* FCDocument::FindForceField(const fm::string& daeId) { return forceFieldLibrary->FindDaeId(daeId); }
FCDGeometry* FCDocument::FindGeometry(const fm::string& daeId) { return geometryLibrary->FindDaeId(daeId); }
FCDImage* FCDocument::FindImage(const fm::string& daeId) { return imageLibrary->FindDaeId(daeId); }
FCDLayer* FCDocument::FindLayer(const fm::string& name) { for (FCDLayerList::iterator itL = layers.begin(); itL != layers.end(); ++itL) { if ((*itL)->name == name) return *itL; } return NULL; }
FCDLight* FCDocument::FindLight(const fm::string& daeId) { return lightLibrary->FindDaeId(daeId); }
FCDMaterial* FCDocument::FindMaterial(const fm::string& daeId) { return  materialLibrary->FindDaeId(daeId); }
FCDSceneNode* FCDocument::FindVisualScene(const fm::string& daeId) { return visualSceneLibrary->FindDaeId(daeId); }
FCDPhysicsScene* FCDocument::FindPhysicsScene(const fm::string& daeId) { return physicsSceneLibrary->FindDaeId(daeId); }
FCDPhysicsMaterial* FCDocument::FindPhysicsMaterial(const fm::string& daeId) { return physicsMaterialLibrary->FindDaeId(daeId); }
FCDPhysicsModel* FCDocument::FindPhysicsModel(const fm::string& daeId) { return physicsModelLibrary->FindDaeId(daeId); }
FCDSceneNode* FCDocument::FindSceneNode(const fm::string& daeId) { return visualSceneLibrary->FindDaeId(daeId); }
FCDEntity* FCDocument::FindEntity(const fm::string& daeId)
{
#define CHECK_LIB(libraryName) { \
	FCDEntity* e = libraryName->FindDaeId(daeId); \
	if (e != NULL) return e; }

	CHECK_LIB(animationLibrary);
	CHECK_LIB(animationClipLibrary);
	CHECK_LIB(cameraLibrary);
	CHECK_LIB(controllerLibrary);
	CHECK_LIB(effectLibrary);
	CHECK_LIB(emitterLibrary);
	CHECK_LIB(forceFieldLibrary);
	CHECK_LIB(geometryLibrary);
	CHECK_LIB(imageLibrary);
	CHECK_LIB(lightLibrary);
	CHECK_LIB(materialLibrary);
	CHECK_LIB(visualSceneLibrary);
	CHECK_LIB(physicsSceneLibrary);
	CHECK_LIB(physicsMaterialLibrary);
	CHECK_LIB(physicsModelLibrary);
#undef CHECK_LIB

	return NULL;
}

// Add an animated value to the list
void FCDocument::RegisterAnimatedValue(FCDAnimated* animated)
{
	// Look for a duplicate in order to avoid memory loss
	//if (animated->GetValueCount() == 0 || FindAnimatedValue(animated->GetValue(0)) != NULL)
	if (animated->GetValueCount() == 0)
	{
		SAFE_RELEASE(animated);
		return;
	}

	// List the new animated value
	animatedValues.insert(animated, animated);

	// Also add to the map the individual values for easy retrieval
	size_t count = animated->GetValueCount();
	for (size_t i = 0; i < count; ++i)
	{
		const float* value = animated->GetValue(i);
		animatedValueMap.insert(value, animated);
	}
}

// Unregisters an animated value of the document.
void FCDocument::UnregisterAnimatedValue(FCDAnimated* animated)
{
	if (animated != NULL)
	{
		// Intentionally search from the end:
		// - In the destructor of the document, we delete from the end.
		// - In animation exporters, we add to the end and are likely to delete right away.
		FCDAnimatedSet::iterator it = animatedValues.find(animated);
		if (it != animatedValues.end())
		{
			animatedValues.erase(it);

			// Also remove to the map the individual values contained
			size_t count = animated->GetValueCount();
			for (size_t i = 0; i < count; ++i)
			{
				const float* value = animated->GetValue(i);
				FCDAnimatedValueMap::iterator itV = animatedValueMap.find(value);
				if (itV != animatedValueMap.end() && (*itV).second == animated)
				{
					animatedValueMap.erase(itV);
				}
			}
		}
	}
}

// Retrieve an animated value, given a value pointer
FCDAnimated* FCDocument::FindAnimatedValue(float* ptr)
{
	FCDAnimatedValueMap::iterator it = animatedValueMap.find((const float*) ptr);
	return (it != animatedValueMap.end()) ? (*it).second : NULL;
}

// Retrieve an animated value, given a value pointer
const FCDAnimated* FCDocument::FindAnimatedValue(const float* ptr) const
{
	FCDAnimatedValueMap::const_iterator it = animatedValueMap.find(ptr);
	return (it != animatedValueMap.end()) ? (*it).second : NULL;
}

// Retrieve an animated float value for a given fully qualified target
const float* FCDocument::FindAnimatedTarget(const fm::string& fullyQualifiedTarget)
{
	if (fullyQualifiedTarget.empty()) return NULL;
	fm::string target = (fullyQualifiedTarget[0] == '#') ? fullyQualifiedTarget.substr(1) : fullyQualifiedTarget;
	fm::string pointer, qualifier;
	SplitTarget(target, pointer, qualifier);

	// Find the pointer
	FCDAnimated* animatedValue = NULL;
	for (FCDAnimatedSet::iterator itA = animatedValues.begin(); itA != animatedValues.end(); ++itA)
	{
		FCDAnimated* animated = (*itA).first;
		if (animated->GetTargetPointer() == pointer) { animatedValue = animated; break; }
	}
	if (animatedValue == NULL) return NULL;

	// Return the qualified value
	size_t index = animatedValue->FindQualifier(qualifier);
	if (index == size_t(-1)) return NULL;
	return animatedValue->GetValue(index);
}

// Returns whether a given value pointer is animated
bool FCDocument::IsValueAnimated(const float* ptr) const
{
	const FCDAnimated* animated = FindAnimatedValue(ptr);
	return (animated != NULL) ? animated->HasCurve() : false;
}

// Insert new library elements
FCDSceneNode* FCDocument::AddVisualScene()
{
	return visualSceneRoot = visualSceneLibrary->AddEntity();
}
FCDPhysicsScene* FCDocument::AddPhysicsScene()
{
	return physicsSceneRoot = physicsSceneLibrary->AddEntity();
}

void FCDocument::SetFileUrl(const fstring& filename)
{
	fileManager->PopRootFile();
	fileUrl = GetFileManager()->MakeFilePathAbsolute(filename);
	fileManager->PushRootFile(fileUrl);
}

// Structure and enumeration used to order the libraries
enum nodeOrder { ANIMATION=0, ANIMATION_CLIP, IMAGE, EFFECT, MATERIAL, GEOMETRY, CONTROLLER, CAMERA, LIGHT, FORCE_FIELD, EMITTER, VISUAL_SCENE, PHYSICS_MATERIAL, PHYSICS_MODEL, PHYSICS_SCENE, UNKNOWN };
struct xmlOrderedNode { xmlNode* node; nodeOrder order; };
typedef fm::vector<xmlOrderedNode> xmlOrderedNodeList;

// Loads an entire COLLADA document file
bool FCDocument::LoadFromFile(const fstring& filename)
{ 
	bool status = true;

	SetFileUrl(filename); 

#if FCOLLADA_EXCEPTION
	try {
#endif

	// Parse the document into a XML tree
	FUXmlDocument daeDocument(filename.c_str(), true);
	xmlNode* rootNode = daeDocument.GetRootNode();
	if (rootNode != NULL)
	{
		// Read in the whole document from the root node
		status &= (LoadDocumentFromXML(rootNode));
	}
	else
	{
		status = false;
		FUError::Error(FUError::ERROR, FUError::ERROR_MALFORMED_XML);
	}

	// Clean-up the XML reader
	xmlCleanupParser();

#if FCOLLADA_EXCEPTION
	} catch(...)	{
		FUError::Error(FUError::ERROR, FUError::ERROR_PARSING_FAILED);
	}
#endif

	if (status) FUError::Error(FUError::D3BUG, FUError::DEBUG_LOAD_SUCCESSFUL);
	return status;
}

#ifdef UNICODE
bool FCDocument::LoadFromText(const fstring& basePath, const fchar* _text, size_t textLength)
{
	fm::string xmlTextString;
	if (textLength != 0)
	{
		// Downsize the text document into something 8-bit
		xmlTextString = FUStringConversion::ToString(fstring(_text, textLength));
		textLength = 0;
	}
	else
	{
		xmlTextString = FUStringConversion::ToString(_text);
	}
	return LoadFromText(basePath, xmlTextString.c_str(), textLength);
}
#endif // UNICODE

// Loads an entire COLLADA document from a given NULL-terminated fstring
bool FCDocument::LoadFromText(const fstring& basePath, const char* _text, size_t textLength)
{
	bool status = true;

	// Push the given path unto the file manager's stack
	fileManager->PushRootPath(basePath);

#if FCOLLADA_EXCEPTION
	try {
#endif

	fm::string textBuffer;
	const xmlChar* text = (const xmlChar*) _text;
	if (textLength != 0)
	{
		textBuffer = fm::string(_text, textLength);
		text = (const xmlChar*) textBuffer.c_str();
	}

	// Parse the document into a XML tree
	xmlDoc* daeDocument = xmlParseDoc(const_cast<xmlChar*>(text));
	if (daeDocument != NULL)
	{
		xmlNode *rootNode = xmlDocGetRootElement(daeDocument);

		// Read in the whole document from the root node
		status &= (LoadDocumentFromXML(rootNode));

		// Free the XML document
		xmlFreeDoc(daeDocument);
	}
	else
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_MALFORMED_XML);
	}

	// Clean-up the XML reader
	xmlCleanupParser();

#if FCOLLADA_EXCEPTION
	} catch(...)	{
		FUError::Error(FUError::ERROR, FUError::ERROR_PARSING_FAILED);
	}
#endif

	// Restore the original OS current folder
	fileManager->PopRootPath();

	if (status)  FUError::Error(FUError::D3BUG, FUError::DEBUG_LOAD_SUCCESSFUL);
	return status;
}

bool FCDocument::LoadDocumentFromXML(xmlNode* colladaNode)
{
	bool status = true;

	// The only root node supported is "COLLADA"
	if (!IsEquivalent(colladaNode->name, DAE_COLLADA_ELEMENT))
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_INVALID_ELEMENT, colladaNode->line);
		return status = false;
	}

	fm::string strVersion = ReadNodeProperty(colladaNode, DAE_VERSION_ATTRIBUTE);
	version.ParseVersionNumbers(strVersion);

	// Bucket the libraries, so that we can read them in our specific order
	// COLLADA 1.4: the libraries are now strongly-typed, so process all the elements
	xmlNode* sceneNode = NULL;
	xmlOrderedNodeList orderedLibraryNodes;
	xmlNodeList extraNodes;
	for (xmlNode* child = colladaNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		xmlOrderedNode n;
		n.node = child;
		n.order = UNKNOWN;
		if (IsEquivalent(child->name, DAE_LIBRARY_ANIMATION_ELEMENT)) n.order = ANIMATION;
		else if (IsEquivalent(child->name, DAE_LIBRARY_ANIMATION_CLIP_ELEMENT)) n.order = ANIMATION_CLIP;
		else if (IsEquivalent(child->name, DAE_LIBRARY_CAMERA_ELEMENT)) n.order = CAMERA;
		else if (IsEquivalent(child->name, DAE_LIBRARY_CONTROLLER_ELEMENT)) n.order = CONTROLLER;
		else if (IsEquivalent(child->name, DAE_LIBRARY_EFFECT_ELEMENT)) n.order = EFFECT;
		else if (IsEquivalent(child->name, DAE_LIBRARY_GEOMETRY_ELEMENT)) n.order = GEOMETRY;
		else if (IsEquivalent(child->name, DAE_LIBRARY_IMAGE_ELEMENT)) n.order = IMAGE;
		else if (IsEquivalent(child->name, DAE_LIBRARY_LIGHT_ELEMENT)) n.order = LIGHT;
		else if (IsEquivalent(child->name, DAE_LIBRARY_MATERIAL_ELEMENT)) n.order = MATERIAL;
		else if (IsEquivalent(child->name, DAE_LIBRARY_VSCENE_ELEMENT)) n.order = VISUAL_SCENE;
		else if (IsEquivalent(child->name, DAE_LIBRARY_FFIELDS_ELEMENT)) n.order = FORCE_FIELD;
		else if (IsEquivalent(child->name, DAE_LIBRARY_NODE_ELEMENT)) n.order = VISUAL_SCENE; // Process them as visual scenes.
		else if (IsEquivalent(child->name, DAE_LIBRARY_PMATERIAL_ELEMENT)) n.order = PHYSICS_MATERIAL;
		else if (IsEquivalent(child->name, DAE_LIBRARY_PMODEL_ELEMENT)) n.order = PHYSICS_MODEL;
		else if (IsEquivalent(child->name, DAE_LIBRARY_PSCENE_ELEMENT)) n.order = PHYSICS_SCENE;
		else if (IsEquivalent(child->name, DAE_ASSET_ELEMENT)) 
		{
			// Read in the asset information
			status &= (asset->LoadFromXML(child));
			continue;
		}
		else if (IsEquivalent(child->name, DAE_SCENE_ELEMENT))
		{
			// The <scene> element should be the last element of the document
			sceneNode = child;
			continue;
		}
		else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT))
		{
			extraNodes.push_back(child);
		}
		else
		{
			FUError::Error(FUError::WARNING, FUError::WARNING_BASE_NODE_TYPE, child->line);
			continue;
		}

		xmlOrderedNodeList::iterator it;
		for (it = orderedLibraryNodes.begin(); it != orderedLibraryNodes.end(); ++it)
		{
			if ((uint32) n.order < (uint32) (*it).order) break;
		}
		orderedLibraryNodes.insert(it, n);
	}

	// Look in the <extra> element for libaries
	for (xmlNodeList::iterator it = extraNodes.begin(); it != extraNodes.end(); ++it)
	{
		fm::string extraType = ReadNodeProperty(*it, DAE_TYPE_ATTRIBUTE);
		if (IsEquivalent(extraType, DAEFC_LIBRARIES_TYPE))
		{
			xmlNodeList techniqueNodes;
			FindChildrenByType(*it, DAE_TECHNIQUE_ELEMENT, techniqueNodes);
			for (xmlNodeList::iterator itT = techniqueNodes.begin(); itT != techniqueNodes.end(); ++itT)
			{
				for (xmlNode* child = (*itT)->children; child != NULL; child = child->next)
				{
					if (child->type != XML_ELEMENT_NODE) continue;

					xmlOrderedNode n;
					n.node = child;
					n.order = UNKNOWN;
					
					if (IsEquivalent(child->name, DAE_LIBRARY_EMITTER_ELEMENT)) n.order = EMITTER;
					else continue; // drop the node

					// Insert the library node at the correct place in the ordered list.
					xmlOrderedNodeList::iterator it;
					for (it = orderedLibraryNodes.begin(); it != orderedLibraryNodes.end(); ++it)
					{
						if ((uint32) n.order < (uint32) (*it).order) break;
					}
					orderedLibraryNodes.insert(it, n);
				}
			}
		}
	}

	// Process the ordered libraries
	size_t libraryNodeCount = orderedLibraryNodes.size();
	for (size_t i = 0; i < libraryNodeCount; ++i)
	{
		xmlOrderedNode& n = orderedLibraryNodes[i];
		switch (n.order)
		{
		case ANIMATION: status &= (animationLibrary->LoadFromXML(n.node)); break;
		case ANIMATION_CLIP: status &= (animationClipLibrary->LoadFromXML(n.node)); break;
		case CAMERA: status &= (cameraLibrary->LoadFromXML(n.node)); break;
		case CONTROLLER: status &= (controllerLibrary->LoadFromXML(n.node)); break;
		case EFFECT: status &= (effectLibrary->LoadFromXML(n.node)); break;
		case EMITTER: status &= (emitterLibrary->LoadFromXML(n.node)); break;
		case FORCE_FIELD: status &= (forceFieldLibrary->LoadFromXML(n.node)); break;
		case GEOMETRY: status &= (geometryLibrary->LoadFromXML(n.node)); break;
		case IMAGE: status &= (imageLibrary->LoadFromXML(n.node)); break;
		case LIGHT: status &= (lightLibrary->LoadFromXML(n.node)); break;
		case MATERIAL: status &= (materialLibrary->LoadFromXML(n.node)); break;
		case PHYSICS_MODEL: status &= (physicsModelLibrary->LoadFromXML(n.node)); break;
		case PHYSICS_MATERIAL: status &= (physicsMaterialLibrary->LoadFromXML(n.node)); break;
		case PHYSICS_SCENE: status &= (physicsSceneLibrary->LoadFromXML(n.node)); break;
		case VISUAL_SCENE: status &= (visualSceneLibrary->LoadFromXML(n.node)); break;
		case UNKNOWN: default: break;
		}
	}

	// Read in the <scene> element
	if (sceneNode != NULL)
	{
		// COLLADA 1.4: Look for a <instance_physics_scene> element
		xmlNode* instancePhysicsNode = FindChildByType(sceneNode, DAE_INSTANCE_PHYSICS_SCENE_ELEMENT);
		if (instancePhysicsNode != NULL)
		{
			FUUri instanceUri = ReadNodeUrl(instancePhysicsNode);
			if (instanceUri.prefix.length() > 0)
			{
				FUError::Error(FUError::ERROR, FUError::ERROR_INVALID_ELEMENT, sceneNode->line);
			}
			else if (instanceUri.suffix.length() == 0)
			{
				FUError::Error(FUError::ERROR, FUError::ERROR_INVALID_URI, sceneNode->line);
			}
			else
			{
				// Look for the correct physics scene to instantiate in the libraries
				physicsSceneRoot = FindPhysicsScene(instanceUri.suffix);
				if (physicsSceneRoot == NULL)
				{
					FUError::Error(FUError::WARNING, FUError::WARNING_MISSING_URI_TARGET, sceneNode->line);
				}
			}
		}

		// COLLADA 1.4: Look for a <instance_visual_scene> element
		xmlNode* instanceSceneNode = FindChildByType(sceneNode, DAE_INSTANCE_VSCENE_ELEMENT);
		if (instanceSceneNode != NULL)
		{
			FUUri instanceUri = ReadNodeUrl(instanceSceneNode);
			if (instanceUri.prefix.length() > 0)
			{
				FUError::Error(FUError::ERROR, FUError::ERROR_INVALID_ELEMENT, sceneNode->line);
			}
			else if (instanceUri.suffix.length() == 0)
			{
				FUError::Error(FUError::ERROR, FUError::ERROR_INVALID_URI, sceneNode->line);
			}
			else
			{
				// Look for the correct visual scene to instantiate in the libraries
				visualSceneRoot = FindVisualScene(instanceUri.suffix);
				if (visualSceneRoot == NULL)
				{
					FUError::Error(FUError::WARNING, FUError::WARNING_MISSING_URI_TARGET, sceneNode->line);
				}
			}
		}
	}

	// Link the effect surface parameters with the images
	for (size_t i = 0; i < materialLibrary->GetEntityCount(); ++i)
	{
		materialLibrary->GetEntity(i)->Link();
	}
	for (size_t i = 0; i < effectLibrary->GetEntityCount(); ++i)
	{
		effectLibrary->GetEntity(i)->Link();
	}
	for (size_t i = 0; i < controllerLibrary->GetEntityCount(); ++i)
	{
		status |= controllerLibrary->GetEntity(i)->LinkImport();
	}

	for (size_t i = 0; i < visualSceneLibrary->GetEntityCount(); i++)
	{
		FCDSceneNode* node = visualSceneLibrary->GetEntity(i);
		status |= node->LinkImport();
	}

	// Link the convex meshes with their point clouds (convex_hull_of)
	for (size_t i = 0; i < geometryLibrary->GetEntityCount(); ++i)
	{
		FCDGeometryMesh* mesh = geometryLibrary->GetEntity(i)->GetMesh();
		if (mesh) mesh->Link();
	}

	// Link the targeted entities, for 3dsMax cameras and lights
	size_t cameraCount = cameraLibrary->GetEntityCount();
	for (size_t i = 0; i < cameraCount; ++i)
	{
		FCDCamera* camera = cameraLibrary->GetEntity(i);
		status &= (camera->LinkTarget());
	}
	size_t lightCount = lightLibrary->GetEntityCount();
	for (size_t i = 0; i < lightCount; ++i)
	{
		FCDLight* light = lightLibrary->GetEntity(i);
		status &= (light->LinkTarget());
	}

	// Link particle systems
	size_t emitterCount = emitterLibrary->GetEntityCount();
	for (size_t i = 0; i < emitterCount; i++)
	{
		status &= emitterLibrary->GetEntity(i)->Link();
	}

	// Check that all the animation curves that need them, have found drivers
	size_t animationCount = animationLibrary->GetEntityCount();
	for (size_t i = 0; i < animationCount; ++i)
	{
		FCDAnimation* animation = animationLibrary->GetEntity(i);
		status &= (animation->Link());
	}
	
	if (!fileUrl.empty())
	{
		FCDExternalReferenceManager::RegisterLoadedDocument(this);
	}
	return status;
}

// Writes out the COLLADA document to a file
bool FCDocument::WriteToFile(const fstring& filename) 
{
	bool status = true;

	const_cast<FCDocument*>(this)->SetFileUrl(filename);

#if FCOLLADA_EXCEPTION
	try {
#endif
	// Create a new XML document
	FUXmlDocument daeDocument(filename.c_str(), false);
	xmlNode* rootNode = daeDocument.CreateRootNode(DAE_COLLADA_ELEMENT);
	status = WriteDocumentToXML(rootNode);
	if (status)
	{
		// Create the XML document and write it out to the given filename
		if (!daeDocument.Write())
		{
			FUError::Error(FUError::ERROR, FUError::ERROR_WRITE_FILE, rootNode->line);
		}
		else
		{
			FUError::Error(FUError::D3BUG, FUError::DEBUG_WRITE_SUCCESSFUL);
		}
	}

#if FCOLLADA_EXCEPTION
	} catch(...)	{
		FUError::Error(FUError::ERROR, FUError::ERROR_PARSING_FAILED);
	}
#endif

	return status;
}

// Writes out the entire COLLADA document to the given XML root node.
bool FCDocument::WriteDocumentToXML(xmlNode* colladaNode)
{
	bool status = true;
	if (colladaNode != NULL)
	{
		// Write the COLLADA document version and namespace: schema-required attributes
		AddAttribute(colladaNode, DAE_NAMESPACE_ATTRIBUTE, DAE_SCHEMA_LOCATION);
		AddAttribute(colladaNode, DAE_VERSION_ATTRIBUTE, DAE_SCHEMA_VERSION);

		// Write out the asset tag
		asset->LetWriteToXML(colladaNode);

		// Record the animation library. This library is built at the end, but should appear before the <scene> element.
		xmlNode* animationLibraryNode = NULL;
		if (!animationLibrary->IsEmpty())
		{
			animationLibraryNode = AddChild(colladaNode, DAE_LIBRARY_ANIMATION_ELEMENT);
		}

		// clean up the sub ids
#define CLEAN_LIBRARY_SUB_IDS(libraryName) if (!(libraryName)->IsEmpty()) { \
			size_t count = (libraryName)->GetEntityCount(); \
			for (size_t i = 0; i < count; i++) { \
			(libraryName)->GetEntity(i)->CleanSubId(); } }

		CLEAN_LIBRARY_SUB_IDS(physicsSceneLibrary);
		CLEAN_LIBRARY_SUB_IDS(physicsModelLibrary);
		CLEAN_LIBRARY_SUB_IDS(visualSceneLibrary);

#undef CLEAN_LIBRARY_SUB_IDS

		// Export the libraries
#define EXPORT_LIBRARY(memberName, daeElementName) if (!(memberName)->IsEmpty()) { \
			xmlNode* libraryNode = AddChild(colladaNode, daeElementName); \
			memberName->LetWriteToXML(libraryNode); }

		EXPORT_LIBRARY(animationClipLibrary, DAE_LIBRARY_ANIMATION_CLIP_ELEMENT);
		EXPORT_LIBRARY(physicsMaterialLibrary, DAE_LIBRARY_PMATERIAL_ELEMENT);
		EXPORT_LIBRARY(forceFieldLibrary, DAE_LIBRARY_FFIELDS_ELEMENT);
		EXPORT_LIBRARY(physicsModelLibrary, DAE_LIBRARY_PMODEL_ELEMENT);
		EXPORT_LIBRARY(physicsSceneLibrary, DAE_LIBRARY_PSCENE_ELEMENT);
		EXPORT_LIBRARY(cameraLibrary, DAE_LIBRARY_CAMERA_ELEMENT);
		EXPORT_LIBRARY(lightLibrary, DAE_LIBRARY_LIGHT_ELEMENT);
		EXPORT_LIBRARY(imageLibrary, DAE_LIBRARY_IMAGE_ELEMENT);
		EXPORT_LIBRARY(materialLibrary, DAE_LIBRARY_MATERIAL_ELEMENT);
		EXPORT_LIBRARY(effectLibrary, DAE_LIBRARY_EFFECT_ELEMENT);
		EXPORT_LIBRARY(geometryLibrary, DAE_LIBRARY_GEOMETRY_ELEMENT);
		EXPORT_LIBRARY(controllerLibrary, DAE_LIBRARY_CONTROLLER_ELEMENT);
		EXPORT_LIBRARY(visualSceneLibrary, DAE_LIBRARY_VSCENE_ELEMENT);

#undef EXPORT_LIBRARY

		if (physicsSceneRoot != NULL || visualSceneRoot != NULL)
		{
			// Create the <scene> element and instantiate the selected visual/physical scene.
			xmlNode* sceneNode = AddChild(colladaNode, DAE_SCENE_ELEMENT);
			if (physicsSceneRoot != NULL)
			{
				xmlNode* instancePhysicsSceneNode = AddChild(sceneNode, DAE_INSTANCE_PHYSICS_SCENE_ELEMENT);
				AddAttribute(instancePhysicsSceneNode, DAE_URL_ATTRIBUTE, fm::string("#") + physicsSceneRoot->GetDaeId());
			}
			if (visualSceneRoot != NULL)
			{
				xmlNode* instanceVisualSceneNode = AddChild(sceneNode, DAE_INSTANCE_VSCENE_ELEMENT);
				AddAttribute(instanceVisualSceneNode, DAE_URL_ATTRIBUTE, fm::string("#") + visualSceneRoot->GetDaeId());
			}
		}

		// Export the extra libraries
		if (emitterLibrary->GetEntityCount() > 0)
		{
			xmlNode* typedExtraNode = AddChild(colladaNode, DAE_EXTRA_ELEMENT);
			AddAttribute(typedExtraNode, DAE_TYPE_ATTRIBUTE, DAEFC_LIBRARIES_TYPE);
			xmlNode* typedTechniqueNode = AddTechniqueChild(typedExtraNode, DAE_FCOLLADA_PROFILE);

			// Export the emitter library
			xmlNode* libraryNode = AddChild(typedTechniqueNode, DAE_LIBRARY_EMITTER_ELEMENT);
			emitterLibrary->LetWriteToXML(libraryNode);
		}

		// Write out the animations
		if (animationLibraryNode != NULL)
		{
			animationLibrary->LetWriteToXML(animationLibraryNode);
		}
	}
	return status;
}

// Writes out a value's animations, if any, to the animation library of a COLLADA XML document.
bool FCDocument::WriteAnimatedValueToXML(const float* value, xmlNode* valueNode, const char* wantedSid, int32 arrayElement) const
{
	// Find the value's animations
	FCDAnimated* animated = const_cast<FCDAnimated*>(FindAnimatedValue(value));
	if (animated == NULL || !animated->HasCurve() || valueNode == NULL) return false;

	animated->SetArrayElement(arrayElement);

	WriteAnimatedValueToXML(animated, valueNode, wantedSid);
	return true;
}

void FCDocument::WriteAnimatedValueToXML(const FCDAnimated* _animated, xmlNode* valueNode, const char* wantedSid) const
{
	FCDAnimated* animated = const_cast<FCDAnimated*>(_animated);
	int32 arrayElement = animated->GetArrayElement();

	// Set a sid unto the XML tree node, in order to support animations
	if (!HasNodeProperty(valueNode, DAE_SID_ATTRIBUTE) && !HasNodeProperty(valueNode, DAE_ID_ATTRIBUTE))
	{
		AddNodeSid(valueNode, wantedSid);
	}

	// Calculate the XML tree node's target for the animation channel
	fm::string target;
	CalculateNodeTargetPointer(valueNode, target);
	animated->SetTargetPointer(target);
	if (!target.empty())
	{
		FCDAnimationChannelList channels;

		// Enforce the target pointer on all the curves
		for (size_t i = 0; i < animated->GetValueCount(); ++i)
		{
			FCDAnimationCurveTrackList& curves = animated->GetCurves()[i];
			for (FCDAnimationCurveTrackList::iterator itC = curves.begin(); itC != curves.end(); ++itC)
			{
				(*itC)->SetTargetElement(arrayElement);
				(*itC)->SetTargetQualifier(animated->GetQualifier(i));

				FCDAnimationChannel* channel = (*itC)->GetParent();
				FUAssert(channel != NULL, continue);
				channel->SetTargetPointer(target);
				if (!channels.contains(channel)) channels.push_back(channel);
			}
		}

		// Enforce the default values on the channels. This is used for curve merging.
		for (FCDAnimationChannelList::iterator itC = channels.begin(); itC != channels.end(); ++itC)
		{
			FCDAnimationChannelDefaultValueList channelDefaultValues;
			FCDAnimationChannel* channel = (*itC);

			for (size_t i = 0; i < animated->GetValueCount(); ++i)
			{
				FCDAnimationCurveTrackList& curves = animated->GetCurves()[i];

				// Find the curve, if any, that comes from this channel.
				FCDAnimationCurve* curve = NULL;
				for (size_t j = 0; j < curves.size() && curve == NULL; ++j)
				{
					if (curves[j]->GetParent() == channel)
					{
						curve = curves[j];
						break;
					}
				}

				float defaultValue = *animated->GetValue(i);
				const char* qualifier = animated->GetQualifier(i).c_str();
				channelDefaultValues.push_back(FCDAnimationChannelDefaultValue(curve, defaultValue, qualifier));
			}

			(*itC)->SetDefaultValues(channelDefaultValues);
		}
	}
}


//
// FCDVersion
//

void FCDVersion::ParseVersionNumbers(const fm::string& _v)
{
	fm::string v = _v;
	major = (uint8)FUStringConversion::ToUInt32(v);
	v.erase(0, min(v.find('.'), v.size()-1)+1);
	minor = (uint8)FUStringConversion::ToUInt32(v);
	v.erase(0, min(v.find('.'), v.size()-1)+1);
	revision = (uint8)FUStringConversion::ToUInt32(v);
}


bool FCDVersion::Is(const fm::string& v) const
{
	FCDVersion version;
	version.ParseVersionNumbers(v);
	if (major == version.major && minor == version.minor && revision == version.revision)
		return true;
	
	return false;
}


bool FCDVersion::IsLowerThan(const fm::string& v) const
{
	FCDVersion version;
	version.ParseVersionNumbers(v);
	if (major < version.major) return true;
	if (major > version.major) return false;
	if (minor < version.minor) return true;
	if (minor > version.minor) return false;
	if (revision < version.revision) return true;
	return false;
}

bool FCDVersion::IsHigherThan(const fm::string& v) const
{
	if (!IsLowerThan(v) && !Is(v))
		return true;
	return false;
}

// More linker-tricking for DLL support.
extern bool CreateLibraryFunctions(FCDocument* doc)
{
	doc->GetForceFieldLibrary()->GetEntityCount();
	doc->GetForceFieldLibrary()->GetEntity(0);
	doc->GetImageLibrary()->GetEntityCount();
	doc->GetImageLibrary()->GetEntity(0);
	doc->GetPhysicsMaterialLibrary()->GetEntityCount();
	doc->GetPhysicsMaterialLibrary()->GetEntity(0);
	doc->GetPhysicsModelLibrary()->GetEntityCount();
	doc->GetPhysicsModelLibrary()->GetEntity(0);
	return true;
}
