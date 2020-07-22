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
#include "FUtils/FUStringConversion.h"
#ifndef MAC_TIGER
#include "FUtils/FUStringConversion.hpp"
#endif // MAC_TIGER

//
// FUStringConversion
//

// Convert a UTF-8 string to a fstring
#ifdef UNICODE
	const fstring& FUStringConversion::ToFString(const char* value)
	{
		globalBuilder.clear();
		uint32 length = (uint32) strlen(value);
		globalBuilder.reserve(length + 1);
		for (uint32 i = 0; i < length; ++i)
		{
			globalBuilder.append((fchar)value[i]);
		}
		return globalBuilder.ToString();
	}
#else // UNICODE
	const fstring& FUStringConversion::ToFString(const char* value)
	{
		static fstring _out;
		_out = value;
		return _out;
	}
#endif // UNICODE

// Convert a fstring string to a UTF-8 string
#ifdef UNICODE
	const fm::string& FUStringConversion::ToString(const fchar* value)
	{
		globalSBuilder.clear();
		uint32 length = (uint32) fstrlen(value);
		globalSBuilder.reserve(length + 1);
		for (uint32 i = 0; i < length; ++i)
		{
			if (value[i] < 0xFF || (value[i] & (~0xFF)) >= 32) globalSBuilder.append((char)value[i]);
			else globalSBuilder.append('_'); // some generic enough character
		}
		return globalSBuilder.ToString();
	}
#else // UNICODE
	const fm::string& FUStringConversion::ToString(const fchar* value)
	{
		static fm::string _out;
		_out = value;
		return _out;
	}
#endif // UNICODE

// Convert a matrix to a string
void FUStringConversion::ToString(FUSStringBuilder& builder, const FMMatrix44& m)
{
	VAL(m[0][0]); SPACE; VAL(m[1][0]); SPACE; VAL(m[2][0]); SPACE; VAL(m[3][0]); SPACE;
	VAL(m[0][1]); SPACE; VAL(m[1][1]); SPACE; VAL(m[2][1]); SPACE; VAL(m[3][1]); SPACE;
	VAL(m[0][2]); SPACE; VAL(m[1][2]); SPACE; VAL(m[2][2]); SPACE; VAL(m[3][2]); SPACE;
	VAL(m[0][3]); SPACE; VAL(m[1][3]); SPACE; VAL(m[2][3]); SPACE; VAL(m[3][3]);
}

const fm::string& FUStringConversion::ToString(const FMMatrix44& m)
{
	globalSBuilder.clear();
	ToString(globalSBuilder, m);
	return globalSBuilder.ToString();
}


const char* FUStringConversion::ToString(const FUDateTime& dateTime)
{
	static char sz[21];
	snprintf(sz, 21, "%04u-%02u-%02uT%02u:%02u:%02uZ", dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetDay(), dateTime.GetHour(), dateTime.GetMinutes(), dateTime.GetSeconds());
	sz[20] = 0;
	return sz;
}

// Convert a matrix to a fstring
void FUStringConversion::ToFString(FUStringBuilder& builder, const FMMatrix44& m)
{
	VAL(m[0][0]); FSPACE; VAL(m[1][0]); FSPACE; VAL(m[2][0]); FSPACE; VAL(m[3][0]); FSPACE;
	VAL(m[0][1]); FSPACE; VAL(m[1][1]); FSPACE; VAL(m[2][1]); FSPACE; VAL(m[3][1]); FSPACE;
	VAL(m[0][2]); FSPACE; VAL(m[1][2]); FSPACE; VAL(m[2][2]); FSPACE; VAL(m[3][2]); FSPACE;
	VAL(m[0][3]); FSPACE; VAL(m[1][3]); FSPACE; VAL(m[2][3]); FSPACE; VAL(m[3][3]);
}

const fstring& FUStringConversion::ToFString(const FMMatrix44& m)
{
	globalBuilder.clear();
	ToFString(globalBuilder, m);
	return globalBuilder.ToString();
}

const fchar* FUStringConversion::ToFString(const FUDateTime& dateTime)
{
	static fchar sz[21];
	fsnprintf(sz, 21, FC("%04u-%02u-%02uT%02u:%02u:%02uZ"), dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetDay(), dateTime.GetHour(), dateTime.GetMinutes(), dateTime.GetSeconds());
	sz[20] = 0;
	return sz;
}

#ifdef HAS_VECTORTYPES

// Split a fstring into multiple substrings
void FUStringConversion::ToFStringList(const fstring& value, FStringList& array)
{
	const fchar* s = value.c_str();

	// Skip beginning white spaces
	fchar c;
	while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }

	size_t index = 0;
	while (*s != 0)
	{
		const fchar* word = s;

		// Find next white space
		while ((c = *s) != 0 && c != ' ' && c != '\t' && c != '\r' && c != '\n') { ++s; }

		if (index < array.size()) array[index++].append(word, s - word);
		else { array.push_back(fstring(word, s - word)); ++index; }

		// Skip all white spaces
		while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }
	}
	array.resize(index);
}

void FUStringConversion::ToStringList(const char* s, StringList& array)
{
	// Skip beginning white spaces
	char c;
	while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }

	size_t index = 0;
	while (*s != 0)
	{
		const char* word = s;

		// Find next white space
		while ((c = *s) != 0 && c != ' ' && c != '\t' && c != '\r' && c != '\n') { ++s; }

		if (index < array.size()) array[index++].append(word, s - word);
		else { array.push_back(fm::string(word, s - word)); ++index; }

		// Skip all white spaces
		while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }
	}
	array.resize(index);
}

#ifdef UNICODE
void FUStringConversion::ToStringList(const fchar* value, StringList& array)
{
	// Performance could be improved...
	ToStringList(ToString(value), array);
}
#endif // UNICODE

#endif // HAS_VECTORTYPES

// Convert a 2D point to a string
void FUStringConversion::ToString(FUSStringBuilder& builder, const FMVector2& p)
{
	VAL(p.u); SPACE; VAL(p.v);
}

void FUStringConversion::ToFString(FUStringBuilder& builder, const FMVector2& p)
{
	VAL(p.u); FSPACE; VAL(p.v);
}

const fm::string& FUStringConversion::ToString(const FMVector2& p)
{
	globalSBuilder.clear();
	ToString(globalSBuilder, p);
	return globalSBuilder.ToString();
}

const fstring& FUStringConversion::ToFString(const FMVector2& p)
{
	globalBuilder.clear();
	ToFString(globalBuilder, p);
	return globalBuilder.ToString();
}

// Convert a point to a string
void FUStringConversion::ToString(FUSStringBuilder& builder, const FMVector3& p)
{
	VAL(p.x); SPACE; VAL(p.y); SPACE; VAL(p.z);
}

const fm::string& FUStringConversion::ToString(const FMVector3& p)
{
	globalSBuilder.clear();
	ToString(globalSBuilder, p);
	return globalSBuilder.ToString();
}

// Convert a vector4 to a string
void FUStringConversion::ToString(FUSStringBuilder& builder, const FMVector4& p)
{
	VAL(p.x); SPACE; VAL(p.y); SPACE; VAL(p.z); SPACE; VAL(p.w);
}

void FUStringConversion::ToFString(FUStringBuilder& builder, const FMVector4& p)
{
	VAL(p.x); FSPACE; VAL(p.y); FSPACE; VAL(p.z); FSPACE; VAL(p.w);
}

const fm::string& FUStringConversion::ToString(const FMVector4& p)
{
	globalSBuilder.clear();
	ToString(globalSBuilder, p);
	return globalSBuilder.ToString();
}

const fstring& FUStringConversion::ToFString(const FMVector4& p)
{
	globalBuilder.clear();
	ToFString(globalBuilder, p);
	return globalBuilder.ToString();
}


// Convert a point to a fstring
void FUStringConversion::ToFString(FUStringBuilder& builder, const FMVector3& p)
{
	VAL(p.x); FSPACE; VAL(p.y); FSPACE; VAL(p.z);
}

const fstring& FUStringConversion::ToFString(const FMVector3& p)
{
	globalBuilder.clear();
	ToFString(globalBuilder, p);
	return globalBuilder.ToString();
}

#ifdef HAS_VECTORTYPES
void FUStringConversion::ToString(FUSStringBuilder& builder, const FloatList& values)
{
	if (values.empty()) return;
	if (!builder.empty()) SPACE;
	FloatList::const_iterator itV = values.begin();
	builder.append(*itV);
	for (++itV; itV != values.end(); ++itV) { SPACE; VAL(*itV); }
}

void FUStringConversion::ToString(FUSStringBuilder& builder, const Int32List& values)
{
	if (values.empty()) return;
	if (!builder.empty()) SPACE;
	Int32List::const_iterator itV = values.begin();
	builder.append(*itV);
	for (++itV; itV != values.end(); ++itV) { SPACE; VAL(*itV); }
}

void FUStringConversion::ToString(FUSStringBuilder& builder, const UInt32List& values)
{
	if (values.empty()) return;
	if (!builder.empty()) SPACE;
	UInt32List::const_iterator itV = values.begin();
	builder.append(*itV);
	for (++itV; itV != values.end(); ++itV) { SPACE; VAL(*itV); }
}

#endif // HAS_VECTORTYPES

// Parasitic: Common string operators
#ifdef UNICODE
const fstring& operator+(const fstring& sz1, int32 i)
{
	globalBuilder.set(sz1);
	globalBuilder.append(i);
	return globalBuilder.ToString();
}
#endif // UNICODE

const fm::string& operator+(const fm::string& sz1, int32 i)
{
	globalSBuilder.set(sz1);
	globalSBuilder.append(i);
	return globalSBuilder.ToString();
}

// Called by TrickLinker2 in FUStringBuilder.cpp
extern void TrickLinkerFUStringConversion(void)
{
	// Exercise the template functions in order to force the linker to generate and expose the methods.
	const char* c = emptyCharString;
	const fchar* fc = emptyFCharString;
	float f = FUStringConversion::ToFloat(&c);
	f = FUStringConversion::ToFloat(&fc);
	bool b = FUStringConversion::ToBoolean(c);
	b = FUStringConversion::ToBoolean(fc);
	int32 i32 = FUStringConversion::ToInt32(&c);
	i32 = FUStringConversion::ToInt32(&fc);
	uint32 u32 = FUStringConversion::ToUInt32(&c);
	u32 = FUStringConversion::ToUInt32(&fc);
	u32 = FUStringConversion::HexToUInt32(&c);
	u32 = FUStringConversion::HexToUInt32(&fc);
	FMMatrix44 m44;
	FUStringConversion::ToMatrix(&c, m44);
	FUStringConversion::ToMatrix(&fc, m44);
	FUDateTime dt;
	FUStringConversion::ToDateTime(c, dt);
	FUStringConversion::ToDateTime(fc, dt);
	FMVector2 f2 = FUStringConversion::ToVector2(&c);
	f2 = FUStringConversion::ToVector2(&fc);
	FMVector3 f3 = FUStringConversion::ToVector3(&c);
	f3 = FUStringConversion::ToVector3(&fc);
	FMVector4 f4 = FUStringConversion::ToVector4(&c);
	f4 = FUStringConversion::ToVector4(&fc);

	BooleanList bl;
	FUStringConversion::ToBooleanList(c, bl);
	FUStringConversion::ToBooleanList(fc, bl);
	Int32List il;
	FUStringConversion::ToInt32List(c, il);
	FUStringConversion::ToInt32List(fc, il);
	UInt32List ul;
	FUStringConversion::ToUInt32List(c, ul);
	FUStringConversion::ToUInt32List(fc, ul);
	FloatList fl;
	FUStringConversion::ToFloatList(c, fl);
	FUStringConversion::ToFloatList(fc, fl);
	FMVector2List f2l;
	FUStringConversion::ToVector2List(c, f2l);
	FUStringConversion::ToVector2List(fc, f2l);
	FMVector3List f3l;
	FUStringConversion::ToVector3List(c, f3l);
	FUStringConversion::ToVector3List(fc, f3l);
	FMVector4List f4l;
	FUStringConversion::ToVector4List(c, f4l);
	FUStringConversion::ToVector4List(fc, f4l);
	FMMatrix44List m44l;
	FUStringConversion::ToMatrixList(c, m44l);
	FUStringConversion::ToMatrixList(fc, m44l);
	fm::pvector<FloatList> pfl;
	FUStringConversion::ToInterleavedFloatList(c, pfl);
	FUStringConversion::ToInterleavedFloatList(fc, pfl);
}