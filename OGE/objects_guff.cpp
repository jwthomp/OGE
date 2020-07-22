/*
 *  objects_guff.cpp
 *  OGE-osx
 *
 *  Created by Jeff Thompson on 6/17/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#include "objects_guff.h"

#include "mesh.h"
#include "mesh_instance.h"
#include "resource_manager.h"
#include "quaternion.h"
#include "render_lib.h"
#include "mesh_instance_dynamic.h"
#include "obj_cloth.h"
#include "physics_lib.h"

#include <stdlib.h>

mesh const*g_mesh_BuildingA;
mesh const*g_mesh_BuildingB;
mesh const*g_mesh_BuildingC;

mesh const*g_mesh_CompoundBuildingA;
mesh const*g_mesh_CompoundBuildingB;
mesh const*g_mesh_CompoundBuildingC;
mesh const*g_mesh_CompoundBuildingD;

mesh const*g_mesh_Ground;
mesh const*g_mesh_Globe;

mesh const*g_mesh_PipeA;
mesh const*g_mesh_PipeACrossJoint;
mesh const*g_mesh_PipeAJoint;
mesh const*g_mesh_PipeATeeJoint;

mesh const*g_mesh_PipeB;
mesh const*g_mesh_PipeBCrossJoint;
mesh const*g_mesh_PipeBJoint;
mesh const*g_mesh_PipeBTeeJoint;

mesh const*g_mesh_PipeC;

mesh const*g_mesh_PipeCover;
mesh const*g_mesh_PipeRim;
mesh const*g_mesh_PipeRing;

mesh const*g_mesh_Platform;

mesh const*g_mesh_ship;

mesh const*g_mesh_CityBlockA;
mesh const*g_mesh_CityBlockB;
mesh const*g_mesh_CityBlockC;
mesh const*g_mesh_CityBlockD;
mesh const*g_mesh_CityBlockE;
mesh const*g_mesh_CityBlockF;
mesh const*g_mesh_CityBlockG;
mesh const*g_mesh_CityBlockH;
mesh const*g_mesh_CityBlockI;
mesh const*g_mesh_CityBlockJ;
mesh const*g_mesh_CityBlockK;
mesh const*g_mesh_CityBlockL;
mesh const*g_mesh_CityBlockM;
mesh const*g_mesh_CityBlockN;
mesh const*g_mesh_CityBlockO;
mesh const*g_mesh_CityBlockP;
mesh const*g_mesh_CityBlockQ;



void objects_guff_demi_redeems()
{
	g_mesh_BuildingA = resource_manager_get_mesh("BuildingA.obj");
	g_mesh_BuildingB = resource_manager_get_mesh("BuildingB.obj");
	g_mesh_BuildingC = resource_manager_get_mesh("BuildingC.obj");

	g_mesh_CompoundBuildingA = resource_manager_get_mesh("CompoundBuildingA.obj");
	g_mesh_CompoundBuildingB = resource_manager_get_mesh("CompoundBuildingB.obj");
	g_mesh_CompoundBuildingC = resource_manager_get_mesh("CompoundBuildingC.obj");
	g_mesh_CompoundBuildingD = resource_manager_get_mesh("CompoundBuildingD.obj");

	g_mesh_Ground = resource_manager_get_mesh("Ground.obj");
	g_mesh_Globe = resource_manager_get_mesh("Globe.obj");

	g_mesh_PipeA = resource_manager_get_mesh("PipeA.obj");
	g_mesh_PipeACrossJoint = resource_manager_get_mesh("PipeACrossJoint.obj");
	g_mesh_PipeAJoint = resource_manager_get_mesh("PipeAJoint.obj");
	g_mesh_PipeATeeJoint = resource_manager_get_mesh("PipeATeeJoint.obj");

	g_mesh_PipeB = resource_manager_get_mesh("PipeB.obj");
	g_mesh_PipeBCrossJoint = resource_manager_get_mesh("PipeBCrossJoint.obj");
	g_mesh_PipeBJoint = resource_manager_get_mesh("PipeBJoint.obj");
	g_mesh_PipeBTeeJoint = resource_manager_get_mesh("PipeBTeeJoint.obj");

	g_mesh_PipeC = resource_manager_get_mesh("PipeC.obj");

	g_mesh_PipeCover = resource_manager_get_mesh("PipeCover.obj");
	g_mesh_PipeRim = resource_manager_get_mesh("PipeRim.obj");
	g_mesh_PipeRing = resource_manager_get_mesh("PipeRing.obj");

	g_mesh_Platform = resource_manager_get_mesh("Platform.obj");
	
	g_mesh_ship = resource_manager_get_mesh("Ship.obj");

	g_mesh_CityBlockA = resource_manager_get_mesh("CityBlockA.obj");
	g_mesh_CityBlockB = resource_manager_get_mesh("CityBlockB.obj");
	g_mesh_CityBlockC = resource_manager_get_mesh("CityBlockC.obj");
	g_mesh_CityBlockD = resource_manager_get_mesh("CityBlockD.obj");
	g_mesh_CityBlockE = resource_manager_get_mesh("CityBlockE.obj");
	g_mesh_CityBlockF = resource_manager_get_mesh("CityBlockF.obj");
	g_mesh_CityBlockG = resource_manager_get_mesh("CityBlockG.obj");
	g_mesh_CityBlockH = resource_manager_get_mesh("CityBlockH.obj");
	g_mesh_CityBlockI = resource_manager_get_mesh("CityBlockI.obj");
	g_mesh_CityBlockJ = resource_manager_get_mesh("CityBlockJ.obj");
	g_mesh_CityBlockK = resource_manager_get_mesh("CityBlockK.obj");
	g_mesh_CityBlockL = resource_manager_get_mesh("CityBlockL.obj");
	g_mesh_CityBlockM = resource_manager_get_mesh("CityBlockM.obj");
	g_mesh_CityBlockN = resource_manager_get_mesh("CityBlockN.obj");
	g_mesh_CityBlockO = resource_manager_get_mesh("CityBlockO.obj");
	g_mesh_CityBlockP = resource_manager_get_mesh("CityBlockP.obj");
	g_mesh_CityBlockQ = resource_manager_get_mesh("CityBlockQ.obj");
}


mesh_instance *objects_guff_create_instance(mesh const*p_mesh, float p_x, float p_y, float p_z, float p_xrot, float p_yrot, float p_zrot,
											float p_xscale, float p_yscale, float p_zscale)
{
	mesh_instance *ml = new mesh_instance;
	ml->m_mesh = p_mesh;
	ml->m_type = RENDER_LIB_MESH_INSTANCE_TYPE_STATIC;
	
//	ml->m_position.set(p_x, p_y, p_z);

	quaternion x;
	quaternion y;
	quaternion z;
	x.CreateFromAxisAngle(1.0f, 0.0f, 0.0f, p_xrot);
	y.CreateFromAxisAngle(0.0f, 1.0f, 0.0f, p_yrot);
	z.CreateFromAxisAngle(0.0f, 0.0f, 1.0f, p_zrot);
	quaternion qout = x * y * z;

//	ml->m_orientation = x * y * z;

	ml->m_transform.set_values(&Vector3(p_x, p_y, p_z), &qout, &Vector3(p_xscale, p_yscale, p_zscale));


	render_lib_mesh_instance_add(ml);

#if 0
	obj_cloth *oc = new obj_cloth();
	oc->init();
	oc->set_obj(ml);
	physics_lib_cloth_sim_add(oc);
#endif
	return ml;
}
