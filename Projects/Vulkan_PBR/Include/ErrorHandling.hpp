#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <utility>

template<typename TReturn, typename TClass, typename ... TArguments>
void RunFunctionAndCatchException(TReturn(TClass::*Function)(TArguments...), TClass* Object, TArguments&& ... Arguments);

template<typename TReturn, typename TClass, typename ... TArguments>
void RunFunctionAndCatchException<TReturn(TClass::*)(TArguments...)>(TReturn (TClass::*Function)(TArguments...), TClass* Object, TArguments&& ... Arguments)
{
	try
	{
		::MessageBox(nullptr, TEXT("Running Member Function"), TEXT("Error Handling"), MB_OK);
		(Object->*Function)(std::forward<TArguments>(Arguments)...);
	}
	catch (...)
	{
		::MessageBox(nullptr, TEXT("An error has occurred. Click Ok to exit."), TEXT("Fatal Error"), MB_OK);
		::PostQuitMessage(EXIT_FAILURE);
	}
}
