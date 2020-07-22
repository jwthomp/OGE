#ifndef __SG_MAIN_SCENE_H_
#define __SG_MAIN_SCENE_H_

#include "scene_base.h"

class sg_main_scene : public scene_base
{
public:
	void init();
	
	void process(real p_frametime);
	
private:
	void process_input(real p_frametime);
	real m_xvel;
	float m_xrot;
	float m_yrot;
	
};

#endif /* __SG_MAIN_SCENE_H_ */