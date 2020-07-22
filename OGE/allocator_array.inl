#include <stdio.h>
#include <stdlib.h>
#include <new.h>

template <class T>
void allocator_array<T>::init(uint32 p_max_elements)
{
	m_elements_max = p_max_elements;
	m_allocated_count = 0;
	m_array_data = (array_element *)malloc(sizeof(array_element) * p_max_elements);
	memset(m_array_data, 0, sizeof(array_element) * p_max_elements);
}

template <class T>
void allocator_array<T>::shutdown()
{
	// Loop through each element and deallocate it
	for (uint32 i = 0; i < m_elements_max; ++i) {
		if (m_array_data[i].m_used == true) {
			(&m_array_data[i].m_data)->T::~T();
		}
	}

	free(m_array_data);
	m_array_data = NULL;
	m_elements_max = 0;
}

template <class T>
T* allocator_array<T>::allocate()
{
	// Find free buffer
	for (uint32 i = 0; i < m_elements_max; ++i) {
		if (m_array_data[i].m_used == false) {
				// construct it
				new(&m_array_data[i].m_data) T();
				
				m_array_data[i].m_used = true;
				m_allocated_count++;
				return &m_array_data[i].m_data;
		}
	}

	return NULL;
}

template <class T>
void allocator_array<T>::destroy(T* p_ptr)
{
	array_element *element = (array_element *)p_ptr;
	if (element->m_used == true) {
		element->m_used = false;
		(&element->m_data)->T::~T();
		m_allocated_count--;
	}
}

template <class T>
T* allocator_array<T>::element_find(T const* p_obj_data, bool (*p_find_func)(T const *p_obj_data_a, T const *p_obj_data_b))
{
	for (uint32 i = 0; i < m_elements_max; ++i) {
		if (p_find_func(p_obj_data, &m_array_data[i].m_data) == true) {
			return &m_array_data[i].m_data;
		}
	}

	return NULL;
}

template <class T>
uint32 allocator_array<T>::get_allocated_count()
{
	return m_allocated_count;
}

template <class T>
T * allocator_array<T>::get_from_index(uint32 p_index)
{
	if (p_index >= m_elements_max) {
		return NULL;
	}

	return &m_array_data[p_index].m_data;
}