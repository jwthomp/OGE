/*
 *  core_lib.cpp
 *  OGE-osx
 *
 *  Created by Jeff Thompson on 7/29/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#include "SDL.h"

#include "core_lib.h"

bool core_lib_init()
{
	if(SDL_Init(SDL_INIT_TIMER) < 0 ) {
		/* Failed, exit. */
		fprintf( stderr, "Core initialization failed: %s\n",
					SDL_GetError( ) );
		SDL_Quit( );
		return false;
	}
	
	return true;
}
