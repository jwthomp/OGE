#include "mesh_meta_data.h"

void mesh_meta_data::data_add(char const* p_name, Vector3 const *p_position)
{
	m_data[p_name] = *p_position;
}

Vector3 const *mesh_meta_data::data_get(char const* p_name)
{
	std::map<std::string, Vector3>::iterator it = m_data.find(p_name);
	
	if (it == m_data.end()) {
		return NULL;
	}

	return &(it->second);
}