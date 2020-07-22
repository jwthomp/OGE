#ifndef __ALLOCATOR_ARRAY_H_
#define __ALLOCATOR_ARRAY_H_

#include "core_types.h"

template<class T>
class allocator_array
{
public:
	void init(uint32 p_max_elements);
	void shutdown();

	T* allocate();

	void destroy(T* p_ptr);

	uint32 get_allocated_count();
	T * get_from_index(uint32 p_index);

	T* element_find(T const* p_obj_data, bool (*p_find_func)(T const *p_obj_data_a, T const *p_obj_data_b));

private:
	class array_element
	{
	public:
		T m_data;
		bool m_used;
	};

	array_element* m_array_data;
	uint32 m_elements_max;
	uint32 m_allocated_count;
};

#include "allocator_array.inl"

#endif /* __ALLOCATOR_ARRAY_H_ */