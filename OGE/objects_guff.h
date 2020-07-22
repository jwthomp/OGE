#ifndef __OBJECTS_GUFF_H_
#define __OBJECTS_GUFF_H_

class mesh;
class mesh_instance;

extern mesh const*g_mesh_BuildingA;
extern mesh const*g_mesh_BuildingB;
extern mesh const*g_mesh_BuildingC;

extern mesh const*g_mesh_CompoundBuildingA;
extern mesh const*g_mesh_CompoundBuildingB;
extern mesh const*g_mesh_CompoundBuildingC;
extern mesh const*g_mesh_CompoundBuildingD;

extern mesh const*g_mesh_Ground;
extern mesh const*g_mesh_Globe;

extern mesh const*g_mesh_PipeA;
extern mesh const*g_mesh_PipeACrossJoint;
extern mesh const*g_mesh_PipeAJoint;
extern mesh const*g_mesh_PipeATeeJoint;

extern mesh const*g_mesh_PipeB;
extern mesh const*g_mesh_PipeBCrossJoint;
extern mesh const*g_mesh_PipeBJoint;
extern mesh const*g_mesh_PipeBTeeJoint;

extern mesh const*g_mesh_PipeC;

extern mesh const*g_mesh_PipeCover;
extern mesh const*g_mesh_PipeRim;
extern mesh const*g_mesh_PipeRing;

extern mesh const*g_mesh_Platform;

extern mesh const*g_mesh_ship;

extern mesh const*g_mesh_CityBlockA;
extern mesh const*g_mesh_CityBlockB;
extern mesh const*g_mesh_CityBlockC;
extern mesh const*g_mesh_CityBlockD;
extern mesh const*g_mesh_CityBlockE;
extern mesh const*g_mesh_CityBlockF;
extern mesh const*g_mesh_CityBlockG;
extern mesh const*g_mesh_CityBlockH;
extern mesh const*g_mesh_CityBlockI;
extern mesh const*g_mesh_CityBlockJ;
extern mesh const*g_mesh_CityBlockK;
extern mesh const*g_mesh_CityBlockL;
extern mesh const*g_mesh_CityBlockM;
extern mesh const*g_mesh_CityBlockN;
extern mesh const*g_mesh_CityBlockO;
extern mesh const*g_mesh_CityBlockP;
extern mesh const*g_mesh_CityBlockQ;


void objects_guff_demi_redeems();

mesh_instance *objects_guff_create_instance(mesh const*p_mesh, 
											float p_x = 0.0f, float p_y = 0.0f, float p_z = 0.0f, 
											float p_xrot = 0.0f, float p_yrot = 0.0f, float p_zrot = 0.0f,
											float p_xscale = 1.0f, float p_yscale = 1.0f, float p_zscale = 1.0f);

#endif // __OBJECTS_GUFF_H_

