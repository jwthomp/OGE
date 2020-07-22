#include "assert.h"

static void default_message_handler(char const *p_msg)
{
	int *a = 0;
	*a = 0;
}

void (*g_assert_callback_handler)(char const *p_msg) = default_message_handler;


void assert_register_handler(void (*p_callback)(char const*p_msg))
{
	g_assert_callback_handler = p_callback;
}