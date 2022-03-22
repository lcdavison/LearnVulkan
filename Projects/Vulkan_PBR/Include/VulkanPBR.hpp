#pragma once

#include <Windows.h>

#include "CommonTypes.hpp"

namespace Application
{
	struct ApplicationState
	{
		HWND WindowHandle;
		bool bRunning;
	};

	TCHAR const * kWindowClassName = TEXT("Vulkan-PBR");
	TCHAR const * kWindowName = TEXT("PBR Window");

	uint32 const kApplicationVersionNo = 0u;

	uint32 const kDefaultWindowWidth = 1280u;
	uint32 const kDefaultWindowHeight = 720u;

	extern ApplicationState State;
}
