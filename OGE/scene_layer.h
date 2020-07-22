#ifndef __SCENE_LAYER_H_
#define __SCENE_LAYER_H_

#include "core_types.h"
#include "input_state.h"

union input_event;

class scene_layer
{
public:
	virtual void process(real p_frametime);

	virtual void process_event(input_event const* p_event, real p_frametime);
	virtual bool should_pass_event_to_scene(input_event const* p_event, bool *p_allow_scene_to_process) const;
	
	scene_layer *m_next;
};

//scene_layer *scene_layer_create();
//void scene_layer_destroy(scene_layer *p_scene_layer);

#endif /* __SCENE_LAYER_H_ */

