/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "FUtils/FUDateTime.h"
#include <limits>

//
// FUStringConversion template functions
//

// Some string writer macros
#define SPACE builder.append(' ')
#define FSPACE builder.append((fchar) ' ');
#define VAL(x) builder.append(x)

// Convert a fstring to a boolean value
template<class CH> FCOLLADA_EXPORT bool FUStringConversion::ToBoolean(const CH* value)
{
	return value != NULL && *value != 0 && *value != '0' && *value != 'f' && *value != 'F';
}

// Convert a fstring to a int32 and advance the character pointer
template <class CH> FCOLLADA_EXPORT int32 FUStringConversion::ToInt32(const CH** value)
{
	if (!*value) return 0;

	// Skip beginning white spaces
	const CH* s = *value;
	CH c;
	while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }

	int32 val = 0;
	int32 sign = 1;
	if (*s == '-') { ++s; sign = -1; }

	while ((c = *s) != 0)
	{
		if (c >= '0' && c <= '9') val = val * 10 + c - '0';
		else break;
		++s;
	}
	val *= sign;
	while ((c = *s) != '\0' && (c != ' ' && c != '\t' && c != '\n')) s++;
	while ((c = *s) != '\0' && (c == ' ' || c == '\t' || c == '\n')) s++;
	*value = s;
	return val;
}

// Convert a fstring to a float and advance the character pointer
template<class CH> FCOLLADA_EXPORT float FUStringConversion::ToFloat(const CH** value)
{
	const CH* s = *value;
	if (s == NULL || *s == 0) return 0.0f;

	// Skip beginning white spaces
	CH c;
	while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }

	// Skip all the plausible numerical characters
	double val = 0.0;
	int sign = 1;
	if (*s == '-') { ++s; sign = -1; }
	int32 decimals = 0;
	int32 exponent = 0;
	bool infinity = false;
	bool nonValidFound = false;
	while ((c = *s) != 0 && !nonValidFound)
	{
		switch(c)
		{
		case '.': decimals = 1; break;
		case '0': val *= 10.0; decimals *= 10; break;
		case '1': val = val * 10.0 + 1; decimals *= 10; break;
		case '2': val = val * 10.0 + 2; decimals *= 10; break;
		case '3': val = val * 10.0 + 3; decimals *= 10; break;
		case '4': val = val * 10.0 + 4; decimals *= 10; break;
		case '5': val = val * 10.0 + 5; decimals *= 10; break;
		case '6': val = val * 10.0 + 6; decimals *= 10; break;
		case '7': val = val * 10.0 + 7; decimals *= 10; break;
		case '8': val = val * 10.0 + 8; decimals *= 10; break;
		case '9': val = val * 10.0 + 9; decimals *= 10; break;
		case 'e':
		case 'E': ++s; exponent = ToInt32(&s); --s;
		case 'I': nonValidFound = true; infinity = true; --s; break;
		default: nonValidFound = true; --s; break;
		}
		++s;
	}

	if (infinity) // test for infinity
	{
		infinity = false;
		if ((*s=='I') && (*(++s) == 'N') && (*(++s)=='F'))
		{
			infinity = true;
			decimals=0;
			val = std::numeric_limits<double>::infinity() * (double)sign;
		}
	}
	if (!infinity)
	{
		// Generate the value
		if (decimals != 0) val /= (double)decimals;
		val *= (double)sign;
		if (exponent != 0) val *= pow(10.0, (double) exponent);
	}

	// Find next whitespaces and Skip end whitespaces
	while ((c = *s) != 0 && c != ' ' && c != '\t' && c != '\r' && c != '\n') { ++s; }
	while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }

	*value = s;
	return (float)val;
}

// Convert a fstring to a uint32 and advance the character pointer
template<class CH> FCOLLADA_EXPORT uint32 FUStringConversion::ToUInt32(const CH** value)
{
	if (value == NULL || *value == NULL || **value == 0) return 0;

	// Skip beginning white spaces
	const CH* s = *value;
	CH c;
	while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }

	uint32 val = 0;
	while ((c = *s) != 0)
	{
		if (c >= '0' && c <= '9') val = val * 10 + c - '0';
		else break;
		++s;
	}

	while ((c = *s) != '\0' && (c != ' ' && c != '\t' && c != '\n')) s++;
	while ((c = *s) != '\0' && (c == ' ' || c == '\t' || c == '\n')) s++;
	*value = s;
	return val;
}

template<class CH> FCOLLADA_EXPORT uint32 FUStringConversion::HexToUInt32(const CH** value, uint32 count)
{
	if (value == NULL || *value == NULL || **value == 0) return 0;

	const CH* s = *value;
	CH c; 

	// Skip any '0x' prefix.
	if (*s == '0' && (*(s+1) == 'x' || *(s+1) == 'X')) s += 2;

	uint32 val = 0;
	for (uint32 i = 0; i < count && (c = *s) != 0; ++i)
	{
		if (c >= '0' && c <= '9') val = val * 16 + c - '0';
		else if (c >= 'A' && c <= 'F') val = val * 16 + c + 10 - 'A';
		else if (c >= 'a' && c <= 'f') val = val * 16 + c + 10 - 'a';
		else break;
		++s;
	}

	*value = s;
	return val;
}

template<class CH> FCOLLADA_EXPORT void FUStringConversion::ToMatrix(const CH** s, FMMatrix44& mx)
{
	if (s != NULL && *s != NULL && **s != 0)
	{
		// COLLADA is Column major 
		mx[0][0] = ToFloat(s); mx[1][0] = ToFloat(s); mx[2][0] = ToFloat(s); mx[3][0] = ToFloat(s);
		mx[0][1] = ToFloat(s); mx[1][1] = ToFloat(s); mx[2][1] = ToFloat(s); mx[3][1] = ToFloat(s);
		mx[0][2] = ToFloat(s); mx[1][2] = ToFloat(s); mx[2][2] = ToFloat(s); mx[3][2] = ToFloat(s);
		mx[0][3] = ToFloat(s); mx[1][3] = ToFloat(s); mx[2][3] = ToFloat(s); mx[3][3] = ToFloat(s);
	}
}

template<class CH> FCOLLADA_EXPORT void FUStringConversion::ToDateTime(const CH* value, FUDateTime& dateTime)
{
	// Manually calculate the length in order to avoid the strlen/fstrlen nightmare.
	size_t length;
	const CH* c = value;
	for (length = 0; *c != 0; ++length) ++c;

	if (length == 20)
	{
		dateTime.SetYear(ToUInt32(value)); value += 5;
		dateTime.SetMonth(ToUInt32(value)); value += 3;
		dateTime.SetDay(ToUInt32(value)); value += 3;
		dateTime.SetHour(ToUInt32(value)); value += 3;
		dateTime.SetMinutes(ToUInt32(value)); value += 3;
		dateTime.SetSeconds(ToUInt32(value));
	}
}

// Convert a fstring to a (X,Y,Z) Point object
template<class CH> FCOLLADA_EXPORT FMVector2 FUStringConversion::ToVector2(const CH** value)
{
	FMVector2 p;
	if (value != NULL && *value != NULL && **value != 0)
	{
		p.x = ToFloat(value);
		p.y = ToFloat(value);
	}
	return p;
}

template<class CH> FCOLLADA_EXPORT FMVector3 FUStringConversion::ToVector3(const CH** value)
{
	FMVector3 p;
	if (value != NULL && *value != NULL && **value != 0)
	{
		p.x = ToFloat(value);
		p.y = ToFloat(value);
		p.z = ToFloat(value);
	}
	return p;
}

template<class CH> FCOLLADA_EXPORT FMVector4 FUStringConversion::ToVector4(const CH** value)
{
	FMVector4 p;
	if (value != NULL && *value != NULL && **value != 0)
	{
		p.x = ToFloat(value);
		p.y = ToFloat(value);
		p.z = ToFloat(value);
		
		// If the fourth component is not provided, default to 1.0f.
		p.w = (*value != NULL && **value != 0) ? ToFloat(value) : 1.0f;
	}
	return p;
}

#ifdef HAS_VECTORTYPES

template<class CH> FCOLLADA_EXPORT void FUStringConversion::ToBooleanList(const CH* value, BooleanList& array)
{
	const CH* s = value;
	CH c = *s;
	array.clear();

	// Start by eating all the white spaces.
	while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }

	while (*s != 0)
	{
		// Process this value
		array.push_back(ToBoolean(s));

		// Find next white space
		while ((c = *s) != 0 && c != ' ' && c != '\t' && c != '\r' && c != '\n') { ++s; }

		// Skip all the white spaces.
		while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }
	}
}

// Convert a fstring to a 32-bit integer list
template<class CH> FCOLLADA_EXPORT void FUStringConversion::ToInt32List(const CH* value, Int32List& array)
{
	size_t length = 0;
	if (value != NULL && *value != 0)
	{ 
		size_t oldLength = array.size();
		for (; length < oldLength && *value != 0; ++length) array[length] = ToInt32(&value);		
		while (*value != 0) { array.push_back(ToInt32(&value)); ++length; }
	}
	array.resize(length);
}

// Convert a fstring to a 32-bit unsigned integer list
template<class CH> FCOLLADA_EXPORT void FUStringConversion::ToUInt32List(const CH* value, UInt32List& array)
{
	size_t length = 0;
	if (value != NULL && *value != 0)
	{ 
		size_t oldLength = array.size();
		for (; length < oldLength && *value != 0; ++length) array[length] = ToUInt32(&value); 
		while (*value != 0) { array.push_back(ToUInt32(&value)); ++length; }
	}
	array.resize(length);
}

// Convert a fstring to 32-bit floating point list
template<class CH> FCOLLADA_EXPORT void FUStringConversion::ToFloatList(const CH* value, FloatList& array)
{
	size_t length = 0;
	if (value != NULL && *value != 0)
	{ 
		size_t oldLength = array.size();
		for (; length < oldLength && *value != 0; ++length) array[length] = ToFloat(&value); 
		while (*value != 0) { array.push_back(ToFloat(&value)); ++length; }
	}
	array.resize(length);
}

// Convert a fstring to a list of interleaved floating points
template<class CH> FCOLLADA_EXPORT void FUStringConversion::ToInterleavedFloatList(const CH* value, fm::pvector<FloatList>& arrays)
{
	size_t stride = arrays.size();
	size_t validCount = 0;
	if (value != NULL && *value != 0 && stride > 0)
	{ 
		size_t length = arrays[0]->size();
		for (size_t count = 0; count < length && *value != 0; ++count, ++validCount)
		{
			for (size_t i = 0; i < stride && *value != 0; ++i)
			{
				FloatList* array = arrays[i];
				if (array != NULL) array->at(count) = ToFloat(&value);
				else ToFloat(&value);
			}
		}
		
		while (*value != 0)
		{
			for (size_t i = 0; i < stride && *value != 0; ++i)
			{
				FloatList* array = arrays[i];
				if (array != NULL) array->push_back(ToFloat(&value));
				else ToFloat(&value);
			}
			if (*value != 0) ++validCount;
		}
	}

	for (size_t i = 0; i < stride; ++i)
	{
		arrays[i]->resize(validCount);
	}
}

// Convert a string to a list of matrices
template<class CH> FCOLLADA_EXPORT void FUStringConversion::ToMatrixList(const CH* value, FMMatrix44List& array)
{
	size_t count = 0;
	if (value != NULL && *value != 0)
	{ 
		size_t length = array.size();
		for (; count < length && *value != 0; ++count)
		{
			ToMatrix(&value, array[count]);
		}
		
		while (*value != 0)
		{
			FMMatrix44List::iterator it = array.insert(array.end(), FMMatrix44::Identity);
			ToMatrix(&value, *it);
			++count;
		}
	}
	array.resize(count);
}

template<class CH> FCOLLADA_EXPORT void FUStringConversion::ToVector2List(const CH* value, FMVector2List& array)
{
	size_t count = 0;
	if (value != NULL && *value != 0)
	{ 
		size_t length = array.size();
		for (; count < length && *value != 0; ++count)
		{
			array[count] = ToVector2(&value);
		}
		
		while (*value != 0) { array.push_back(ToVector2(&value)); ++count; }
	}
	array.resize(count);
}

template<class CH> FCOLLADA_EXPORT void FUStringConversion::ToVector3List(const CH* value, FMVector3List& array)
{
	size_t count = 0;
	if (value != NULL && *value != 0)
	{ 
		size_t length = array.size();
		for (; count < length && *value != 0; ++count)
		{
			array[count] = ToVector3(&value);
		}
		
		while (*value != 0) { array.push_back(ToVector3(&value)); ++count; }
	}
	array.resize(count);
}

template<class CH> FCOLLADA_EXPORT void FUStringConversion::ToVector4List(const CH* value, FMVector4List& array)
{
	size_t count = 0;
	if (value != NULL && *value != 0)
	{ 
		size_t length = array.size();
		for (; count < length && *value != 0; ++count)
		{
			array[count] = ToVector4(&value);
		}
		
		while (*value != 0) { array.push_back(ToVector4(&value)); ++count; }
	}
	array.resize(count);
}

#endif // HAS_VECTORTYPES
