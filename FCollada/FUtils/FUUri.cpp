/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUtils/FUDaeWriter.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUUri.h"

FUUri::FUUri()
{
}

FUUri::FUUri(const fm::string& uri)
{
	size_t index = uri.find('#');
	if (index == fm::string::npos) prefix = TO_FSTRING(uri);
	else
	{
		prefix = TO_FSTRING(uri.substr(0, index));
		suffix = uri.substr(index + 1);
		suffix = FUDaeWriter::CleanId(suffix);
	}
}

#ifdef UNICODE
FUUri::FUUri(const fstring& uri)
{
	size_t index = uri.find('#');
	if (index == fstring::npos) prefix = TO_FSTRING(uri);
	else
	{
		prefix = uri.substr(0, index);
		suffix = TO_STRING(uri.substr(index + 1));
		suffix = FUDaeWriter::CleanId(suffix);
	}
}
#endif // UNICODE
