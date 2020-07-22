#include "scene_layer.h"

#include "input_event.h"

#pragma message(__FILE__ "    TODO: Implement scene_layer::process")
void scene_layer::process_event(input_event const* p_event, real p_frametime)
{
	if (m_next) {
		m_next->process_event(p_event, p_frametime);
	}
}

#pragma message(__FILE__ "    TODO: Implement scene_layer::should_pass_event_to_scene")
bool scene_layer::should_pass_event_to_scene(input_event const* p_event, bool *p_allow_scene_to_process) const
{
	return false;
}

void scene_layer::process(real p_frametime)
{
	if (m_next) {
		m_next->process(p_frametime);
	}
}




#if 0
scene_layer *scene_layer::scene_layer_create()
{
	return new scene_layer;
}

void scene_layer::scene_layer_destroy(scene_layer *p_scene_layer)
{
	delete p_scene_layer;
}
#endif