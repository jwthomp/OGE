#ifndef __ASSERT_H_
#define __ASSERT_H_

extern void (*g_assert_callback_handler)(char const *p_msg);

#define assert(condition) {								\
	if (!(condition)) {										\
		g_assert_callback_handler(#condition);			\
	}																\
}																	


void assert_register_handler(void (*p_callback)(char const*p_msg));

#endif // __ASSERT_H_