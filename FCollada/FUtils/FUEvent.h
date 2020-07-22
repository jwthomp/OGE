/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	This file was taken off the Protect project on 26-09-2005
*/

/**
	@file FUEvent.h
	This file contains templates to contain and trigger callback events.
*/


#ifndef _EVENT_H_
#define _EVENT_H_

#ifndef _FUNCTOR_H_
#include "FUtils/FUFunctor.h"
#endif // _FUNCTOR_H_

/**
	An event with no argument.
	@ingroup FUtils
*/
class FUEvent0
{
private:
	typedef IFunctor0<void> Handler;
	typedef fm::pvector<Handler> HandlerList;
	HandlerList handlers;

public:
	/** Constructor. */
	FUEvent0() {}

	/** Destructor. */
	~FUEvent0()
	{
		FUAssert(handlers.empty(), CLEAR_POINTER_VECTOR(handlers));
	}

	/** Retrieves the number of callbacks registered for this event.
		@return The number of callbacks registered. */
	size_t GetHandlerCount() { return handlers.size(); }

	/** Adds a new callback that handles the event.
		@param handle The object that contains the member function.
		@param _function The member function to callback. */
	template <class Class>
	void InsertHandler(Class* handle, void (Class::*_function)())
	{
		handlers.push_back(new FUFunctor0<Class, void>(handle, _function));
	}

	/** Adds a new callback that handles the event.
		@param _function The static function to callback. */
	void InsertHandler(void (*_function)())
	{
		handlers.push_back(new FUStaticFunctor0<void>(_function));
	}

	/** Releases and unregisters a callback that handles the event.
		@param handle The object that contains the member function.
		@param _function The member function callback to unregister. */
	template <class Class>
	void ReleaseHandler(Class* handle, void (Class::*_function)(void))
	{
		void* function = *(void**)&_function;
		typename HandlerList::iterator it;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			if ((*it)->Compare(handle, function))
			{
				delete (*it);
				handlers.erase(it);
                break;
			}
		}
	}

	/** Releases and unregisters a callback that handles the event.
		@param _function The static function callback to unregister. */
	void ReleaseHandler(void (*_function)(void))
	{
		void* function = *(void**)&_function;
		HandlerList::iterator it;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			if ((*it)->Compare(NULL, function))
			{
				delete (*it);
				handlers.erase(it);
                break;
			}
		}
	}

	/** Triggers the event.
		All the registered callbacks will be called, in reverse-order
		of their registration. */
	void operator()()
	{
		intptr_t index = handlers.size() - 1; 
		for (; index >= 0; --index) 
		{ 
			(*handlers[index])();
		}
	}
};

/**
	An event with one argument.
	@ingroup FUtils
*/
template <class Arg1>
class FUEvent1
{
private:
	typedef IFunctor1<Arg1, void> Handler;
	typedef fm::pvector<Handler> HandlerList;
	HandlerList handlers;

public:
	/** Constructor. */
	FUEvent1() {}

	/** Destructor. */
	~FUEvent1()
	{
		FUAssert(handlers.empty(), CLEAR_POINTER_VECTOR(handlers));
	}

	/** Retrieves the number of callbacks registered for this event.
		@return The number of callbacks registered. */
	size_t GetHandlerCount() { return handlers.size(); }

	/** Adds a new callback that handles the event.
		@param handle The object that contains the member function.
		@param _function The member function to callback. */
	template <class Class>
	void InsertHandler(Class* handle, void (Class::*_function)(Arg1))
	{
		handlers.push_back(NewFUFunctor1(handle, _function));
	}

	/** Adds a new callback that handles the event.
		@param _function The static function to callback. */
	void InsertHandler(void (*_function)(Arg1))
	{
		handlers.push_back(new FUStaticFunctor1<Arg1, void>(_function));
	}

	/** Releases and unregisters a callback that handles the event.
		@param handle The object that contains the member function.
		@param _function The member function callback to unregister. */
	template <class Class>
	void ReleaseHandler(Class* handle, void (Class::*_function)(Arg1))
	{
		typename HandlerList::iterator it;
		void* function = *(void**)&_function;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			if ((*it)->Compare(handle, function))
			{
				delete (*it);
				handlers.erase(it);
                break;
			}
		}
	}

	/** Releases and unregisters a callback that handles the event.
		@param _function The static function callback to unregister. */
	void ReleaseHandler(void (*_function)(Arg1))
	{
		void* function = *(void**)&_function;
		typename HandlerList::iterator it;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			if ((*it)->Compare(NULL, function))
			{
				delete (*it);
				handlers.erase(it);
                break;
			}
		}
	}

	/** Triggers the event.
		All the registered callbacks will be called, in reverse-order
		of their registration.
		@param argument1 A first argument. */
	void operator()(Arg1 argument1)
	{
		intptr_t index = handlers.size() - 1; 
		for (; index >= 0; --index) 
		{ 
			(*handlers[index])(argument1);
		} 
	}
};

/**
	An event with two argument.
	@ingroup FUtils
*/
template <class Arg1, class Arg2>
class FUEvent2
{
private:
	typedef IFunctor2<Arg1, Arg2, void> Handler;
	typedef fm::pvector<Handler> HandlerList;
	HandlerList handlers;

public:
	/** Constructor. */
	FUEvent2() {}

	/** Destructor. */
	~FUEvent2()
	{
		FUAssert(handlers.empty(), CLEAR_POINTER_VECTOR(handlers));
	}

	/** Retrieves the number of callbacks registered for this event.
		@return The number of callbacks registered. */
	size_t GetHandlerCount() { return handlers.size(); }

	/** Adds a new callback that handles the event.
		@param handle The object that contains the member function.
		@param _function The member function to callback. */
	template <class Class>
	void InsertHandler(Class* handle, void (Class::*_function)(Arg1, Arg2))
	{
		handlers.push_back(new FUFunctor2<Class, Arg1, Arg2, void>(handle, _function));
	}

	/** Adds a new callback that handles the event.
		@param _function The static function to callback. */
	void InsertHandler(void (*_function)(Arg1, Arg2))
	{
		handlers.push_back(new FUStaticFunctor2<Arg1, Arg2, void>(_function));
	}

	/** Releases and unregisters a callback that handles the event.
		@param handle The object that contains the member function.
		@param _function The member function callback to unregister. */
	template <class Class>
	void ReleaseHandler(Class* handle, void (Class::*_function)(Arg1, Arg2))
	{
		void* function = *(void**)&_function;
		typename HandlerList::iterator it;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			if ((*it)->Compare(handle, function))
			{
				delete (*it);
				handlers.erase(it);
				break;
			}
		}
	}

	/** Releases and unregisters a callback that handles the event.
		@param _function The static function callback to unregister. */
	void ReleaseHandler(void (*_function)(Arg1, Arg2))
	{
		void* function = *(void**)&_function;
		typename HandlerList::iterator it;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			if ((*it)->Compare(NULL, function))
			{
				delete (*it);
				handlers.erase(it);
				break;
			}
		}
	}

	/** Triggers the event.
		All the registered callbacks will be called, in reverse-order
		of their registration.
		@param argument1 A first argument.
		@param argument2 A second argument. */
	void operator()(Arg1 argument1, Arg2 argument2)
	{
		intptr_t index = handlers.size() - 1; 
		for (; index >= 0; --index) 
		{ 
			(*handlers[index])(argument1, argument2);
		}
	}
};

#endif // _EVENT_H_
