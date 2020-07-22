#include "frametime.h"

#include "SDL.h"

#define START_FRAMETIME_MS (33)

static uint32 g_frametime_last_ms;
static uint32 g_frametime_ms;

static uint32 g_framecount = 0;


void frametime_init()
{
	g_frametime_ms = START_FRAMETIME_MS;
	g_frametime_last_ms = SDL_GetTicks();
	g_framecount = 0;
}

void frametime_process()
{
	uint32 tmp_ticks = SDL_GetTicks();
	g_frametime_ms = tmp_ticks - g_frametime_last_ms;
	g_frametime_last_ms = tmp_ticks;

	g_framecount++;
}

unsigned long frametime_get()
{
	return g_frametime_ms;
}

uint32 frametime_get_count()
{
	return g_framecount;
}