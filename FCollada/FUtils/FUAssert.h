/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FUAssert.h
	This file contains a simple debugging assertion mechanism.
*/

#ifndef _FU_ASSERT_H_
#define _FU_ASSERT_H_

/**
	Breaks into the debugger.
	In debug builds, this intentionally crashes the application.
	In release builds, this is an empty function.
*/

#ifdef _DEBUG
static bool ignoreAssert = false; //Definition is needed in other platforms too.
#ifdef WIN32
#define __DEBUGGER_BREAK \
	static bool ignoreAssert = false; \
	if (!ignoreAssert) { \
		char message[1024]; \
		snprintf(message, 1024, "[%s@%u] Assertion failed.\nAbort: Enter debugger.\nRetry: Continue execution.\nIgnore: Do not assert at this line for the duration of the application.", __FILE__, __LINE__); \
		message[1023] = 0; \
		int32 buttonPressed = MessageBoxA(NULL, message, "Assertion failed.", MB_ABORTRETRYIGNORE | MB_ICONWARNING); \
		if (buttonPressed == IDABORT) { \
			__debugbreak(); \
		} else if (buttonPressed == IDRETRY) {} \
		else if (buttonPressed == IDIGNORE) { ignoreAssert = true; } }

#else

// On non-Windows platforms: force a crash?
#define __DEBUGGER_BREAK { \
	ignoreAssert = false; \
	uint32* __p = NULL; \
	uint32 __v = *__p; \
	*__p = 0xD1ED0D1E; \
	__v = __v + 1; }

#endif // WIN32
#else

/**
	Breaks into the debugger.
	In debug builds, this intentionally crashes the application.
	In release builds, this is an empty function.
*/
#define __DEBUGGER_BREAK

#endif // _DEBUG

/** Forces the debugger to break, or take the fall-back.
	@param command The fall_back command to execute. */
#define FUFail(command) { __DEBUGGER_BREAK; command; }

/** Asserts that a condition is met.
	Use this macro, instead of 'if' statements
	when you are asserting for a programmer's error.
	@param condition The condition to assert.
	@param fall_back The command to execute if the condition is not met. */
#define FUAssert(condition, fall_back) { if (!(condition)) { __DEBUGGER_BREAK; fall_back; } }

#endif // _FU_ASSERT_H_
