#ifndef __MESH_META_DATA
#define __MESH_META_DATA

#define MAX_META_NAME_LENGTH (64)

#include "vector3.h"

#include <map>
#include <string>


class mesh_meta_data
{
public:
	void data_add(char const* p_name, Vector3 const *p_position);
	bool data_exists(char const* p_name);
	Vector3 const *data_get(char const* p_name);

private:
	std::map<std::string, Vector3> m_data;
};

#endif /* __MESH_META_DATA */