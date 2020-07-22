#ifndef __SCENE_MANAGER_H_
#define __SCENE_MANAGER_H_

#include "scene_base.h"

void scene_manager_init();
void scene_manager_shutdown();

scene_base *scene_manager_get_current();
void scene_manager_set_current(scene_base *p_scene);

#endif /* __SCENE_MANAGER_H_ */