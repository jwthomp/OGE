#ifndef __SCENE_BASE_H_
#define __SCENE_BASE_H_

#include "scene_layer.h"
#include "input_state.h"
#include "core_types.h"

union input_event;

class scene_base
{
public:
	void init();

	void clear_input_state();

	virtual void process(real p_frametime);
	virtual void process_event(input_event *p_event, real p_frametime);

	void scene_layer_add(scene_layer * p_scene_layer);
	
protected:
	scene_layer * m_scene_layers;
	input_state m_input_state;
	bool m_allow_scene_process;

private:
	bool should_pass_event_to_layer(input_event const* p_event) const;
	void handle_event(input_event const*p_event, real p_frametime);
};

#endif /* __SCENE_BASE_H_ */
