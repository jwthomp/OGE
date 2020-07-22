
#include <string.h>

template<class T, uint32 T_MAX_NAME_LEN, class T_ALLOCATOR>
bool ref_counted<T, T_MAX_NAME_LEN, T_ALLOCATOR>::find(ref_count_store<T, T_MAX_NAME_LEN> const* p_obj_a, ref_count_store<T, T_MAX_NAME_LEN> const* p_obj_b)
{
	if (!strncmp(p_obj_a->m_name, p_obj_b->m_name, T_MAX_NAME_LEN)) {
		return true;
	}

	return false;
}

template<class T, uint32 T_MAX_NAME_LEN, class T_ALLOCATOR>
void ref_counted<T, T_MAX_NAME_LEN, T_ALLOCATOR>::init(uint32 p_elements_max)
{
	m_allocator.init(p_elements_max);
	m_elements_max = p_elements_max;
}

template<class T, uint32 T_MAX_NAME_LEN, class T_ALLOCATOR>
void ref_counted<T, T_MAX_NAME_LEN, T_ALLOCATOR>::shutdown()
{
	m_allocator.shutdown();
}

template<class T, uint32 T_MAX_NAME_LEN, class T_ALLOCATOR>
T* ref_counted<T, T_MAX_NAME_LEN, T_ALLOCATOR>::create(char p_name[T_MAX_NAME_LEN])
{
	// Do we already have this texture loaded?
	// If so, increment reference count and return
	ref_count_store<T, T_MAX_NAME_LEN> tmp_store;
	strncpy(tmp_store.m_name, p_name, m_elements_max);
	ref_count_store<T, T_MAX_NAME_LEN> *ref_store = m_allocator.element_find(&tmp_store, find);

	if (ref_store != NULL) {
		ref_store->m_reference_count++;
		return &ref_store->m_data;
	}
		
	ref_store = m_allocator.allocate();
	if (ref_store == NULL) {
		return NULL;
	}

	strncpy(ref_store->m_name, p_name, T_MAX_NAME_LEN);
	ref_store->m_reference_count = 1;
	
	return (T *)&ref_store->m_data;
}

template<class T, uint32 T_MAX_NAME_LEN, class T_ALLOCATOR>
void ref_counted<T, T_MAX_NAME_LEN, T_ALLOCATOR>::release(T* p_obj)
{
}

template<class T, uint32 T_MAX_NAME_LEN, class T_ALLOCATOR>
uint32 ref_counted<T, T_MAX_NAME_LEN, T_ALLOCATOR>::get_allocated_count()
{
	return m_allocator.get_allocated_count();;
}

template<class T, uint32 T_MAX_NAME_LEN, class T_ALLOCATOR>
T * ref_counted<T, T_MAX_NAME_LEN, T_ALLOCATOR>::get_from_index(uint32 p_index)
{
	ref_count_store<T, T_MAX_NAME_LEN> *ref_store = m_allocator.get_from_index(p_index);
	if (ref_store) {
		ref_store->m_reference_count++;
		return (T *)&ref_store->m_data;
	}

	return NULL;
}