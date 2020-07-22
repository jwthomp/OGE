#ifndef __CAMERA_H_
#define __CAMERA_H_

#include "matrix.h"

class camera
{
public:
	camera();
	~camera();

	void set_perspective() const;
	void set_transform(matrix44 const* p_transform);
	matrix44 const* get_transform() const;

private:

	matrix44 m_transform;
};

#endif /* __CAMERA_H_ */