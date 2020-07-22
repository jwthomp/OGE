/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/**
	@file FCDAsset.h
	This file contains the FCDAsset and FCDAssetContributor classes.
*/

#ifndef _FCD_ASSET_H_
#define _FCD_ASSET_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_
#ifndef _FU_DATETIME_H_
#include "FUtils/FUDateTime.h"
#endif // _FU_DATETIME_H_

/**
	A COLLADA asset declaration structure.

	In COLLADA, there are three types of assets.
	FCollada recognizes two.

	1) The COLLADA document is the most important asset and an
	asset declaration structure is always created for it.
	2) The FCDEntity objects may also contain assets structures.
	3) COLLADA also allows asset structure on entity libraries,
	but FCollada does not support them.

	Every asset contains its own list of contributors.
	Every COLLADA application and conditioner that modifies an asset
	should attach its signature, in the form of a contributor,
	to the asset.
	@see FCDAssetContributor

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDAsset : public FCDObject
{
private:
	DeclareObjectType(FCDObject);

	FUObjectContainer<class FCDAssetContributor> contributors;
	FUDateTime creationDateTime;
	FUDateTime modifiedDateTime;
	fstring keywords;
	fstring revision;
	fstring subject;
	fstring title;
	FMVector3 upAxis;

	// <unit>
	fstring unitName;
	float unitConversionFactor;

	// existence flags
	bool hasUnits, hasUpAxis;

public:
	/**	Constructor.
		@param document The COLLADA document that owns the asset. */
	FCDAsset(FCDocument* document);

	/** Destructor. */
	virtual ~FCDAsset();

	/** Retrieves the list of contributors to this asset.
		@return The list of contributors. */
	inline FCDAssetContributor** GetContributors() { return contributors.begin(); }
	inline const FCDAssetContributor** GetContributors() const { return contributors.begin(); } /**< See above. */

	/** Retrieves the number of contributors to this asset.
		@return The number of contributors. */
	inline size_t GetContributorCount() const { return contributors.size(); }
	
	/** Retrieves a contributor tied to this asset.
		@param index The index of the contributor.
		@return The contributor at the given index. */
	inline FCDAssetContributor* GetContributor(size_t index) { FUAssert(index < GetContributorCount(), return NULL); return contributors.at(index); }
	inline const FCDAssetContributor* GetContributor(size_t index) const { FUAssert(index < GetContributorCount(), return NULL); return contributors.at(index); } /**< See above. */
	
	/** Inserts a new contributor to this asset.
		@return An empty contributor structure. */
	inline FCDAssetContributor* AddContributor() { return contributors.Add(GetDocument()); SetDirtyFlag(); }

	/** Retrieves the creation date-time of the document.
		This date-time is not modifiable. It is set automatically
		when FCollada creates the document or imported when FCollada
		reads in a COLLADA document.
		@return The date-time at which the COLLADA document
			was originally created. */
	inline const FUDateTime& GetCreationDateTime() const { return creationDateTime; }
	
	/** Retrieves the last modification date-time of the document.
		This date-time is not modifiable. It is set automatically
		when FCollada writes out a COLLADA document to a file, when
		FCollada creates an original document and when FCollada reads in
		a COLLADA document from a file.
		@return The date-time at which the COLLADA document
			was last modified. */
	inline const FUDateTime& GetModifiedDateTime() const { return modifiedDateTime; }
	
	/** Retrieves the list of keywords identifying this asset.
		@return The list of keywords for the asset. */
	inline const fstring& GetKeywords() const { return keywords; }

	/** Sets the list of keywords for this asset.
		@param _keywords The list of keywords for the asset. */
	inline void SetKeywords(const fstring& _keywords) { keywords = _keywords; SetDirtyFlag(); }

	/** Retrieves the revision string for this asset.
		COLLADA doesn't define a standard for the revisions of assets.
		@return The revision string for the asset. */
	inline const fstring& GetRevision() const { return revision; }

	/** Sets the revision string for this asset.
		COLLADA doesn't define a standard for the revisions of assets.
		@param _revision The revision string for the asset. */
	inline void SetRevision(const fstring& _revision) { revision = _revision; SetDirtyFlag(); }

	/** Retrieves the subject of the asset.
		@return The subject of the asset. */
	inline const fstring& GetSubject() const { return subject; }

	/** Sets the subject of the asset.
		@param _subject The subject of the asset. */
	inline void SetSubject(const fstring& _subject) { subject = _subject; SetDirtyFlag(); }

	/** Retrieves the title of the asset.
		@return The title of the asset. */
	inline const fstring& GetTitle() const { return title; }
	
	/** Sets the title of the asset.
		@param _title The title of the asset. */
	inline void SetTitle(const fstring& _title) { title = _title; SetDirtyFlag(); }

	/** Retrieves the up-axis of the asset.
		The up-axis of two entities within the same document may differ.
		To avoid issues with up-axis differences, it is suggested that you use
		the FCDocumentTools::StandardizeUpAxisAndLength function.
		@return The up-axis of the asset. */
	inline const FMVector3& GetUpAxis() const { return upAxis; }

	/** Sets the up-axis of the asset.
		Changing the up-axis of an asset does not modify its data.
		@param _upAxis The up-axis of the asset. */
	inline void SetUpAxis(const FMVector3& _upAxis) { upAxis = _upAxis; hasUpAxis = true; SetDirtyFlag(); }

	/** Retrieves the name of the length unit for the asset.
		@return The name of the length unit for the asset. */
	inline const fstring& GetUnitName() const { return unitName; }

	/** Sets the name of the length unit for the asset.
		@param _unitName The name of the length unit for the asset. */
	inline void SetUnitName(const fstring& _unitName) { unitName = _unitName; SetDirtyFlag(); }

	/** Retrieves the length unit conversion factor, in meters, for the asset.
		The length unit of two entities within the same document may differ.
		To avoid issues with length unit differences, it is suggested that you use
		the FCDocumentTools::StandardizeUpAxisAndLength function.
		@return The length unit conversion factor. */
	inline float GetUnitConversionFactor() const { return unitConversionFactor; }

	/** Sets the length unit conversion factor for the asset.
		Changing the length unit conversion factor of an asset
		does not modify its data.
		@param factor The new length unit conversion factor. */
	inline void SetUnitConversionFactor(float factor) { unitConversionFactor = factor; hasUnits = true; SetDirtyFlag(); }

	/** Retrieves whether an up-axis is set for this asset.
		If no up-axis is set for this asset, you should
		use the up-axis of the parent's asset.
		@return Whether this asset defines an up-axis. */
	inline bool HasUpAxis() const { return hasUpAxis; }
	
	/** Retrieves whether a length unit is set for this asset.
		If no length unit is set for this asset, you should
		use the length unit of the parent's asset.
		@return Whether this asset defines a length unit. */
	inline bool HasUnits() const { return hasUnits; }

	/** Resets the up-axis of the asset.
		The parent asset up-axis should henceforth be used.
		Changing the up-axis of an asset does not modify its data. */
	inline void ResetUpAxis() { hasUpAxis = false; }
	
	/** Resets the length unit of the asset.
		The parent asset length unit should henceforth be used.
		Changing the length unit of an asset does not modify its data. */
	inline void ResetUnits() { hasUnits = false; }

	/** Clones the asset structure into another asset structure.
		@param clone The asset structure that will become the copy of this asset.
			When this pointer is NULL, a new asset structure will be created.
		@param cloneAllContributors Whether all the contributors of this asset
			should be copied into the clone.
		@return The clone. */
	FCDAsset* Clone(FCDAsset* clone = NULL, bool cloneAllContributors = true) const;

	/** [INTERNAL] Reads in the asset structure from the XML node tree.
		@param assetNode The asset structure XML tree node.
		@return Whether the asset structure parsing was successful. */
	bool LoadFromXML(xmlNode* assetNode);

	/** [INTERNAL] Writes out the asset structure to a XML node tree.
		@param parentNode The parent XML tree node.
		@return The created XML tree node. */
	xmlNode* WriteToXML(xmlNode* parentNode) const;
};

/**
	A COLLADA asset contributor.

	The asset contributor represent each step that the COLLADA document
	has taken, in terms of applications and conditioners, in order
	to get to its current state.

	Every COLLADA application and conditioner that modifies an asset
	should therefore attach its signature, in the form of a contributor,
	to the asset.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDAssetContributor : public FCDObject
{
private:
	DeclareObjectType(FCDObject);

	fstring author;
	fstring authoringTool;
	fstring comments;
	fstring copyright;
	fstring sourceData;

public:
	/** Constructor.
		@param document The COLLADA document that owns the contributor. */
	FCDAssetContributor(FCDocument* document);

	/** Destructor. */
	virtual ~FCDAssetContributor();

	/** Retrieves the name of the user that applied this contributor.
		@return The name of the user. */
	inline const fstring& GetAuthor() const { return author; }

	/** Sets the name of the user that applies the current contributor.
		It is suggested to use the following code snippet:
			const char* userName = getenv("USER");
			if (userName == NULL) userName = getenv("USERNAME");
			if (userName != NULL) contributor->SetAuthor(TO_FSTRING(userName));
		@param _author The name of the user. */
	inline void SetAuthor(const fstring& _author) { author = _author; SetDirtyFlag(); }

	/** Retrieves the name of the contributor.
		@return The name of the contributor. */
	inline const fstring& GetAuthoringTool() const { return authoringTool; }

	/** Sets the name of the current contributor.
		It is suggested that the version number of the contributor
		be included.
		@param _authoringTool The name of the contributor. */
	inline void SetAuthoringTool(const fstring& _authoringTool) { authoringTool = _authoringTool; SetDirtyFlag(); }

	/** Retrieves the contributor's comment about the asset.
		@return The contributor comments. */
	inline const fstring& GetComments() const { return comments; }

	/** Sets the contributor's comment about the asset.
		For document-level assets, it is suggested, for debugging
		purposes, to write down all the user-selected export options
		instead of actual user text input.
		@param _comments The contributor's comment. */
	inline void SetComments(const fstring& _comments) { comments = _comments; SetDirtyFlag(); }

	/** Retrieves the copyright information for the asset.
		@return The user copyright information for the asset. */
	inline const fstring& GetCopyright() const { return copyright; }

	/** Sets the copyright information for the asset.
		@param _copyright The user copyright information for the asset. */
	inline void SetCopyright(const fstring& _copyright) { copyright = _copyright; SetDirtyFlag(); }

	/** Retrieves the URI of the source data used by the contributor
		to generate the COLLADA asset.
		@return The URI of the source data. */
	inline const fstring& GetSourceData() const { return sourceData; }

	/** Sets the URI of the source data used by the contributor
		to generate the COLLADA asset.
		@param _sourceData The URI of the source data. */
	inline void SetSourceData(const fstring& _sourceData) { sourceData = _sourceData; SetDirtyFlag(); }

	/** Retrieves whether this contributor structure has any useful
		information.
		@return The validity of the contributor structure. */
	bool IsEmpty() const;

	/** Clones a contributor structure.
		@param clone The contributor structure that will become the copy
			of this contributor structure. When this pointer is NULL,
			a new contributor structure is created.
		@return The clone. */
	FCDAssetContributor* Clone(FCDAssetContributor* clone = NULL) const;

	/** [INTERNAL] Reads in the contributor structure from the XML node tree.
		@param contributorNode The XML tree node containing the contributor
			structure information.
		@return Whether the contributor structure was parsed in correctly. */
	bool LoadFromXML(xmlNode* contributorNode);

	/** [INTERNAL] Writes out the contributor structure into the given
		XML tree node.
		@param parentNode The XML tree node into which to write the contributor
			structure information.
		@return The created XML tree node for the contributor. */
	xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_ASSET_H_
