/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FUDebug.h
	This file contains macros useful to write debugging output.
*/

#ifndef _DEBUG_H_
#define _DEBUG_H_

class FULogFile;

#define FU_LOG_DEBUG	0	/**< The lesser verbosity level. Used for debugging information. */
#define FU_LOG_WARNING	1	/**< The mid verbosity level. Used to indicate that something is wrong but the application can handle it. */
#define FU_LOG_ERROR	2	/**< The highest verbosity level. Used to indicate an important error that the application will avoid. */
#define FU_LOG_INDEX	3	/**< The number of verbosity levels. */

/**
	A debugging logging facility.
	Do not use this static class directly. Instead, use the macros listed below.
	This class is NOT used in FCollada. For FCollada-related error messages,
	see the FUError class.

	@see DEBUG_OUT DEBUG1_OUT DEBUG2_OUT
	@see WARNING_OUT WARNING1_OUT WARNING2_OUT
	@see ERROR_OUT ERROR1_OUT ERROR2_OUT
	@see FUError

	@ingroup FUtils
*/
class FCOLLADA_EXPORT FUDebug
{
private:
	/**	Block access to the constructors and destructors 
		as only static functions of the class will be used. */
	FUDebug();
	virtual ~FUDebug();

	/**	Outputs a string to the debug monitor.
		The formatted message is the first parameter. */
	static void DebugOut(uint8 verbosity, const char* message, ...);
#ifdef UNICODE
	static void DebugOut(uint8 verbosity, const fchar* message, ...); /**< See above. */
#endif // UNICODE

	/**	Outputs a string to the debug monitor.
		The formatted message is the first parameter.
		The second parameter is the variable parmeter list. */
	static void DebugOutV(uint8 verbosity, const char* message, va_list& vars);
#ifdef UNICODE
	static void DebugOutV(uint8 verbosity, const fchar* message, va_list& vars); /**< See above. */
#endif // UNICODE

	/**	Outputs a string to the debug monitor.
		The filename and line number are the first two parameters.
		The formatted message is the third parameter.
		The fourth parameter is the variable parameter list. */
	static void DebugOutV(uint8 verbosity, const char* filename, uint32 line, const char* message, va_list& vars);
#ifdef UNICODE
	static void DebugOutV(uint8 verbosity, const char* filename, uint32 line, const fchar* message, va_list& vars); /**< See above. */
#endif // UNICODE
	
	/**	The log file receiving warning and error messages. */
	static FULogFile* logFile;

	/**	The number of new debug, warning, and error messages since last checked. */
	static uint32 newMessages[FU_LOG_INDEX];

public:
	/**	Outputs a string to the debug monitor.
		@param verbosity The verbosity level at which this message will be triggered.
		@param filename The code filename that writes out the message. 
		@param line The code line that writes out the message.
		@param message The formatted log message. */
	static void DebugOut(uint8 verbosity, const char* filename, uint32 line, const char* message, ...);
#ifdef UNICODE
	static void DebugOut(uint8 verbosity, const char* filename, uint32 line, const fchar* message, ...); /**< See above. */
#endif // UNICODE

	/**	Allows to determine if new messages have been generated since the last query.
		@param debug The number of new debug-level messages. 
		@param warnings The number of new warning-level messages. 
		@param errors The number of new error-level messages. */
	static void QueryNewMessages(uint32& debug, uint32& warnings, uint32& errors);

	/** Sets the log file that will be used to write out the generated messages.
		@param _logFile The output log file. */
	static void SetLogFile(FULogFile* _logFile) {logFile = _logFile;}
};

/** Outputs a string to the debug monitor.
	@param token The string to output. */
#define DEBUG_OUT(token)	FUDebug::DebugOut(FU_LOG_DEBUG, __FILE__, __LINE__, token);
/** See above. */
#define WARNING_OUT(token)	FUDebug::DebugOut(FU_LOG_WARNING, __FILE__, __LINE__, token);
/** See above. */
#define ERROR_OUT(token)	FUDebug::DebugOut(FU_LOG_ERROR, __FILE__, __LINE__, token);

/** Outputs a string to the debug monitor.
	@param token The formatted string to output.
	@param arg1 A first argument. */
#define DEBUG_OUT1(token, arg1)		FUDebug::DebugOut(FU_LOG_DEBUG, __FILE__, __LINE__, token, arg1);
/** See above. */
#define WARNING_OUT1(token, arg1)	FUDebug::DebugOut(FU_LOG_WARNING, __FILE__, __LINE__, token, arg1);
/** See above. */
#define ERROR_OUT1(token, arg1)		FUDebug::DebugOut(FU_LOG_ERROR, __FILE__, __LINE__, token, arg1);

/** Outputs a string to the debug monitor.
	@param token The formatted string to output.
	@param arg1 A first argument.
	@param arg2 A second argument. */
#define DEBUG_OUT2(token, arg1, arg2)		FUDebug::DebugOut(FU_LOG_DEBUG, __FILE__, __LINE__, token, arg1, arg2);
/** See above. */
#define WARNING_OUT2(token, arg1, arg2)		FUDebug::DebugOut(FU_LOG_WARNING, __FILE__, __LINE__, token, arg1, arg2);
/** See above. */
#define ERROR_OUT2(token, arg1, arg2)		FUDebug::DebugOut(FU_LOG_ERROR, __FILE__, __LINE__, token, arg1, arg2);

#endif // _DEBUG_H_
