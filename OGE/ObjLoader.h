/*===================================================================================================
**
**	Author	:	Robert Bateman
**	E-Mail	:	rbateman@bournemouth.ac.uk
**	Brief	:	This Sourcefile is part of a series explaining how to load and render Alias Wavefront
**				Files somewhat efficently. If you are simkply after a reliable Obj Loader, then I would
**				Recommend version8; or possibly version9 and the supplied loader for extreme efficency.
**
**	Note	:	This Source Code is provided as is. No responsibility is accepted by myself for any
**				damage to hardware or software caused as a result of using this code. You are free to
**				make any alterations you see fit to this code for your own purposes, and distribute
**				that code either as part of a source code or binary executable package. All I ask is
**				for a little credit somewhere for my work!
** 
**				Any source improvements or bug fixes should be e-mailed to myself so I can update the
**				code for the greater good of the community at large. Credit will be given to the 
**				relevant people as always....
**				
**
**				Copyright (c) Robert Bateman, www.robthebloke.org, 2004
**
**				
**				National Centre for Computer Animation,
**				Bournemouth University,
**				Talbot Campus,
**				Bournemouth,
**				BH3 72F,
**				United Kingdom
**				ncca.bournemouth.ac.uk
**	
**
===================================================================================================*/

/*
**	Sentry, prevent the header file from being included more than once
*/
#ifndef _OBJ_LOADER_H_
#define _OBJ_LOADER_H_

#ifdef __cplusplus
extern "C" {
#endif

	/*
	**	A wee typedef that holds a reference to the objfile
	*/
	typedef unsigned int ObjFile;

	/*
	**	func	:	LoadOBJ
	**	params	:	filename	-	the name of the file you wish to load
	**	returns	:	a reference ID for this file. Will return 0 if the file failed to load
	**	notes	:	loads the *.obj file up and stores all the data internally to make the
	**				source nice and easy to use. Each objfile loaded is stored as a node on
	**				a linked list and uses a single unsigned integer as an ID to allow you
	**				to reference it.
	*/
	extern ObjFile	LoadOBJ(const char *filename);

	/*
	**	func	:	DrawOBJ
	**	params	:	id	-	a reference ID to the mesh you wish to draw
	**	returns	:	nothing
	**	notes	:	Gets the node related to the reference number and draws its data.
	*/
	extern void		DrawOBJ(ObjFile id);

	/*
	**	func	:	DeleteOBJ
	**	params	:	id	-	a reference ID to the mesh you wish to delete
	**	returns	:	nothing
	**	notes	:	Deletes the requested mesh node from the linked list, and frees all the data
	*/
	extern void		DeleteOBJ(ObjFile id);

	/*
	**	func	:	CleanUpOBJ
	**	returns	:	nothing
	**	notes	:	Deletes all the currently loaded data. Handy for cleaning up all data easily
	*/
	extern void		CleanUpOBJ(void);


	/*
	**	func	:	SetLightPosition
	**	param	:	lx	-	the lights x coord
	**	param	:	ly	-	the lights y coord
	**	param	:	lz	-	the lights z coord
	**	returns	:	nothing
	**	notes	:	Deletes all the currently loaded data. Handy for cleaning up all data easily
	*/
	extern void SetLightPosition(float lx,float ly,float lz);

	/*
	**	
	*/
	extern void SetTextures(ObjFile id,const char BumpTex[],const char BaseTex[]);


	/*=========================================================================================
	**
	**	This section handle conversion from the immediate mode format to one of three other
	**	OpenGL rendering methods, display lists, vertex arrays or vertex buffer objects
	**
	**=======================================================================================*/

	#define DISPLAY_LIST 1
	#define VERTEX_ARRAY 2
	#define VERTEX_BUFFER_OBJECT 3

	/*
	**	func	:	ConvertMeshTo
	**	params	:	id	-	a reference ID to the mesh you wish to convert to either a display list or vertex array
	**				What	-	either the defines DISPLAY_LIST, VERTEX_ARRAY or VERTEX_BUFFER_OBJECT
	**	returns	:	nothing
	**	notes	:	Changes the representation of the Mesh node so that it either uses Immediate mode
	**				Display lists or vertex arrays. Bear in mind that once the mesh has been converted,
	**				It cannot be converted to another format.
	*/
	extern void		ConvertMeshTo(ObjFile id,unsigned char What);

#ifdef __cplusplus
}
#endif

#endif
