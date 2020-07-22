#include "scene_manager.h"

#include <stdio.h>

static scene_base *g_current_scene;

void scene_manager_init()
{
	g_current_scene = NULL;
}

void scene_manager_shutdown()
{
}


scene_base *scene_manager_get_current()
{
	return g_current_scene;
}

void scene_manager_set_current(scene_base *p_scene)
{
	// Notify current scene it's going away?
	g_current_scene = p_scene;
}