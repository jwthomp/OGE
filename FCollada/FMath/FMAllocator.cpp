/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMath/FMAllocator.h"

namespace fm
{
	// These two are simple enough, but have the advantage of
	// always allocating/releasing memory from the same heap.
	void* Allocate(size_t byteCount)
	{
		return malloc(byteCount);
	}

	void Release(void* buffer)
	{
		free(buffer);
	}
};

