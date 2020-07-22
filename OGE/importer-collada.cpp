#include "importer-collada.h"

#include "Vector3.h"
#include "mesh.h"
#include "assert.h"

#include "material.h"
#include "mesh_meta_data.h"
#include "render_lib.h"

// FCollada
#include "FCollada.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDSceneNode.h" 
#include "FCDocument/FCDLight.h" 
#include "FUtils/FUObject.h" 
#include "FCDocument/FCDGeometryPolygonsTools.h"
#include "FCDocument/FCDTransform.h"

#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectProfile.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDTexture.h"
#include "FCDocument/FCDImage.h"

#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDExtra.h"

#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDMaterial.h" 
#include "FCDocument/FCDMaterialInstance.h"
#include "FCDocument/FCDLibrary.h"

struct geometry_load_cb_data
{
	mesh *m_mesh_ptr;
	unsigned long m_render_block_index;
};

material * load_material(fstring const & p_mat_name, FCDGeometryInstance const* p_geom_instance)
{
	FCDMaterialInstance const *mat_instance = p_geom_instance->FindMaterialInstance(p_mat_name);
	assert(mat_instance != NULL);

	FCDMaterial const* mat = mat_instance->GetMaterial();

	FMVector4 color_diffuse;
	FMVector4 color_specular;
	FMVector4 color_ambient;

	material *render_mat = NULL;

	if(mat!=NULL) {
		FCDEffect const* fx = mat->GetEffect();

		FCDEffectProfile const* profile = fx->FindProfile(FUDaeProfileType::COMMON);
		FCDEffectStandard const* standardProfile = (FCDEffectStandard const *)profile;
		if (standardProfile) {
			color_ambient = standardProfile->GetAmbientColor();
			color_diffuse = standardProfile->GetDiffuseColor();
			color_specular = standardProfile->GetSpecularColor();
		} else {
			assert(!"Not a standard profile");
		}

		char material_name[MAX_MATERIAL_NAME_LENGTH];
#ifdef MAC_OS_X
		strncpy(material_name, imagen->GetName().c_str(), MAX_MATERIAL_NAME_LENGTH);
#else
		wchar_t const*texture_name = p_mat_name.c_str();
		assert(texture_name != NULL);

		assert(wcstombs(NULL, texture_name, 0) < MAX_MATERIAL_NAME_LENGTH);

		wcstombs(material_name, texture_name, MAX_MATERIAL_NAME_LENGTH);
#endif
		render_mat = material_create(material_name);
		assert(render_mat != NULL);

		if (render_mat->m_texture == NULL) {
			render_mat->m_color_ambient[0] = color_ambient.x;
			render_mat->m_color_ambient[1] = color_ambient.y;
			render_mat->m_color_ambient[2] = color_ambient.z;
			render_mat->m_color_ambient[3] = color_ambient.w;
			

			render_mat->m_color_diffuse[0] = color_diffuse.x;
			render_mat->m_color_diffuse[1] = color_diffuse.y;
			render_mat->m_color_diffuse[2] = color_diffuse.z;
			render_mat->m_color_diffuse[3] = color_diffuse.w;

			render_mat->m_color_spec[0] = color_specular.x;
			render_mat->m_color_spec[1] = color_specular.y;
			render_mat->m_color_spec[2] = color_specular.z;
			render_mat->m_color_spec[3] = color_specular.w;

			render_mat->m_shader = render_lib_get_default_shader();

			size_t sz = standardProfile->GetTextureCount(FUDaeTextureChannel::DIFFUSE);
			// We only support one texture in DIFFUSE channel
			assert(sz < 2);
			if(sz > 0) {
				FCDTexture const*textura = standardProfile->GetTexture(FUDaeTextureChannel::DIFFUSE,0);
				if(textura) {
					FCDImage const* imagen = textura->GetImage();
					
					char texture_name[MAX_TEXTURE_NAME_LENGTH];
	#ifdef MAC_OS_X
					strncpy(texture_name, imagen->GetName().c_str(), MAX_TEXTURE_NAME_LENGTH);
	#else
					wchar_t const*wtexture_name = imagen->GetFilename().c_str(); // GetName().c_str();
					assert(wtexture_name != NULL);

					assert(wcstombs(NULL, wtexture_name, 0) < MAX_TEXTURE_NAME_LENGTH);

					wcstombs(texture_name, wtexture_name, MAX_TEXTURE_NAME_LENGTH);
	#endif

					// Strip out to last part of name
					char *txt, *last_txt;
					last_txt = texture_name;

					txt = strtok(texture_name, "\\");
					while(txt) {
						last_txt = txt;
						txt = strtok(NULL, "\\");
					}

					material_load(render_mat, last_txt);
					
				}
			}
		}
	}

	return render_mat;
}

void geometry_count_cb(FCDGeometry *p_geom, FCDGeometryInstance const* p_geom_instance, FMMatrix44 const*p_matrix, void *p_data, fstring const* p_name)
{
	if (p_geom == NULL) {
		return;
	}

	FCDGeometryMesh* mesh = p_geom->GetMesh();
	if (mesh->IsTriangles() == false) {
		assert(!"Collada mesh has non-triangle primitives");
		return;
	} else {
		size_t nrPolygons = mesh->GetPolygonsCount();
		int &count = *(int *)p_data;
		count = count + (int)nrPolygons;
	}
}

void ParseSceneNodeRecursive(FCDocument* document, FCDSceneNode* inode, FMMatrix44 p_matrix, void (*p_callback)(FCDGeometry *, FCDGeometryInstance const*, FMMatrix44 const*, void *p_data, fstring const* p_name), void *p_data)
{
	size_t nbInstance = inode->GetInstanceCount();
	size_t nbChild = inode->GetChildrenCount();

	// Get transformation
	size_t tranform_count = inode->GetTransformCount();
	for (size_t i = 0; i < tranform_count; ++i) {
		FCDTransform const *trans = inode->GetTransform(i);
		switch (trans->GetType()) {
			case FCDTransform::TRANSLATION:
			case FCDTransform::ROTATION:
			case FCDTransform::SCALE:
			case FCDTransform::MATRIX:
			{
				FMMatrix44 mat = trans->ToMatrix();
				p_matrix = p_matrix * mat;
				break;
			}
			default:
				break;
		}
	}

	if (nbInstance == 0 || nbChild!=0) { // It's a group		
		//Deal with it
		// [...]

		if (nbChild == 0) {
			FCDExtra const* extra = inode->GetExtra();
			
			// This is meta data
			if (extra) {
				p_callback(NULL, NULL, &p_matrix, p_data, &inode->GetName());
			}

		} else {
			//Go through is children
			for (size_t c=0; c<nbChild; c++) {
				
				FCDSceneNode *cnode = inode->GetChild(c);
				ParseSceneNodeRecursive(document, cnode, p_matrix, p_callback, p_data);
			}
		}
	} else {
		for (size_t m=0; m<nbInstance; m++) {
			FCDEntityInstance * instance = inode->GetInstance(m);
			FCDEntity::Type type = instance->GetEntityType();

			if (type == FCDEntity::GEOMETRY) {				
				//get the mesh
				//instance->GetEntity();
				FCDGeometryInstance *geo_instance = (FCDGeometryInstance *)instance;


				FCDGeometry *geom = (FCDGeometry *)instance->GetEntity();
				if (geom->IsMesh()) {
					p_callback(geom, geo_instance, &p_matrix, p_data, NULL);
					//fstring const& fs = inode->GetName();
					//wchar_t const*wc = fs.c_str();
				}
			}
		}
	}
}


unsigned long count_geometries(FCDocument *document)
{
	FCDVisualSceneNodeLibrary* vsl = document->GetVisualSceneLibrary();
	size_t nbEntity = vsl->GetEntityCount();
	unsigned long count = 0;

	FMMatrix44 mat = FMMatrix44::Identity;

	for (size_t i = 0; i < nbEntity; ++i) {
		FCDSceneNode* inode = vsl->GetEntity(i);
		ParseSceneNodeRecursive(document, inode, mat, &geometry_count_cb, &count);
	}

	return count;
}

void load_mesh(mesh *p_mesh_ptr, unsigned long &p_render_block_index, FCDGeometryMesh *p_collada_mesh, FCDGeometryInstance const* p_geom_instance, FMMatrix44 const*p_matrix)
{
	FCDGeometryPolygonsTools::GenerateUniqueIndices(p_collada_mesh);

	if (p_collada_mesh->IsTriangles() == false) {
		assert(!"Collada mesh has non-triangle primitives");
		return;
	} else {
		size_t nrPolygons = p_collada_mesh->GetPolygonsCount();

		for (unsigned long i = 0; i < nrPolygons; i++) {
			// get first polygonal object
			FCDGeometryPolygons* polygon = p_collada_mesh->GetPolygons(i);

			// get float data sources
			// TODO: How about multiple texture coordinates?
			FCDGeometrySource* positionSource = p_collada_mesh->FindSourceByType(FUDaeGeometryInput::POSITION);
			FCDGeometrySource* normalSource   = p_collada_mesh->FindSourceByType(FUDaeGeometryInput::NORMAL);
			FCDGeometrySource* texcoordSource = p_collada_mesh->FindSourceByType(FUDaeGeometryInput::TEXCOORD);
			FCDGeometrySource* colorSource    = p_collada_mesh->FindSourceByType(FUDaeGeometryInput::COLOR);

			if (positionSource == NULL) {
				assert(!"Collada mesh has no position source");
				return;
			}

			FCDGeometryPolygonsInput* positionInput = NULL;
			FCDGeometryPolygonsInput* normalInput = NULL;
			FCDGeometryPolygonsInput* texcoordInput = NULL;
			FCDGeometryPolygonsInput* colorInput = NULL;					
			
			positionInput = polygon->FindInput(positionSource);
			normalInput = polygon->FindInput(normalSource);
			texcoordInput = polygon->FindInput(texcoordSource);
			colorInput = polygon->FindInput(colorSource);
		
			// get index lists
			uint32* positionIndices = NULL;
			uint32* normalIndices   = NULL;
			uint32* texcoordIndices = NULL;
			uint32* colorIndices    = NULL;

			size_t position_index_count = 0;
			size_t normal_index_count = 0;
			size_t texcoord_index_count = 0;
			size_t color_index_count = 0;

			if (positionInput) {
				position_index_count = positionInput->GetIndexCount();
				positionIndices = positionInput->GetIndices();
			}

			if (normalInput) {
				normal_index_count = normalInput->GetIndexCount();
				normalIndices = normalInput->GetIndices();
			}

			if (texcoordInput) {
				texcoord_index_count = texcoordInput->GetIndexCount();
				texcoordIndices = texcoordInput->GetIndices();
			}

			if (colorInput) {
				color_index_count = colorInput->GetIndexCount();
				colorIndices = colorInput->GetIndices();
			}

			render_block &render_block_ptr = p_mesh_ptr->m_render_blocks[p_render_block_index];

			render_block_ptr.m_prepared = false;
			render_block_ptr.m_format = RENDER_LIB_MESH_FORMAT_VA_TRIANGLES;
			render_block_ptr.m_vertex_count = 0;
			render_block_ptr.m_index_count = 0;

			render_block_ptr.m_index_count = (unsigned long)position_index_count;
			render_block_ptr.m_index_buffer = (unsigned long *)malloc(sizeof(unsigned long) * render_block_ptr.m_index_count);
			memcpy(render_block_ptr.m_index_buffer, positionIndices, position_index_count * sizeof(unsigned long));

			float *data = positionSource->GetData();
			size_t len2 = positionSource->GetDataCount();

			render_block_ptr.m_vertex_count = (unsigned long)len2 / 3;

			render_block_ptr.m_pos = (Vector3 *)malloc(sizeof(Vector3) * render_block_ptr.m_vertex_count);
			memcpy(render_block_ptr.m_pos, data, sizeof(Vector3) * render_block_ptr.m_vertex_count);

			// Transform all read in verts
			matrix44 transform_matrix;
			memcpy(transform_matrix.m_data, p_matrix->m, sizeof(p_matrix->m));


			
			for (unsigned long i = 0; i < render_block_ptr.m_vertex_count; i++) {
				Vector3 out = transform_matrix * render_block_ptr.m_pos[i];
				render_block_ptr.m_pos[i].set(out.m_data[0], out.m_data[1], out.m_data[2]);
			}
				
			data = texcoordSource->GetData();
			len2 = texcoordSource->GetDataCount();
			render_block_ptr.m_uv = (uv_coord *)malloc(sizeof(uv_coord) * render_block_ptr.m_vertex_count);
			
			// Test for whether this is a two or three coordinate texture
			if (texcoordSource->GetStride() == 3) {
				for (unsigned long i = 0; i < render_block_ptr.m_vertex_count; i++) {
					render_block_ptr.m_uv[i].m_data[0] = data[i * 3];
					render_block_ptr.m_uv[i].m_data[1] = data[(i * 3) + 1];
				}
			} else {
				memcpy(render_block_ptr.m_uv, data, sizeof(uv_coord) * render_block_ptr.m_vertex_count);
			}

			data = normalSource->GetData();
			len2 = normalSource->GetDataCount();
			render_block_ptr.m_normal = (Vector3 *)malloc(sizeof(Vector3) * render_block_ptr.m_vertex_count);
			memcpy(render_block_ptr.m_normal, data, sizeof(Vector3) * render_block_ptr.m_vertex_count);

			// Find material for this polygon
			fstring const& mat_name = polygon->GetMaterialSemantic();
			material const *render_mat = load_material(mat_name.c_str(), p_geom_instance);
			render_block_ptr.m_material = render_mat;

			p_render_block_index++;
		}
	}
}

void add_meta_to_mesh(mesh * p_mesh_ptr, fstring const* p_name, FMMatrix44 const* p_matrix)
{
	Vector3 origin(0.0f, 0.0f, 0.0f);
	matrix44 transform_matrix;

	memcpy(transform_matrix.m_data, p_matrix->m, sizeof(p_matrix->m));
	Vector3 position = transform_matrix * origin;

	char name[MAX_META_NAME_LENGTH];

#ifdef MAC_OS_X
	char const* meta_name = p_name->c_str();
	assert(meta_name != NULL);
	strncpy(name, meta_name, MAX_META_NAME_LENGTH);
#else
	wchar_t const*meta_name = p_name->c_str();
	assert(meta_name != NULL);

	assert(wcstombs(NULL, meta_name, 0) < MAX_META_NAME_LENGTH);

	wcstombs(name, meta_name, MAX_META_NAME_LENGTH);
#endif


	p_mesh_ptr->m_meta_data.data_add(name, &position);
}

void geometry_load_cb(FCDGeometry *p_geom, FCDGeometryInstance const* p_geom_instance, FMMatrix44 const*p_matrix, void *p_data, fstring const* p_name)
{
	geometry_load_cb_data &cb_data = *(geometry_load_cb_data *)p_data;

	if (p_geom == NULL) {
		// We have meta data, so load that in
		add_meta_to_mesh(cb_data.m_mesh_ptr, p_name, p_matrix);
	} else {
		// We have geometry so load it in	
		FCDGeometryMesh* mesh = p_geom->GetMesh();
		load_mesh(cb_data.m_mesh_ptr, cb_data.m_render_block_index, mesh, p_geom_instance, p_matrix);
	}
}

void load_geometries(mesh *p_mesh_ptr, FCDocument *p_document)
{
	FCDVisualSceneNodeLibrary* vsl = p_document->GetVisualSceneLibrary();
	size_t nbEntity = vsl->GetEntityCount();
	unsigned long count = 0;

	geometry_load_cb_data cb_data;
	cb_data.m_mesh_ptr = p_mesh_ptr;
	cb_data.m_render_block_index = 0;

	FMMatrix44 mat = FMMatrix44::Identity;

	for (size_t i = 0; i < nbEntity; ++i) {
		FCDSceneNode* inode = vsl->GetEntity(i);
		ParseSceneNodeRecursive(p_document, inode, mat, &geometry_load_cb, &cb_data);
	}
}

mesh *importer_collada_load(char const* p_mesh_name)
{	
// open dae file
	FCDocument *document = FCollada::NewTopDocument();
	bool z_is_up = true;

	bool ret = document->LoadFromFile(FUStringConversion::ToFString(p_mesh_name));
	if (ret == false) {
		return NULL;
	}

	// 3dsmax, maya and opengl have up axis differnt
	// with this information  we could rotate the model
	if ((document->GetAsset()->GetUpAxis().z) == 1) {
		z_is_up=true;
	} else {
		z_is_up=false;
	}

	unsigned long num_geometries = count_geometries(document);

	mesh *mesh_ptr = new mesh;
	mesh_ptr->m_render_block_count = num_geometries;
	mesh_ptr->m_render_blocks = (render_block *)malloc(sizeof(render_block) * num_geometries);


	// Traverse visual scene and load data
	load_geometries(mesh_ptr, document);

#if 0
	// how many geometries there are?
	FCDGeometryLibrary* geolib = document->GetGeometryLibrary();

#if 0
	unsigned long num_geometries = 0;
	for (int i = 0; i < (int) geolib->GetEntityCount(); i++) {
		if (geolib->GetEntity(i)->IsMesh() == true) {
			FCDGeometryMesh* mesh = geolib->GetEntity(i)->GetMesh();
			num_geometries += mesh->GetPolygonsCount();
		}
	}
#endif

	unsigned long render_block_index = 0;

	FCDGeometry *geom;
	int entity_count = geolib->GetEntityCount();
	for (int i = 0; i < (int) entity_count; i++) {
		geom = geolib->GetEntity(i);

		if (geom->IsMesh()) {
			

		}
	}
#endif

	return mesh_ptr;
}