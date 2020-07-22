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

/**
	@file FUXmlNodeIdPair.h
	This file contains the FUXmlNodeIdPair class.
	Look forward to a fm::pair template, soon.
*/

#ifndef _FU_XML_NODE_ID_PAIR_H_
#define _FU_XML_NODE_ID_PAIR_H_

/**
	A pair of hashed XML node name and the actual xml node.
*/
class FUXmlNodeIdPair
{
public:
	FUCrc32::crc32 id; /**< The hash value of the XML tree node name. */
	xmlNode* node; /**< The XML tree node. */
};

typedef fm::vector<FUXmlNodeIdPair> FUXmlNodeIdPairList; /**< A dynamically-sized array of XML node+name pairs. */

#endif // _FU_XML_NODE_ID_PAIR_H_

