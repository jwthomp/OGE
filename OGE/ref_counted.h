#ifndef __REF_COUNTED_H_
#define __REF_COUNTED_H_

#include "core_types.h"

template<class T, uint32 T_MAX_NAME_LEN>
class ref_count_store
{
public:
	T m_data;
	char m_name[T_MAX_NAME_LEN];
	uint32 m_reference_count;
};

template<class T, uint32 T_MAX_NAME_LEN, typename T_ALLOCATOR>
class ref_counted
{
public:
	void init(uint32 p_elements_max);
	void shutdown();

	T* create(char p_name[T_MAX_NAME_LEN]);
	void release(T* p_obj);

	uint32 get_allocated_count();
	T * get_from_index(uint32 p_index);

private:
	static bool find(ref_count_store<T, T_MAX_NAME_LEN> const* p_obj_a, ref_count_store<T, T_MAX_NAME_LEN> const* p_obj_b);

	T_ALLOCATOR m_allocator;
	uint32 m_elements_max;
};

#include "ref_counted.inl"

#endif /* __REF_COUNTED_H_ */